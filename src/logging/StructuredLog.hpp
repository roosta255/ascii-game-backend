#pragma once

// Structured (NDJSON) request/event logging.
//
// Every emitted line is exactly one compact JSON object - one event per line, safe to
// stream into a log aggregator (Loki/ELK/etc). This is the one place in the server that
// knows how to format a log line; callers only ever supply *data* (an event name and a
// handful of fields), never text to format themselves. See src/logging/StructuredLog.cpp
// for the field/event-name conventions.
//
// Typical use, at the top of an HTTP handler:
//
//   RequestLog rlog("getMatch", req);                       // emits http_request_started
//   ...
//   rlog.failed(404, "Failed to load match", "", {{"error", slog::codeError(error)}});
//   ...
//   rlog.completed(200, "match_loaded", "Match loaded", {{"match_id", matchId}});
//
// file/line are captured automatically at the call site (a GCC/Clang extension every
// compiler this project builds with supports) - never pass them yourself.
//
// Anything a query might filter or group by (ids, status codes, enum names) belongs in
// the `fields` json object, not folded into `message` - message is for humans only.

#include <drogon/HttpRequest.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <string>

namespace slog {

enum class Level { Debug, Info, Warn, Error };

// Captures __FILE__/__LINE__ *at the call site* of whatever function has this as a
// trailing default argument - callers never mention it.
struct SourceLoc {
    const char* file;
    int line;
    SourceLoc(const char* f = __builtin_FILE(), int l = __builtin_LINE())
        : file(f), line(l) {}
};

// Builds {"type": code_to_text(code), "code": code} for the `error` field of a failed()
// call. Reuse this wherever a CodeEnum is available so error.type stays an exact,
// queryable enum name instead of something parsed out of a message string.
nlohmann::json codeError(int code);

// Emits one JSON line if `level` is at or above the configured threshold (the same
// app.log.log_level used by LOG_INFO/LOG_DEBUG elsewhere). `function` is the logical
// handler/component name (e.g. "getMatch") - not necessarily the enclosing C++ function,
// since some handlers delegate their real work elsewhere. `fields` is merged into the
// top-level JSON object.
void emit(Level level,
          const char* function,
          const char* event,
          const nlohmann::json& fields = nlohmann::json::object(),
          SourceLoc loc = SourceLoc());

// A short opaque id correlating every log line belonging to one request.
std::string newRequestId();

}  // namespace slog

// Binds one HTTP request's method/path/request_id/start-time so a handler never repeats
// them (and can't forget to, or let them drift out of sync between its log calls).
// Construct exactly one per incoming request - it emits "http_request_started"
// immediately. A handler that only validates input before delegating to a shared worker
// (see ApiController.cpp's performCharacterAction family) should pass its RequestLog down
// rather than constructing a second one, so every line for one request shares one
// request_id.
class RequestLog {
public:
    RequestLog(const char* function, const drogon::HttpRequestPtr& req,
               slog::SourceLoc loc = slog::SourceLoc());

    // event defaults to the generic completion event; pass a more specific vocabulary
    // name (e.g. "match_loaded", "door_activated") when this outcome has one.
    void completed(int statusCode,
                   const char* event = "http_request_completed",
                   const std::string& message = "",
                   const nlohmann::json& fields = nlohmann::json::object(),
                   slog::SourceLoc loc = slog::SourceLoc()) const;

    // Populates error.message (from errorMessage) and, when given, error.type - so
    // consumers never need to parse `message` to find out what kind of failure this was.
    // Pass a pre-built {"error": slog::codeError(code)} in `fields` instead of `errorType`
    // when a CodeEnum is available; its "type"/"code" are preserved alongside error.message.
    void failed(int statusCode,
                const std::string& errorMessage,
                const std::string& errorType = "",
                const nlohmann::json& fields = nlohmann::json::object(),
                const char* event = "http_request_failed",
                slog::SourceLoc loc = slog::SourceLoc()) const;

    // Anything worth recording between started/completed - a request body dump, codeset
    // state, etc. Always DEBUG level.
    void debug(const char* event,
               const nlohmann::json& fields = nlohmann::json::object(),
               slog::SourceLoc loc = slog::SourceLoc()) const;

    const std::string& requestId() const { return requestId_; }

private:
    void emitLifecycle(slog::Level level, const char* event, const std::string& message,
                        const nlohmann::json& fields, slog::SourceLoc loc) const;

    const char* function_;
    std::string method_;
    std::string path_;
    std::string requestId_;
    std::chrono::steady_clock::time_point startTime_;
};
