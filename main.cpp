#include <drogon/drogon.h>
#include "CORSFilter.hpp"
#include "StructuredLog.hpp"
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <filesystem> // For C++17 and above
#include <memory>
#include <mutex>
#include <trantor/utils/AsyncFileLogger.h>

namespace {

const std::string LOG_DIRECTORY = "var/output/logs/";
const std::string LOG_BASE_NAME = "server_log";
const std::string LOG_EXT_NAME = ".txt";

// Bounds total log disk usage to LOG_FILE_SIZE_LIMIT * LOG_MAX_FILES ≈ 15.6 GiB, under the
// 16 GB budget. trantor::AsyncFileLogger enforces both natively: the size limit forces an
// extra rotation if a single hour is unusually chatty, and setMaxFiles prunes the oldest
// rotated files once the count is exceeded (it rescans the directory on each new instance,
// so this still works with our one-AsyncFileLogger-per-hour approach).
constexpr uint64_t LOG_FILE_SIZE_LIMIT = 50ull * 1024 * 1024;  // 50 MiB per rotated file
constexpr size_t LOG_MAX_FILES = 320;                          // 320 * 50 MiB ≈ 15.6 GiB retained

std::mutex hourlyLoggerMutex;
std::shared_ptr<trantor::AsyncFileLogger> hourlyLogger;

// Mirrors trantor's own default sink (fwrite to stdout) so `docker logs` keeps working,
// in addition to writing into the active hourly log file.
void hourlyLoggerOutput(const char* msg, const uint64_t len) {
    std::fwrite(msg, 1, static_cast<size_t>(len), stdout);
    std::shared_ptr<trantor::AsyncFileLogger> logger;
    {
        std::lock_guard<std::mutex> lock(hourlyLoggerMutex);
        logger = hourlyLogger;
    }
    if (logger) logger->output(msg, len);
}

void hourlyLoggerFlush() {
    std::fflush(stdout);
    std::shared_ptr<trantor::AsyncFileLogger> logger;
    {
        std::lock_guard<std::mutex> lock(hourlyLoggerMutex);
        logger = hourlyLogger;
    }
    if (logger) logger->flush();
}

// Swaps in a fresh AsyncFileLogger writing to LOG_DIRECTORY/server_log.txt. Destroying the
// previous logger (once in-flight output() calls release it) triggers its own rename of the
// completed file to a timestamped name, so each hour ends up as its own file.
void startNewHourlyLogFile() {
    auto logger = std::make_shared<trantor::AsyncFileLogger>();
    logger->setFileName(LOG_BASE_NAME, LOG_EXT_NAME, LOG_DIRECTORY);
    logger->setFileSizeLimit(LOG_FILE_SIZE_LIMIT);
    logger->setMaxFiles(LOG_MAX_FILES);
    logger->startLogging();
    std::lock_guard<std::mutex> lock(hourlyLoggerMutex);
    hourlyLogger = logger;
}

void scheduleHourlyLogRotation() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    auto nextHourTimeT = system_clock::to_time_t(now);
    std::tm nextHourTm{};
    gmtime_r(&nextHourTimeT, &nextHourTm);
    nextHourTm.tm_min = 0;
    nextHourTm.tm_sec = 0;
    nextHourTm.tm_hour += 1;
    const auto nextHour = system_clock::from_time_t(timegm(&nextHourTm));
    const double delaySeconds = duration<double>(nextHour - now).count();

    drogon::app().getLoop()->runAfter(delaySeconds, [] {
        startNewHourlyLogFile();
        scheduleHourlyLogRotation();
    });
}

}  // namespace

int main() {
    // Line-buffer stdout so mirrored log lines reach `docker logs -f` promptly instead of
    // waiting for the libc block buffer to fill (stdout is fully-buffered once redirected).
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    const char* configFile = std::getenv("CONFIG_FILE");
    drogon::app().loadConfigFile(configFile ? configFile : "config.json");

    // app.custom_config.sync_logging = true makes *every* request log synchronously (direct
    // write + fdatasync per line, mirrored to stderr) so lines survive a crash. Individual
    // requests can still opt in on their own with the `X-Sync-Log: 1` header regardless of
    // this default. See src/logging/StructuredLog.hpp.
    if (drogon::app().getCustomConfig().get("sync_logging", false).asBool()) {
        slog::setGlobalSyncLogging(true);
    }

    // File logging: var/output/logs/server_log.txt, rotated into its own timestamped
    // file at the top of every hour. Console output (docker logs) keeps working too.
    std::filesystem::create_directories(LOG_DIRECTORY);
    startNewHourlyLogFile();
    trantor::Logger::setOutputFunction(hourlyLoggerOutput, hourlyLoggerFlush);
    drogon::app().registerBeginningAdvice([] { scheduleHourlyLogRotation(); });

    // Apply CORS headers to all outgoing responses
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    });

    drogon::app().registerHandler(
        "/api/{_:.*}",
        [](const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            callback(resp);
        },
        { drogon::Options }  // Respond only to OPTIONS
    );

    drogon::app().registerHandler("/", [](const drogon::HttpRequestPtr& req,
        std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody("<h1>ASCII Game Backend is LIVE</h1>");
        callback(resp);
    });

    drogon::app().run();
    return 0;
}
