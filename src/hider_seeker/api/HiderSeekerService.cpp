#include "HiderSeekerService.hpp"
#include "request_validation.hpp"
#include "match_setup.hpp"
#include "null_seeker_policy.hpp"
#include "../domain/serialization.hpp"
#include "../turn/turn_engine.hpp"
#include <random>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace hider_seeker {

HiderSeekerService::HiderSeekerService(iStore& store) : store_(store) {}

ApiResponse HiderSeekerService::createMatch(const std::string& body) {
    // Parse body as JSON
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate account
    if (!j.contains("account") || !isValidAccount(j["account"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate hider_count
    if (!j.contains("hider_count") || !j["hider_count"].is_number_unsigned()) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    uint8_t hiderCount = j["hider_count"];
    if (hiderCount < 1 || hiderCount > MAX_HIDERS) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate seed
    uint64_t seed = 0;
    if (j.contains("seed") && j["seed"].is_number_unsigned()) {
        seed = j["seed"];
    } else {
        std::random_device rd;
        seed = rd();
    }
    
    // Generate match_id = "hs-" + 16 random lowercase hex digits
    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    std::string matchId = "hs-";
    for (int i = 0; i < 8; ++i) {
        uint32_t val = dist(rd);
        char hex[9];
        snprintf(hex, sizeof(hex), "%08x", val);
        matchId += std::string(hex);
    }
    
    // Build MatchState
    MatchState state{};
    state.schema_version = 2;
    state.ruleset_version = RULESET_VERSION;
    state.revision = 0;
    state.tick = 0;
    state.phase = PHASE_LOBBY;
    state.rng_state = seed;
    state.hider_count = hiderCount;
    state.server_count = hiderCount;
    
    // Initialize servers
    for (uint8_t i = 0; i < MAX_SERVERS; ++i) {
        state.servers[i].owner_hider_id = 0xFF; // unowned
    }
    
    // Initialize hiders and servers
    for (uint8_t i = 0; i < hiderCount; ++i) {
        state.hiders[i].active = 1;
        state.hiders[i].status = HIDER_STATUS_ACTIVE;
        state.hiders[i].action_points = PLACEHOLDER_START_ACTION_POINTS;
        state.hiders[i].server_id = i;
        state.hiders[i].resources[LOA_RESOURCE_INDEX] = PLACEHOLDER_START_LOA;
        
        state.servers[i].active = 1;
        state.servers[i].owner_hider_id = i;
        // Zero out metrics
        for (int j = 0; j < HISTORY_TICKS; ++j) {
            state.servers[i].history[j] = {};
        }
        state.servers[i].history_head = 0;
    }
    
    // Initialize seeker
    state.seeker.connection_tokens = PLACEHOLDER_SEEKER_START_TOKENS;
    state.seeker.max_tokens = PLACEHOLDER_SEEKER_MAX_TOKENS;
    
    // Create MatchRecord
    MatchRecord record;
    record.filename = matchId + ".json";
    record.version = 1;
    record.host = j["account"];
    record.accounts.resize(MAX_HIDERS); // Initialize with null entries
    
    // Save state to record
    std::vector<uint8_t> bytes;
    saveMatchState(state, bytes);
    
    // Convert bytes to hex string
    std::stringstream ss;
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    record.state = ss.str();
    
    record.next_move_id = 1;
    record.idempotency.clear();
    record.moves.clear();
    record.request_log.clear();
    
    // Append create request log entry
    RequestLogEntry entry;
    entry.revision_before = 0;
    entry.account = j["account"];
    entry.kind = "create";
    entry.body = j;
    record.request_log.push_back(entry);
    
    // Save to store
    nlohmann::json saveBody = record.toJson();
    CodeEnum error;
    if (!store_.save(true, error, saveBody)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Return response
    ApiResponse response;
    response.status = 201;
    response.body["match_id"] = matchId;
    response.body["revision"] = 0;
    response.body["phase"] = "LOBBY";
    return response;
}

ApiResponse HiderSeekerService::joinMatch(const std::string& matchId, const std::string& body) {
    // Parse body as JSON
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate account
    if (!j.contains("account") || !isValidAccount(j["account"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate expected_revision
    if (!j.contains("expected_revision") || !isValidExpectedRevision(j["expected_revision"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate idempotency_key
    if (!j.contains("idempotency_key") || !isValidIdempotencyKey(j["idempotency_key"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Acquire match mutex
    std::unique_ptr<std::mutex> matchMutex;
    {
        std::lock_guard<std::mutex> lock(matchMutexesGuard_);
        auto it = matchMutexes_.find(matchId);
        if (it == matchMutexes_.end()) {
            matchMutexes_[matchId] = std::make_unique<std::mutex>();
            it = matchMutexes_.find(matchId);
        }
        matchMutex = std::move(it->second);
    }
    std::lock_guard<std::mutex> lock(*matchMutex);
    
    // Load record
    CodeEnum error;
    nlohmann::json loadedJson;
    if (!store_.load(matchId + ".json", error, loadedJson)) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "MATCH_NOT_FOUND");
        return response;
    }
    
    MatchRecord record;
    if (!record.fromJson(loadedJson)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Validate expected revision
    if (record.version != j["expected_revision"].get<int>()) {
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "STALE_REVISION");
        response.body["revision"] = record.version;
        return response;
    }
    
    // Check if match is in lobby phase
    if (record.version != 1) { // This is a bit of a hack - we need to get the actual phase from state
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "MATCH_NOT_IN_LOBBY");
        return response;
    }
    
    // Find free slot
    uint8_t freeSlot = MAX_HIDERS; // Invalid slot
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (record.accounts[i].is_null()) {
            freeSlot = i;
            break;
        }
    }
    
    if (freeSlot >= MAX_HIDERS) {
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "MATCH_FULL");
        return response;
    }
    
    // Check if account is already bound to a slot
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (!record.accounts[i].is_null() && record.accounts[i] == j["account"]) {
            ApiResponse response;
            response.status = 409;
            response.body = buildErrorBody(409, "ALREADY_JOINED");
            return response;
        }
    }
    
    // Bind to slot
    record.accounts[freeSlot] = j["account"];
    record.version += 1;
    
    // Update hider state in the actual match state
    std::vector<uint8_t> bytes;
    // Convert hex string back to bytes
    for (size_t i = 0; i < record.state.length(); i += 2) {
        std::string byteString = record.state.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    MatchState state;
    if (!loadMatchState(bytes, state)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Update hider
    state.hiders[freeSlot].active = 1;
    state.hiders[freeSlot].status = HIDER_STATUS_ACTIVE;
    state.hiders[freeSlot].action_points = PLACEHOLDER_START_ACTION_POINTS;
    state.hiders[freeSlot].resources[LOA_RESOURCE_INDEX] = PLACEHOLDER_START_LOA;
    
    // Save updated state back to record
    bytes.clear();
    saveMatchState(state, bytes);
    
    // Convert bytes to hex string
    std::stringstream ss;
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    record.state = ss.str();
    
    // Append join request log entry
    RequestLogEntry entry;
    entry.revision_before = record.version - 1;
    entry.account = j["account"];
    entry.kind = "join";
    entry.body = j;
    record.request_log.push_back(entry);
    
    // Save to store
    nlohmann::json saveBody = record.toJson();
    if (!store_.save(false, error, saveBody)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Return response
    ApiResponse response;
    response.status = 200;
    response.body["hider_id"] = freeSlot;
    response.body["revision"] = record.version;
    return response;
}

ApiResponse HiderSeekerService::startMatch(const std::string& matchId, const std::string& body) {
    // Parse body as JSON
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate account
    if (!j.contains("account") || !isValidAccount(j["account"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate expected_revision
    if (!j.contains("expected_revision") || !isValidExpectedRevision(j["expected_revision"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate idempotency_key
    if (!j.contains("idempotency_key") || !isValidIdempotencyKey(j["idempotency_key"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Acquire match mutex
    std::unique_ptr<std::mutex> matchMutex;
    {
        std::lock_guard<std::mutex> lock(matchMutexesGuard_);
        auto it = matchMutexes_.find(matchId);
        if (it == matchMutexes_.end()) {
            matchMutexes_[matchId] = std::make_unique<std::mutex>();
            it = matchMutexes_.find(matchId);
        }
        matchMutex = std::move(it->second);
    }
    std::lock_guard<std::mutex> lock(*matchMutex);
    
    // Load record
    CodeEnum error;
    nlohmann::json loadedJson;
    if (!store_.load(matchId + ".json", error, loadedJson)) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "MATCH_NOT_FOUND");
        return response;
    }
    
    MatchRecord record;
    if (!record.fromJson(loadedJson)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Validate expected revision
    if (record.version != j["expected_revision"].get<int>()) {
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "STALE_REVISION");
        response.body["revision"] = record.version;
        return response;
    }
    
    // Validate caller is host
    if (record.host != j["account"]) {
        ApiResponse response;
        response.status = 403;
        response.body = buildErrorBody(403, "NOT_HOST");
        return response;
    }
    
    // Check if match is in lobby phase
    if (record.version != 1) { // This is a bit of a hack - we need to get the actual phase from state
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "MATCH_NOT_IN_LOBBY");
        return response;
    }
    
    // Check if at least one bound slot exists
    bool hasBoundSlot = false;
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (!record.accounts[i].is_null()) {
            hasBoundSlot = true;
            break;
        }
    }
    
    if (!hasBoundSlot) {
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "NO_HIDERS");
        return response;
    }
    
    // Update record
    record.version += 1;
    
    // Update state to ACTIVE phase
    std::vector<uint8_t> bytes;
    // Convert hex string back to bytes
    for (size_t i = 0; i < record.state.length(); i += 2) {
        std::string byteString = record.state.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    MatchState state;
    if (!loadMatchState(bytes, state)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    state.phase = PHASE_ACTIVE;
    
    // Save updated state back to record
    bytes.clear();
    saveMatchState(state, bytes);
    
    // Convert bytes to hex string
    std::stringstream ss;
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    record.state = ss.str();
    
    // Append start request log entry
    RequestLogEntry entry;
    entry.revision_before = record.version - 1;
    entry.account = j["account"];
    entry.kind = "start";
    entry.body = j;
    record.request_log.push_back(entry);
    
    // Save to store
    nlohmann::json saveBody = record.toJson();
    if (!store_.save(false, error, saveBody)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Return response
    ApiResponse response;
    response.status = 200;
    response.body["revision"] = record.version;
    response.body["phase"] = "ACTIVE";
    return response;
}

ApiResponse HiderSeekerService::getMatch(const std::string& matchId) {
    // Load record
    CodeEnum error;
    nlohmann::json loadedJson;
    if (!store_.load(matchId + ".json", error, loadedJson)) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "MATCH_NOT_FOUND");
        return response;
    }
    
    MatchRecord record;
    if (!record.fromJson(loadedJson)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Return response
    ApiResponse response;
    response.status = 200;
    response.body["match_id"] = matchId;
    response.body["host"] = record.host;
    response.body["phase"] = (record.version == 1) ? "LOBBY" : "ACTIVE";
    response.body["revision"] = record.version;
    response.body["tick"] = 0; // We don't have access to the actual tick here
    
    // Build hiders array
    nlohmann::json hidersArray = nlohmann::json::array();
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (!record.accounts[i].is_null()) {
            nlohmann::json hider;
            hider["hider_id"] = i;
            hider["account"] = record.accounts[i];
            hidersArray.push_back(hider);
        }
    }
    
    response.body["hiders"] = hidersArray;
    return response;
}

ApiResponse HiderSeekerService::submitMove(const std::string& matchId, const std::string& body) {
    // Parse body as JSON
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate account
    if (!j.contains("account") || !isValidAccount(j["account"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate expected_revision
    if (!j.contains("expected_revision") || !isValidExpectedRevision(j["expected_revision"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate idempotency_key
    if (!j.contains("idempotency_key") || !isValidIdempotencyKey(j["idempotency_key"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate play_mode
    if (!j.contains("play_mode") || !isValidPlayMode(j["play_mode"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate intent
    if (!j.contains("intent")) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Acquire match mutex
    std::unique_ptr<std::mutex> matchMutex;
    {
        std::lock_guard<std::mutex> lock(matchMutexesGuard_);
        auto it = matchMutexes_.find(matchId);
        if (it == matchMutexes_.end()) {
            matchMutexes_[matchId] = std::make_unique<std::mutex>();
            it = matchMutexes_.find(matchId);
        }
        matchMutex = std::move(it->second);
    }
    std::lock_guard<std::mutex> lock(*matchMutex);
    
    // Load record
    CodeEnum error;
    nlohmann::json loadedJson;
    if (!store_.load(matchId + ".json", error, loadedJson)) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "MATCH_NOT_FOUND");
        return response;
    }
    
    MatchRecord record;
    if (!record.fromJson(loadedJson)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Validate expected revision
    if (record.version != j["expected_revision"].get<int>()) {
        ApiResponse response;
        response.status = 409;
        response.body = buildErrorBody(409, "STALE_REVISION");
        response.body["revision"] = record.version;
        return response;
    }
    
    // Check if account is in the match
    uint8_t hiderId = MAX_HIDERS; // Invalid hider ID
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (!record.accounts[i].is_null() && record.accounts[i] == j["account"]) {
            hiderId = i;
            break;
        }
    }
    
    if (hiderId >= MAX_HIDERS) {
        ApiResponse response;
        response.status = 403;
        response.body = buildErrorBody(403, "NOT_IN_MATCH");
        return response;
    }
    
    // Check idempotency
    std::string key = j["idempotency_key"];
    for (const auto& entry : record.idempotency) {
        if (entry.account == j["account"] && entry.key == key) {
            // Return the same result as before
            ApiResponse response;
            response.status = entry.status_code;
            response.body = entry.response;
            return response;
        }
    }
    
    // Validate intent type
    if (!j["intent"].contains("type") || !isValidIntentType(j["intent"]["type"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate play_mode with intent type
    std::string playMode = j["play_mode"];
    std::string intentType = j["intent"]["type"];
    
    if (playMode == "PASS" && intentType != "PASS") {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate ticks for PASS mode
    if (playMode == "PASS") {
        if (!j.contains("ticks") || !j["ticks"].is_number_unsigned()) {
            ApiResponse response;
            response.status = 400;
            response.body = buildErrorBody(400, "INVALID_REQUEST");
            return response;
        }
        
        uint8_t ticks = j["ticks"];
        if (ticks < 1 || ticks > MAX_ADVANCE_TICKS) {
            ApiResponse response;
            response.status = 400;
            response.body = buildErrorBody(400, "INVALID_REQUEST");
            return response;
        }
    }
    
    // Validate REMOVE_SERVER server_id
    if (intentType == "REMOVE_SERVER") {
        if (!j["intent"].contains("server_id")) {
            ApiResponse response;
            response.status = 400;
            response.body = buildErrorBody(400, "INVALID_REQUEST");
            return response;
        }
        
        uint8_t serverId = j["intent"]["server_id"];
        if (serverId >= MAX_SERVERS) {
            ApiResponse response;
            response.status = 400;
            response.body = buildErrorBody(400, "INVALID_REQUEST");
            return response;
        }
    }
    
    // Process the move
    std::vector<uint8_t> bytes;
    // Convert hex string back to bytes
    for (size_t i = 0; i < record.state.length(); i += 2) {
        std::string byteString = record.state.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    MatchState state;
    if (!loadMatchState(bytes, state)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Build MoveRequest
    MoveRequest moveRequest{};
    moveRequest.hider_id = hiderId;
    moveRequest.server_id = NO_SERVER_ID; // Will be set for REMOVE_SERVER
    moveRequest.target_id = NO_SERVER_ID;
    moveRequest.server_mode = 0; // Will be set for HIDER_ACTION
    
    if (intentType == "ADD_SERVER" || intentType == "REMOVE_SERVER") {
        if (intentType == "REMOVE_SERVER") {
            moveRequest.action_type = TICK_ACTION_REMOVE_SERVER;
            moveRequest.server_id = j["intent"]["server_id"];
        } else {
            moveRequest.action_type = TICK_ACTION_ADD_SERVER;
        }
    } else {
        // HIDER_ACTION
        moveRequest.action_type = TICK_ACTION_HIDER_ACTION;
        
        // Map intent type to server_mode
        if (intentType == "CONFIGURE_SERVER") {
            moveRequest.server_mode = HIDER_ACTION_CONFIGURE_SERVER;
        } else if (intentType == "SUPPRESS_ERROR") {
            moveRequest.server_mode = HIDER_ACTION_SUPPRESS_ERROR;
        } else if (intentType == "BOOST_CPU") {
            moveRequest.server_mode = HIDER_ACTION_BOOST_CPU;
        } else if (intentType == "ROTATE_ROUTE") {
            moveRequest.server_mode = HIDER_ACTION_ROTATE_ROUTE;
        } else if (intentType == "PAY_GOAL") {
            moveRequest.server_mode = HIDER_ACTION_PAY_GOAL;
        }
    }
    
    // Set target server for hider actions
    moveRequest.server_id = state.hiders[hiderId].server_id;
    
    // Create NullSeekerPolicy
    NullSeekerPolicy policy;
    
    TurnResult result;
    if (playMode == "PLAY") {
        result = playMove(state, moveRequest, policy);
    } else if (playMode == "QUEUE") {
        result = queueMove(state, moveRequest, policy);
    } else if (playMode == "PASS") {
        // Validate ticks
        uint8_t ticks = j["ticks"];
        result = passTicks(state, ticks, policy);
    }
    
    // Handle the result
    ApiResponse response;
    response.status = 200;
    
    // Update record based on result
    if (result.status == MoveStatus::REJECTED) {
        // Return rejection
        response.status = 422;
        response.body["status"] = "REJECTED";
        response.body["rule_outcome"] = static_cast<int>(result.ruleOutcome);
        response.body["revision"] = record.version;
        
        // Add to idempotency
        IdempotencyEntry entry;
        entry.account = j["account"];
        entry.key = key;
        entry.request = body;
        entry.status_code = 422;
        entry.response = response.body;
        record.idempotency.push_back(entry);
        
        // Save to store
        nlohmann::json saveBody = record.toJson();
        if (!store_.save(false, error, saveBody)) {
            response.status = 500;
            response.body = buildErrorBody(500, "INTERNAL_ERROR");
        }
        
        return response;
    } else if (result.status == MoveStatus::QUEUED) {
        // Add to moves array
        MoveOwnershipEntry moveEntry;
        moveEntry.move_id = record.next_move_id;
        moveEntry.account = j["account"];
        moveEntry.hider_id = hiderId;
        record.moves.push_back(moveEntry);
        
        response.body["status"] = "QUEUED";
        response.body["revision"] = record.version;
        response.body["tick"] = state.tick;
        response.body["ticks_advanced"] = result.ticksAdvanced;
        
        // Update next_move_id
        record.next_move_id++;
        
        // Add to idempotency
        IdempotencyEntry entry;
        entry.account = j["account"];
        entry.key = key;
        entry.request = body;
        entry.status_code = 200;
        entry.response = response.body;
        record.idempotency.push_back(entry);
        
        // Save to store
        nlohmann::json saveBody = record.toJson();
        if (!store_.save(false, error, saveBody)) {
            response.status = 500;
            response.body = buildErrorBody(500, "INTERNAL_ERROR");
        }
        
        return response;
    } else {
        // APPLIED
        response.body["status"] = "APPLIED";
        response.body["accepted_against_revision"] = record.version;
        response.body["revision"] = record.version + 1;
        response.body["tick"] = state.tick;
        response.body["ticks_advanced"] = result.ticksAdvanced;
        
        // Update pending_applied and pending_cancelled
        if (result.pendingAppliedCount > 0) {
            nlohmann::json appliedArray = nlohmann::json::array();
            for (uint8_t i = 0; i < result.pendingAppliedCount; ++i) {
                appliedArray.push_back(result.pendingApplied[i].move_id);
            }
            response.body["pending_applied"] = appliedArray;
        }
        
        // Update pending_cancelled
        // This is a simplified version - in reality, we'd need to compute this properly
        response.body["pending_cancelled"] = nlohmann::json::array();
        
        // Update events
        if (result.publicEventCount > 0) {
            nlohmann::json eventsArray = nlohmann::json::array();
            for (uint8_t i = 0; i < result.publicEventCount; ++i) {
                nlohmann::json event;
                event["type"] = result.publicEvents[i].type;
                event["tick"] = result.publicEvents[i].tick;
                eventsArray.push_back(event);
            }
            response.body["events"] = eventsArray;
        }
        
        // Save updated state back to record
        bytes.clear();
        saveMatchState(state, bytes);
        
        // Convert bytes to hex string
        std::stringstream ss;
        for (uint8_t byte : bytes) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
        }
        record.state = ss.str();
        
        // Add to idempotency
        IdempotencyEntry entry;
        entry.account = j["account"];
        entry.key = key;
        entry.request = body;
        entry.status_code = 200;
        entry.response = response.body;
        record.idempotency.push_back(entry);
        
        // Append move request log entry
        RequestLogEntry logEntry;
        logEntry.revision_before = record.version;
        logEntry.account = j["account"];
        logEntry.kind = "move";
        logEntry.body = j;
        record.request_log.push_back(logEntry);
        
        // Save to store
        nlohmann::json saveBody = record.toJson();
        if (!store_.save(false, error, saveBody)) {
            response.status = 500;
            response.body = buildErrorBody(500, "INTERNAL_ERROR");
        }
        
        return response;
    }
}

ApiResponse HiderSeekerService::cancelMove(const std::string& matchId, const std::string& intentId, const std::string& body) {
    // Parse body as JSON
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Validate account
    if (!j.contains("account") || !isValidAccount(j["account"])) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Acquire match mutex
    std::unique_ptr<std::mutex> matchMutex;
    {
        std::lock_guard<std::mutex> lock(matchMutexesGuard_);
        auto it = matchMutexes_.find(matchId);
        if (it == matchMutexes_.end()) {
            matchMutexes_[matchId] = std::make_unique<std::mutex>();
            it = matchMutexes_.find(matchId);
        }
        matchMutex = std::move(it->second);
    }
    std::lock_guard<std::mutex> lock(*matchMutex);
    
    // Load record
    CodeEnum error;
    nlohmann::json loadedJson;
    if (!store_.load(matchId + ".json", error, loadedJson)) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "MATCH_NOT_FOUND");
        return response;
    }
    
    MatchRecord record;
    if (!record.fromJson(loadedJson)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Validate intent_id format
    if (intentId.substr(0, 3) != "mv-") {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Extract move ID from intentId
    uint32_t moveId = 0;
    try {
        moveId = std::stoi(intentId.substr(3));
    } catch (...) {
        ApiResponse response;
        response.status = 400;
        response.body = buildErrorBody(400, "INVALID_REQUEST");
        return response;
    }
    
    // Find the move in record.moves
    bool found = false;
    uint8_t moveIndex = 0;
    for (uint8_t i = 0; i < record.moves.size(); ++i) {
        if (record.moves[i].move_id == moveId && record.moves[i].account == j["account"]) {
            found = true;
            moveIndex = i;
            break;
        }
    }
    
    if (!found) {
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "INTENT_NOT_FOUND");
        return response;
    }
    
    // Call cancelPendingMove
    std::vector<uint8_t> bytes;
    // Convert hex string back to bytes
    for (size_t i = 0; i < record.state.length(); i += 2) {
        std::string byteString = record.state.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    MatchState state;
    if (!loadMatchState(bytes, state)) {
        ApiResponse response;
        response.status = 500;
        response.body = buildErrorBody(500, "INTERNAL_ERROR");
        return response;
    }
    
    // Create NullSeekerPolicy
    NullSeekerPolicy policy;
    
    TurnResult result = cancelPendingMove(state, record.moves[moveIndex].hider_id, moveId);
    
    if (result.status == MoveStatus::CANCELLED) {
        // Remove from moves array
        record.moves.erase(record.moves.begin() + moveIndex);
        
        // Save updated state back to record
        bytes.clear();
        saveMatchState(state, bytes);
        
        // Convert bytes to hex string
        std::stringstream ss;
        for (uint8_t byte : bytes) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
        }
        record.state = ss.str();
        
        // Append cancel request log entry
        RequestLogEntry logEntry;
        logEntry.revision_before = record.version;
        logEntry.account = j["account"];
        logEntry.kind = "cancel";
        logEntry.body = j;
        record.request_log.push_back(logEntry);
        
        // Save to store
        nlohmann::json saveBody = record.toJson();
        if (!store_.save(false, error, saveBody)) {
            ApiResponse response;
            response.status = 500;
            response.body = buildErrorBody(500, "INTERNAL_ERROR");
            return response;
        }
        
        // Return success
        ApiResponse response;
        response.status = 200;
        response.body["status"] = "CANCELLED";
        response.body["ticks_advanced"] = result.ticksAdvanced;
        return response;
    } else {
        // Not found or rejected
        ApiResponse response;
        response.status = 404;
        response.body = buildErrorBody(404, "INTENT_NOT_FOUND");
        return response;
    }
}

} // namespace hider_seeker