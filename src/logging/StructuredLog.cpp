#include "StructuredLog.hpp"
#include "CodeEnum.hpp"
#include <trantor/utils/Logger.h>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <random>
#include <sys/syscall.h>
#include <unistd.h>

// This file is the only place that knows how to turn a (level, function, event, fields)
// tuple into bytes on the wire. It writes through trantor's LOG_RAW - a Logger that emits
// exactly what's streamed into it with no added prefix/suffix - so the line lands in
// whatever sink main.cpp has wired up via trantor::Logger::setOutputFunction (the hourly
// rotating file + stdout mirror), with no changes needed there.

namespace slog {

namespace {

const char* levelName(Level level) {
    switch (level) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

trantor::Logger::LogLevel toTrantorLevel(Level level) {
    switch (level) {
        case Level::Debug: return trantor::Logger::kDebug;
        case Level::Info:  return trantor::Logger::kInfo;
        case Level::Warn:  return trantor::Logger::kWarn;
        case Level::Error: return trantor::Logger::kError;
    }
    return trantor::Logger::kInfo;
}

// Matches trantor's own thread id (see trantor::Logger's threadId_) so `thread_id` lines
// up with what ops already correlates against from other trantor-emitted lines.
long currentThreadId() {
    static thread_local long tid = static_cast<long>(::syscall(SYS_gettid));
    return tid;
}

std::string isoTimestampNow() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto micros = duration_cast<microseconds>(now.time_since_epoch()).count();
    const std::time_t seconds = static_cast<std::time_t>(micros / 1000000);
    const long microsOfSecond = static_cast<long>(micros % 1000000);
    std::tm utc{};
    gmtime_r(&seconds, &utc);
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                  utc.tm_hour, utc.tm_min, utc.tm_sec, microsOfSecond);
    return buf;
}

// Trims a __FILE__ path down to its basename, e.g. "ApiController.cpp" rather than the
// full build path.
const char* baseName(const char* path) {
    const char* base = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return base;
}

}  // namespace

nlohmann::json codeError(int code) {
    return { {"type", code_to_text(code)}, {"code", code} };
}

void emit(Level level, const char* function, const char* event, const nlohmann::json& fields, SourceLoc loc) {
    if (toTrantorLevel(level) < trantor::Logger::logLevel()) return;

    nlohmann::json out;
    out["timestamp"] = isoTimestampNow();
    out["level"] = levelName(level);
    out["thread_id"] = currentThreadId();
    out["function"] = function;
    out["event"] = event;
    if (fields.is_object()) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            out[it.key()] = it.value();
        }
    }
    out["source"] = { {"file", baseName(loc.file)}, {"line", loc.line} };

    // dump() with no arguments produces compact, single-line JSON - never pretty-printed -
    // which is exactly what NDJSON requires. LOG_RAW writes the stream verbatim with no
    // extra formatting of its own.
    LOG_RAW << out.dump() << "\n";
}

std::string newRequestId() {
    static std::atomic<uint64_t> counter{0};
    thread_local std::mt19937_64 rng{std::random_device{}()};
    const uint64_t random = rng();
    const uint64_t sequence = counter.fetch_add(1, std::memory_order_relaxed);
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%08llx%08llx",
                  static_cast<unsigned long long>(random & 0xFFFFFFFFull),
                  static_cast<unsigned long long>(sequence));
    return buf;
}

}  // namespace slog

RequestLog::RequestLog(const char* function, const drogon::HttpRequestPtr& req, slog::SourceLoc loc)
    : function_(function),
      method_(req->methodString()),
      path_(req->path()),
      requestId_(slog::newRequestId()),
      startTime_(std::chrono::steady_clock::now())
{
    slog::emit(slog::Level::Info, function_, "http_request_started",
               { {"method", method_}, {"path", path_}, {"request_id", requestId_} }, loc);
}

void RequestLog::emitLifecycle(slog::Level level, const char* event, const std::string& message,
                                const nlohmann::json& fields, slog::SourceLoc loc) const
{
    nlohmann::json payload = { {"method", method_}, {"path", path_}, {"request_id", requestId_} };
    const auto durationMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - startTime_).count();
    payload["duration_ms"] = durationMs;
    if (!message.empty()) payload["message"] = message;
    if (fields.is_object()) {
        for (auto it = fields.begin(); it != fields.end(); ++it) payload[it.key()] = it.value();
    }
    slog::emit(level, function_, event, payload, loc);
}

void RequestLog::completed(int statusCode, const char* event, const std::string& message,
                            const nlohmann::json& fields, slog::SourceLoc loc) const
{
    nlohmann::json payload = fields.is_object() ? fields : nlohmann::json::object();
    payload["status"] = statusCode;
    emitLifecycle(slog::Level::Info, event, message, payload, loc);
}

void RequestLog::failed(int statusCode, const std::string& errorMessage, const std::string& errorType,
                         const nlohmann::json& fields, const char* event, slog::SourceLoc loc) const
{
    nlohmann::json payload = fields.is_object() ? fields : nlohmann::json::object();
    payload["status"] = statusCode;

    nlohmann::json error = nlohmann::json::object();
    if (payload.contains("error") && payload["error"].is_object()) {
        error = payload["error"];  // caller pre-populated type/code via slog::codeError(...)
    }
    error["message"] = errorMessage;
    if (!errorType.empty()) error["type"] = errorType;
    payload["error"] = error;

    emitLifecycle(slog::Level::Error, event, errorMessage, payload, loc);
}

void RequestLog::debug(const char* event, const nlohmann::json& fields, slog::SourceLoc loc) const
{
    nlohmann::json payload = fields.is_object() ? fields : nlohmann::json::object();
    payload["request_id"] = requestId_;
    slog::emit(slog::Level::Debug, function_, event, payload, loc);
}
