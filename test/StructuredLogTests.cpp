#include "StructuredLog.hpp"
#include "CodeEnum.hpp"
#include <catch2/catch_test_macros.hpp>
#include <drogon/HttpRequest.h>
#include <trantor/utils/Logger.h>
#include <mutex>
#include <sstream>
#include <vector>

// Verifies the structured logging abstraction itself (src/logging/StructuredLog.*): every
// emitted event is one line of valid, NDJSON-compatible JSON, common fields are always
// present and consistently typed, and callers' structured data lands as real top-level
// fields rather than being folded into the free-text `message`.

namespace {

std::mutex captureMutex;
std::string captureBuffer;

void captureOutput(const char* msg, const uint64_t len) {
    std::lock_guard<std::mutex> lock(captureMutex);
    captureBuffer.append(msg, len);
}

void captureFlush() {}

// Installed once for the whole test binary: redirects trantor's sink into captureBuffer
// and makes sure DEBUG-level events aren't filtered by whatever level the process starts
// at, so every TEST_CASE below sees everything it emits.
struct CaptureInstaller {
    CaptureInstaller() {
        trantor::Logger::setLogLevel(trantor::Logger::kTrace);
        trantor::Logger::setOutputFunction(captureOutput, captureFlush);
    }
};
const CaptureInstaller installer;

size_t bufferSize() {
    std::lock_guard<std::mutex> lock(captureMutex);
    return captureBuffer.size();
}

// Every complete line appended to the capture buffer since `sizeBefore` - i.e. everything
// a test emitted, ignoring whatever earlier tests (or other trantor logging) left behind.
std::vector<std::string> linesSince(size_t sizeBefore) {
    std::string tail;
    {
        std::lock_guard<std::mutex> lock(captureMutex);
        tail = captureBuffer.substr(sizeBefore);
    }
    std::vector<std::string> lines;
    std::stringstream ss(tail);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

drogon::HttpRequestPtr makeRequest(drogon::HttpMethod method, const std::string& path, const std::string& body = "") {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(method);
    req->setPath(path);
    if (!body.empty()) req->setBody(body);
    return req;
}

}  // namespace

TEST_CASE("slog::emit writes exactly one line of valid JSON carrying the common fields", "[structured-log]") {
    const auto before = bufferSize();
    slog::emit(slog::Level::Info, "unitTest", "widget_created", {{"widget_id", "abc123"}});
    const auto lines = linesSince(before);

    REQUIRE(lines.size() == 1);

    nlohmann::json event;
    REQUIRE_NOTHROW(event = nlohmann::json::parse(lines[0]));

    REQUIRE(event["timestamp"].is_string());
    REQUIRE(event["level"] == "INFO");
    REQUIRE(event["thread_id"].is_number());
    REQUIRE(event["function"] == "unitTest");
    REQUIRE(event["event"] == "widget_created");
    REQUIRE(event["source"]["file"].is_string());
    REQUIRE(event["source"]["line"].is_number());
}

TEST_CASE("a message containing newlines still yields exactly one NDJSON line", "[structured-log]") {
    const auto before = bufferSize();
    slog::emit(slog::Level::Info, "unitTest", "multiline_message_test", {{"message", "line one\nline two\nline three"}});
    const auto lines = linesSince(before);

    // The embedded newlines must be JSON-escaped inside the single line, not literal
    // line breaks - otherwise a log shipper reading line-by-line would split one event
    // into three malformed fragments.
    REQUIRE(lines.size() == 1);
    nlohmann::json event;
    REQUIRE_NOTHROW(event = nlohmann::json::parse(lines[0]));
    REQUIRE(event["message"] == "line one\nline two\nline three");
}

TEST_CASE("structured fields land at the top level, not only inside message", "[structured-log]") {
    const auto before = bufferSize();
    slog::emit(slog::Level::Info, "unitTest", "match_loaded",
               {{"match_id", "match-abc-123"}, {"message", "Match loaded"}});
    const auto lines = linesSince(before);
    REQUIRE(lines.size() == 1);
    const auto event = nlohmann::json::parse(lines[0]);

    // The id must be its own queryable field...
    REQUIRE(event["match_id"] == "match-abc-123");
    // ...not something a consumer has to grep out of free text.
    REQUIRE(event["message"].get<std::string>().find("match-abc-123") == std::string::npos);
}

TEST_CASE("RequestLog emits one line per lifecycle event, all sharing one request_id", "[structured-log]") {
    const auto before = bufferSize();
    auto req = makeRequest(drogon::Get, "/api/match/match-abc-123");

    RequestLog rlog("getMatch", req);
    rlog.debug("codeset", {{"table", nlohmann::json::object()}, {"logs", nlohmann::json::object()}});
    rlog.completed(200, "match_loaded", "Match loaded", {{"match_id", "match-abc-123"}, {"turn", 3}});

    const auto lines = linesSince(before);
    REQUIRE(lines.size() == 3);

    std::vector<nlohmann::json> events;
    for (const auto& line : lines) {
        nlohmann::json event;
        REQUIRE_NOTHROW(event = nlohmann::json::parse(line));
        events.push_back(event);
    }

    REQUIRE(events[0]["event"] == "http_request_started");
    REQUIRE(events[0]["method"] == "GET");
    REQUIRE(events[0]["path"] == "/api/match/match-abc-123");

    REQUIRE(events[1]["event"] == "codeset");
    REQUIRE(events[1]["level"] == "DEBUG");

    REQUIRE(events[2]["event"] == "match_loaded");
    REQUIRE(events[2]["status"] == 200);
    REQUIRE(events[2]["match_id"] == "match-abc-123");
    REQUIRE(events[2]["turn"] == 3);
    REQUIRE(events[2]["duration_ms"].is_number());

    const auto requestId = events[0]["request_id"].get<std::string>();
    REQUIRE_FALSE(requestId.empty());
    for (const auto& event : events) {
        REQUIRE(event["request_id"] == requestId);
    }
}

TEST_CASE("two different requests get two different request_ids", "[structured-log]") {
    auto reqA = makeRequest(drogon::Get, "/api/match/match-a");
    auto reqB = makeRequest(drogon::Get, "/api/match/match-b");

    const auto before = bufferSize();
    RequestLog rlogA("getMatch", reqA);
    RequestLog rlogB("getMatch", reqB);
    const auto lines = linesSince(before);
    REQUIRE(lines.size() == 2);

    const auto eventA = nlohmann::json::parse(lines[0]);
    const auto eventB = nlohmann::json::parse(lines[1]);
    REQUIRE(eventA["request_id"] != eventB["request_id"]);
}

TEST_CASE("RequestLog::failed exposes structured error.type/message/code, not just prose", "[structured-log]") {
    const auto before = bufferSize();
    auto req = makeRequest(drogon::Post, "/api/match/match-abc-123/join", "{\"account\":\"p1\"}");

    RequestLog rlog("joinMatch", req);
    rlog.failed(404, "Failed to load match", "", {{"match_id", "match-abc-123"}, {"error", slog::codeError(CODE_MATCH_FILE_NOT_FOUND_ON_DISK)}});

    const auto lines = linesSince(before);
    REQUIRE(lines.size() == 2);  // started + failed
    const auto failedEvent = nlohmann::json::parse(lines[1]);

    REQUIRE(failedEvent["event"] == "http_request_failed");
    REQUIRE(failedEvent["level"] == "ERROR");
    REQUIRE(failedEvent["status"] == 404);
    REQUIRE(failedEvent["match_id"] == "match-abc-123");
    REQUIRE(failedEvent["error"]["type"] == code_to_text(CODE_MATCH_FILE_NOT_FOUND_ON_DISK));
    REQUIRE(failedEvent["error"]["code"] == (int)CODE_MATCH_FILE_NOT_FOUND_ON_DISK);
    REQUIRE(failedEvent["error"]["message"] == "Failed to load match");
}

TEST_CASE("failed() without a CodeEnum still gets a machine-readable error.type", "[structured-log]") {
    const auto before = bufferSize();
    auto req = makeRequest(drogon::Post, "/api/match", "{}");

    RequestLog rlog("createMatch", req);
    rlog.failed(400, "Missing host field", "missing_field", {{"field", "host"}});

    const auto lines = linesSince(before);
    REQUIRE(lines.size() == 2);
    const auto failedEvent = nlohmann::json::parse(lines[1]);

    REQUIRE(failedEvent["error"]["type"] == "missing_field");
    REQUIRE(failedEvent["field"] == "host");
}

TEST_CASE("slog::emit respects the configured log level threshold", "[structured-log]") {
    const auto originalLevel = trantor::Logger::logLevel();
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);

    const auto before = bufferSize();
    slog::emit(slog::Level::Debug, "unitTest", "should_be_filtered");
    const auto lines = linesSince(before);
    REQUIRE(lines.empty());

    trantor::Logger::setLogLevel(originalLevel);
}
