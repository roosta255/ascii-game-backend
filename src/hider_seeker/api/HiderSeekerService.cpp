#include "HiderSeekerService.hpp"
#include "match_record.hpp"
#include "match_setup.hpp"
#include "null_seeker_policy.hpp"
#include "request_validation.hpp"
#include "../domain/state.hpp"
#include "../ruleset/match_end.hpp"
#include "view_snapshots.hpp"
#include "../projection/audience_views.hpp"
#include "../projection/names.hpp"
#include "../projection/public_event.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/lifecycle_events.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../tick/tick_types.hpp"
#include "../turn/turn_engine.hpp"
#include "../turn/turn_types.hpp"
#include <algorithm>
#include <random>
#include <vector>

namespace hider_seeker {

namespace {

using nlohmann::json;

// How many fresh match ids createMatch tries before giving up on collisions.
constexpr int MATCH_ID_ATTEMPTS = 8;

ApiResponse errorResponse(int status, const std::string& code, const std::string& message) {
    return ApiResponse{status, buildErrorBody(code, message)};
}

ApiResponse invalidRequest(const std::string& message) {
    return errorResponse(400, "INVALID_REQUEST", message);
}

// Parses body into out; true only when it is a JSON object.
bool parseObject(const std::string& body, json& out) {
    try {
        out = json::parse(body);
    } catch (const json::exception&) {
        return false;
    }
    return out.is_object();
}

bool readString(const json& object, const char* field, std::string& out) {
    const auto it = object.find(field);
    if (it == object.end() || !it->is_string()) {
        return false;
    }
    out = it->get<std::string>();
    return true;
}

// Reads an integer field into out when it lies in [minValue, maxValue].
bool readIntInRange(const json& object, const char* field, int64_t minValue, int64_t maxValue, int64_t& out) {
    const auto it = object.find(field);
    if (it == object.end() || !it->is_number_integer()) {
        return false;
    }
    if (it->is_number_unsigned()) {
        const uint64_t value = it->get<uint64_t>();
        if (value > static_cast<uint64_t>(maxValue)) {
            return false;
        }
        out = static_cast<int64_t>(value);
    } else {
        out = it->get<int64_t>();
    }
    return out >= minValue && out <= maxValue;
}

std::string formatMoveId(uint32_t moveNumber) {
    return "mv-" + std::to_string(moveNumber);
}

// Accepts exactly "mv-<n>" with n a positive uint32 written without leading zeros.
bool parseMoveId(const std::string& intentId, uint32_t& out) {
    const std::string prefix = "mv-";
    if (intentId.size() <= prefix.size() || intentId.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string digits = intentId.substr(prefix.size());
    if (digits.size() > 10 || digits[0] == '0') {
        return false;
    }
    uint64_t value = 0;
    for (const char c : digits) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + static_cast<uint64_t>(c - '0');
    }
    if (value == 0 || value > UINT32_MAX) {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

std::string randomHex16(std::random_device& device) {
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    const uint64_t value = (static_cast<uint64_t>(device()) << 32) | static_cast<uint64_t>(device());
    std::string hex(16, '0');
    for (int nibble = 0; nibble < 16; ++nibble) {
        hex[15 - nibble] = HEX_DIGITS[(value >> (nibble * 4)) & 0x0F];
    }
    return hex;
}

const char* moveStatusName(MoveStatus status) {
    switch (status) {
        case MoveStatus::APPLIED:   return "APPLIED";
        case MoveStatus::QUEUED:    return "QUEUED";
        case MoveStatus::REJECTED:  return "REJECTED";
        case MoveStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

const char* turnReasonName(TurnReason reason) {
    switch (reason) {
        case TurnReason::NONE:               return "NONE";
        case TurnReason::NEVER_AFFORDABLE:   return "NEVER_AFFORDABLE";
        case TurnReason::QUEUE_FULL:         return "QUEUE_FULL";
        case TurnReason::HIDER_QUEUE_LIMIT:  return "HIDER_QUEUE_LIMIT";
        case TurnReason::DUPLICATE_MOVE_ID:  return "DUPLICATE_MOVE_ID";
        case TurnReason::RULE_REJECTED:      return "RULE_REJECTED";
        case TurnReason::HIDER_CAPTURED:     return "HIDER_CAPTURED";
        case TurnReason::MATCH_NOT_ACTIVE:   return "MATCH_NOT_ACTIVE";
        case TurnReason::ADVANCE_LIMIT:      return "ADVANCE_LIMIT";
        case TurnReason::NOT_FOUND:          return "NOT_FOUND";
        case TurnReason::INVALID_TICK_COUNT: return "INVALID_TICK_COUNT";
    }
    return "UNKNOWN";
}

const char* ruleOutcomeName(RuleOutcome outcome) {
    switch (outcome) {
        case RuleOutcome::ACCEPTED:                       return "ACCEPTED";
        case RuleOutcome::REJECTED_NO_SLOT_AVAILABLE:     return "REJECTED_NO_SLOT_AVAILABLE";
        case RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE: return "REJECTED_INSUFFICIENT_RESOURCE";
        case RuleOutcome::REJECTED_RELOCATION_FAILED:     return "REJECTED_RELOCATION_FAILED";
        case RuleOutcome::REJECTED_INVALID_TARGET:        return "REJECTED_INVALID_TARGET";
        case RuleOutcome::REJECTED_NO_CONNECTIONS:        return "REJECTED_NO_CONNECTIONS";
    }
    return "UNKNOWN";
}

// RECORD-002: appends every public event of a committed turn result, in
// order, stamped with the post-commit revision. Each entry keeps the tick the
// ruleset recorded for that event, not the final state tick.
void appendTurnPublicEvents(MatchRecord& record, const TurnResult& result) {
    for (int i = 0; i < result.publicEventCount; ++i) {
        result.publicEvents.accessConst(i, [&record](const PublicLifecycleEvent& event) {
            if (event.type == HiderSeekerEventType::SERVER_PROBED) {
                appendPublicEvent(record, PublicEventEntry{eventTypeName(event.type), event.tick,
                                                           record.state.revision, event.server_id,
                                                           probeOutcomeName(event.outcome)});
            } else {
                appendPublicEvent(record, PublicEventEntry{eventTypeName(event.type), event.tick,
                                                           record.state.revision});
            }
        });
    }
}

// The API-level rule_outcome of a turn result. A REJECTED/CANCELLED result
// names its turn reason (e.g. NEVER_AFFORDABLE) unless the reason is a rule
// rejection, in which case the rule's own outcome is more specific.
std::string ruleOutcomeField(const TurnResult& result) {
    const bool failed = result.status == MoveStatus::REJECTED || result.status == MoveStatus::CANCELLED;
    if (failed && result.reason != TurnReason::RULE_REJECTED && result.reason != TurnReason::NONE) {
        return turnReasonName(result.reason);
    }
    return ruleOutcomeName(result.ruleOutcome);
}

// Maps an intent.type to its HiderActionMode (ADD_SERVER/REMOVE_SERVER map to
// their own modes, which only label the intent - their kind is what dispatches).
HiderActionMode intentMode(const std::string& intentType) {
    HiderActionMode mode;
    if (hiderActionFromName(intentType.c_str(), mode)) {
        return mode;
    }
    // Default fallback for invalid types
    return HiderActionMode::PAY_GOAL;
}

// Maps an intent.type to its TickActionKind.
TickActionKind intentKind(const std::string& intentType) {
    if (intentType == "ADD_SERVER")    return TickActionKind::ADD_SERVER;
    if (intentType == "REMOVE_SERVER") return TickActionKind::REMOVE_SERVER;
    if (intentType == "CONNECT")       return TickActionKind::CONNECT;
    if (intentType == "DISCONNECT")    return TickActionKind::DISCONNECT;
    return TickActionKind::HIDER_ACTION;
}

// move_ids of state's pending ring, walked from pending_head in submit order.
std::vector<uint32_t> pendingMoveIds(const MatchState& state) {
    std::vector<uint32_t> ids;
    for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
        ids.push_back(state.pending[(state.pending_head + offset) % MAX_PENDING_MOVES].move_id);
    }
    return ids;
}

bool containsId(const std::vector<uint32_t>& ids, uint32_t id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// Hider slot bound to account, or -1 when the account is not in the match.
int findAccountSlot(const MatchRecord& record, const std::string& account) {
    int slot = 0;
    for (const auto& bound : record.accounts) {
        if (bound == account) {
            return slot;
        }
        ++slot;
    }
    return -1;
}

// Drops ownership entries whose move is no longer pending in the state.
void pruneMoves(MatchRecord& record) {
    const std::vector<uint32_t> pending = pendingMoveIds(record.state);
    record.moves.erase(std::remove_if(record.moves.begin(), record.moves.end(),
                                      [&pending](const MoveOwnershipEntry& entry) {
                                          uint32_t moveNumber = 0;
                                          return !parseMoveId(entry.move_id, moveNumber) ||
                                                 !containsId(pending, moveNumber);
                                      }),
                       record.moves.end());
}

// Builds the shared move/cancel success body. pending_cancelled holds every
// move pending before the call that is neither pending afterward nor applied.
json turnBody(const MatchRecord& record, const TurnResult& result, uint64_t acceptedAgainstRevision,
              const std::vector<uint32_t>& pendingBefore) {
    const std::vector<uint32_t> pendingAfter = pendingMoveIds(record.state);

    std::vector<uint32_t> appliedIds;
    json pendingApplied = json::array();
    for (int i = 0; i < result.pendingAppliedCount; ++i) {
        result.pendingApplied.accessConst(i, [&](const PendingMoveApplied& applied) {
            appliedIds.push_back(applied.move_id);
            pendingApplied.push_back(formatMoveId(applied.move_id));
        });
    }

    json pendingCancelled = json::array();
    for (const uint32_t id : pendingBefore) {
        if (!containsId(pendingAfter, id) && !containsId(appliedIds, id)) {
            pendingCancelled.push_back(formatMoveId(id));
        }
    }

    json events = json::array();
    for (int i = 0; i < result.publicEventCount; ++i) {
        result.publicEvents.accessConst(i, [&events](const PublicLifecycleEvent& event) {
            events.push_back({{"type", eventTypeName(event.type)}, {"tick", event.tick}});
        });
    }

    return {
        {"status", moveStatusName(result.status)},
        {"rule_outcome", ruleOutcomeField(result)},
        {"accepted_against_revision", acceptedAgainstRevision},
        {"revision", record.state.revision},
        {"tick", record.state.tick},
        {"ticks_advanced", result.ticksAdvanced},
        {"pending_applied", pendingApplied},
        {"pending_cancelled", pendingCancelled},
        {"events", events}
    };
}

bool loadRecord(iStore& store, const std::string& matchId, MatchRecord& record, ApiResponse& failure) {
    CodeEnum error = CODE_SUCCESS;
    json root;
    if (!store.load(matchId, error, root)) {
        failure = error == CODE_FILE_NOT_FOUND_ON_DISK
                ? errorResponse(404, "MATCH_NOT_FOUND", "No match with id " + matchId)
                : errorResponse(500, "STORE_ERROR", "Failed to load match " + matchId);
        return false;
    }
    if (!record.fromJson(root)) {
        failure = errorResponse(500, "STORE_ERROR", "Stored match " + matchId + " is unreadable");
        return false;
    }
    return true;
}

bool saveRecord(iStore& store, const MatchRecord& record, bool isInitial, ApiResponse& failure) {
    CodeEnum error = CODE_SUCCESS;
    if (!store.save(isInitial, error, record.toJson())) {
        failure = errorResponse(500, "STORE_ERROR", "Failed to save match " + record.filename);
        return false;
    }
    return true;
}

// Parses body and validates the account and match id every per-match
// endpoint shares, before the match's mutex is taken.
bool parseAccountRequest(const std::string& body, const std::string& matchId, json& request,
                         std::string& account, ApiResponse& failure) {
    if (!parseObject(body, request)) {
        failure = invalidRequest("Request body must be a JSON object");
        return false;
    }
    if (!readString(request, "account", account) || !isValidAccount(account)) {
        failure = invalidRequest("Missing or invalid account");
        return false;
    }
    if (!isValidMatchId(matchId)) {
        failure = invalidRequest("Invalid match id");
        return false;
    }
    return true;
}

// What an endpoint's state change produced.
struct GuardedOutcome {
    ApiResponse response;
    // Store this response under the idempotency key and persist the record.
    // False for errors that leave the match untouched.
    bool recordable = false;
    // Append a request_log entry (state actually changed).
    bool logged = false;
    json logBody;
};

GuardedOutcome unrecorded(ApiResponse response) {
    GuardedOutcome outcome;
    outcome.response = std::move(response);
    return outcome;
}

GuardedOutcome recordedOnly(ApiResponse response) {
    GuardedOutcome outcome = unrecorded(std::move(response));
    outcome.recordable = true;
    return outcome;
}

GuardedOutcome logged(ApiResponse response, json logBody) {
    GuardedOutcome outcome = recordedOnly(std::move(response));
    outcome.logged = true;
    outcome.logBody = std::move(logBody);
    return outcome;
}

// The shared load / idempotency / expected_revision / change / save sequence
// of join, start, move, and cancel. The caller holds the match's mutex.
// action may only modify the record when it returns a recordable outcome.
template <typename Action>
ApiResponse runGuarded(iStore& store, const std::string& matchId, const json& request, const std::string& account,
                       const std::string& kind, const std::string& target, Action action) {
    MatchRecord record;
    ApiResponse failure;
    if (!loadRecord(store, matchId, record, failure)) {
        return failure;
    }

    std::string key;
    if (!readString(request, "idempotency_key", key) || !isValidIdempotencyKey(key)) {
        return invalidRequest("Missing or invalid idempotency_key");
    }
    const auto expected = request.find("expected_revision");
    if (expected == request.end() || !isValidExpectedRevision(*expected)) {
        return invalidRequest("Missing or invalid expected_revision");
    }

    // MOVE-006: idempotency runs before the revision check, so a replayed
    // request carrying its original (now stale) expected_revision still replays.
    const std::string requestText = kind + " " + target + " " + request.dump();
    for (const auto& entry : record.idempotency) {
        if (entry.account == account && entry.key == key) {
            if (entry.request == requestText) {
                return ApiResponse{entry.status_code, entry.response};
            }
            return errorResponse(422, "IDEMPOTENCY_KEY_REUSED",
                                 "idempotency_key was already used with a different request");
        }
    }

    // MOVE-005: optimistic concurrency.
    const uint64_t revisionBefore = record.state.revision;
    if (expected->get<uint64_t>() != revisionBefore) {
        ApiResponse stale = errorResponse(409, "STALE_REVISION", "expected_revision does not match the match revision");
        stale.body["revision"] = revisionBefore;
        return stale;
    }

    GuardedOutcome outcome = action(record, revisionBefore);
    if (!outcome.recordable) {
        return outcome.response;
    }

    record.idempotency.push_back(
        IdempotencyEntry{account, key, requestText, outcome.response.status, outcome.response.body});
    if (outcome.logged) {
        record.request_log.push_back(RequestLogEntry{revisionBefore, account, kind, outcome.logBody});
    }
    if (!saveRecord(store, record, false, failure)) {
        return failure;
    }
    // SNAPSHOT-002: snapshot every committed revision while the caller still
    // holds the match mutex. A failed snapshot write never changes the
    // response of the already-committed request.
    if (record.state.revision != revisionBefore) {
        publishSnapshots(store, record);
    }
    return outcome.response;
}

const char* const LATEST_CACHE_CONTROL = "no-cache";
const char* const REVISION_CACHE_CONTROL = "private, max-age=31536000, immutable";

bool isValidAudience(const std::string& audience) {
    return audience == "hider" || audience == "public";
}

// Accepts 1-20 ASCII digits. outOfRange is set when the digits exceed uint64
// (a revision that can never have been stored).
bool parseRevision(const std::string& text, uint64_t& out, bool& outOfRange) {
    if (text.empty() || text.size() > 20) {
        return false;
    }
    uint64_t value = 0;
    outOfRange = false;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        const uint64_t digit = static_cast<uint64_t>(c - '0');
        if (value > (UINT64_MAX - digit) / 10) {
            outOfRange = true;
        }
        value = value * 10 + digit;
    }
    out = value;
    return true;
}

// Validates the view request steps shared by both view endpoints after the
// audience/revision checks: account (hider only), match lookup, and slot
// binding. On success audienceKey is "public" or "hider-<slot>".
bool resolveViewRequest(iStore& store, const std::string& matchId, const std::string& audience,
                        const std::string& account, MatchRecord& record, int& slot, std::string& audienceKey,
                        ApiResponse& failure) {
    const bool isHider = audience == "hider";
    if (isHider && !isValidAccount(account)) {
        failure = invalidRequest("Missing or invalid account");
        return false;
    }
    if (!loadRecord(store, matchId, record, failure)) {
        return false;
    }
    slot = -1;
    audienceKey = "public";
    if (isHider) {
        slot = findAccountSlot(record, account);
        if (slot < 0 || slot >= record.state.hider_count) {
            failure = errorResponse(403, "NOT_IN_MATCH", "This account has not joined the match");
            return false;
        }
        audienceKey = "hider-" + std::to_string(slot);
    }
    return true;
}

// 200 with view, or 304 with a null body when ifNoneMatch equals the ETag.
ApiResponse viewResponse(const json& view, const std::string& cacheControl, const std::string& ifNoneMatch) {
    const auto hash = view.find("content_hash");
    const std::string etag = "\"" + (hash != view.end() && hash->is_string() ? hash->get<std::string>() : "") + "\"";
    std::vector<std::pair<std::string, std::string>> headers = {{"ETag", etag}, {"Cache-Control", cacheControl}};
    if (ifNoneMatch == etag) {
        return ApiResponse{304, json(nullptr), headers};
    }
    return ApiResponse{200, view, headers};
}

} // namespace

HiderSeekerService::HiderSeekerService(iStore& store) : store_(store) {}

std::mutex& HiderSeekerService::mutexFor(const std::string& matchId) {
    std::lock_guard<std::mutex> guard(matchMutexesGuard_);
    auto& slot = matchMutexes_[matchId];
    if (!slot) {
        slot = std::make_unique<std::mutex>();
    }
    return *slot;
}

ApiResponse HiderSeekerService::createMatch(const std::string& body) {
    json request;
    if (!parseObject(body, request)) {
        return invalidRequest("Request body must be a JSON object");
    }
    std::string account;
    if (!readString(request, "account", account) || !isValidAccount(account)) {
        return invalidRequest("Missing or invalid account");
    }
    int64_t hiderCount = 0;
    if (!readIntInRange(request, "hider_count", 1, MAX_HIDERS, hiderCount)) {
        return invalidRequest("hider_count must be an integer from 1 to 8");
    }

    std::random_device device;
    uint64_t seed = 0;
    const auto seedField = request.find("seed");
    if (seedField != request.end()) {
        if (!seedField->is_number_integer() ||
            (!seedField->is_number_unsigned() && seedField->get<int64_t>() < 0)) {
            return invalidRequest("seed must be a non-negative 64-bit integer");
        }
        seed = seedField->get<uint64_t>();
    } else {
        seed = (static_cast<uint64_t>(device()) << 32) | static_cast<uint64_t>(device());
    }

    MatchRecord record;
    record.host = account;
    record.version = 0;
    record.next_move_id = 1;

    MatchState& state = record.state;
    state.schema_version = 2;
    state.ruleset_version = RULESET_VERSION;
    state.revision = 0;
    state.tick = 0;
    state.phase = PHASE_LOBBY;
    state.rng_state = seed;
    state.hider_count = static_cast<uint8_t>(hiderCount);
    state.server_count = static_cast<uint8_t>(hiderCount);
    for (uint8_t slot = 0; slot < MAX_SERVERS; ++slot) {
        ServerState& server = state.servers[slot];
        server = ServerState{};
        server.active = slot < hiderCount ? 1 : 0;
        server.owner_hider_id = slot < hiderCount ? slot : 0xFF;
    }
    for (uint8_t slot = 0; slot < MAX_HIDERS; ++slot) {
        HiderState& hider = state.hiders[slot];
        hider = HiderState{};
        hider.active = 0; // not joined yet
        hider.server_id = slot < hiderCount ? slot : NO_SERVER_ID;
    }
    state.seeker.connection_tokens = PLACEHOLDER_SEEKER_START_CONNECTIONS;
    state.seeker.max_tokens = PLACEHOLDER_SEEKER_START_CONNECTIONS;

    record.request_log.push_back(RequestLogEntry{0, account, "create", request});

    ApiResponse failure = errorResponse(500, "STORE_ERROR", "Failed to allocate a match id");
    for (int attempt = 0; attempt < MATCH_ID_ATTEMPTS; ++attempt) {
        const std::string matchId = "hs-" + randomHex16(device);
        std::lock_guard<std::mutex> lock(mutexFor(matchId));
        record.filename = matchId;
        CodeEnum error = CODE_SUCCESS;
        if (store_.save(true, error, record.toJson())) {
            publishSnapshots(store_, record); // revision 0; result ignored (SNAPSHOT-002)
            return ApiResponse{201, {{"match_id", matchId}, {"revision", 0}, {"phase", "LOBBY"}}};
        }
        if (error != CODE_INITIAL_STORE_BUT_WITH_EXISTING_STORED) {
            return errorResponse(500, "STORE_ERROR", "Failed to save match " + matchId);
        }
    }
    return failure;
}

ApiResponse HiderSeekerService::joinMatch(const std::string& matchId, const std::string& body) {
    json request;
    std::string account;
    ApiResponse failure;
    if (!parseAccountRequest(body, matchId, request, account, failure)) {
        return failure;
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    return runGuarded(store_, matchId, request, account, "join", "",
                      [&](MatchRecord& record, uint64_t) -> GuardedOutcome {
        MatchState& state = record.state;
        if (state.phase != PHASE_LOBBY) {
            return unrecorded(errorResponse(409, "MATCH_NOT_IN_LOBBY", "The match is not in its lobby"));
        }
        if (findAccountSlot(record, account) >= 0) {
            return unrecorded(errorResponse(409, "ALREADY_JOINED", "This account already joined the match"));
        }

        // Lowest-numbered free slot below hider_count.
        int freeSlot = -1;
        int slot = 0;
        for (const auto& bound : record.accounts) {
            if (slot >= state.hider_count) {
                break;
            }
            if (bound.empty()) {
                freeSlot = slot;
                break;
            }
            ++slot;
        }
        if (freeSlot < 0) {
            return unrecorded(errorResponse(409, "MATCH_FULL", "Every hider slot is taken"));
        }

        HiderState& hider = state.hiders[freeSlot];
        hider.active = 1;
        hider.status = HIDER_STATUS_ACTIVE;
        hider.action_points = PLACEHOLDER_START_ACTION_POINTS;
        hider.resources[LOA_RESOURCE_INDEX] = PLACEHOLDER_START_LOA;
        state.revision += 1;
        record.accounts.assignValue(freeSlot, account);

        return logged(ApiResponse{200, {{"hider_id", freeSlot}, {"revision", state.revision}}}, request);
    });
}

ApiResponse HiderSeekerService::startMatch(const std::string& matchId, const std::string& body) {
    json request;
    std::string account;
    ApiResponse failure;
    if (!parseAccountRequest(body, matchId, request, account, failure)) {
        return failure;
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    return runGuarded(store_, matchId, request, account, "start", "",
                      [&](MatchRecord& record, uint64_t) -> GuardedOutcome {
        MatchState& state = record.state;
        if (account != record.host) {
            return unrecorded(errorResponse(403, "NOT_HOST", "Only the host may start the match"));
        }
        if (state.phase != PHASE_LOBBY) {
            return unrecorded(errorResponse(409, "MATCH_NOT_IN_LOBBY", "The match is not in its lobby"));
        }
        const bool anyBound = std::any_of(record.accounts.begin(), record.accounts.end(),
                                          [](const std::string& bound) { return !bound.empty(); });
        if (!anyBound) {
            return unrecorded(errorResponse(409, "NO_HIDERS", "No hider has joined the match"));
        }

        state.phase = PHASE_ACTIVE;
        state.revision += 1;
        // Initialize new fields added in schema version 3
        state.hider_connection_pool = PLACEHOLDER_HIDER_CONNECTIONS_PER_HIDER * state.hider_count; // PLACEHOLDER pending BALANCE-001
        state.seeker_connection_mask = 0;
        state.winner = static_cast<uint8_t>(MatchWinner::NONE);
        
        // CONN-002c: Set all hiders to disconnected at match start
        for (int i = 0; i < state.hider_count; ++i) {
            state.hiders[i].server_id = NO_SERVER_ID;
        }
        return logged(ApiResponse{200, {{"revision", state.revision}, {"phase", "ACTIVE"}}}, request);
    });
}

ApiResponse HiderSeekerService::getMatch(const std::string& matchId) {
    if (!isValidMatchId(matchId)) {
        return invalidRequest("Invalid match id");
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    MatchRecord record;
    ApiResponse failure;
    if (!loadRecord(store_, matchId, record, failure)) {
        return failure;
    }

    json hiders = json::array();
    int slot = 0;
    for (const auto& bound : record.accounts) {
        if (slot >= record.state.hider_count) {
            break;
        }
        hiders.push_back({{"hider_id", slot}, {"account", bound.empty() ? json(nullptr) : json(bound)}});
        ++slot;
    }

    return ApiResponse{200, {
        {"match_id", record.filename},
        {"host", record.host},
        {"phase", phaseName(record.state.phase)},
        {"revision", record.state.revision},
        {"tick", record.state.tick},
        {"hiders", hiders}
    }};
}

ApiResponse HiderSeekerService::submitMove(const std::string& matchId, const std::string& body) {
    json request;
    std::string account;
    ApiResponse failure;
    if (!parseAccountRequest(body, matchId, request, account, failure)) {
        return failure;
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    return runGuarded(store_, matchId, request, account, "move", "",
                      [&](MatchRecord& record, uint64_t revisionBefore) -> GuardedOutcome {
        // MOVE-007 intent validation.
        std::string playMode;
        if (!readString(request, "play_mode", playMode) || !isValidPlayMode(playMode)) {
            return unrecorded(invalidRequest("play_mode must be PLAY, QUEUE, or PASS"));
        }
        const auto intentField = request.find("intent");
        if (intentField == request.end() || !intentField->is_object()) {
            return unrecorded(invalidRequest("intent must be a JSON object"));
        }
        const json& intent = *intentField;
        std::string intentType;
        if (!readString(intent, "type", intentType)) {
            return unrecorded(invalidRequest("intent.type is required"));
        }

        const bool isPass = playMode == "PASS";
        int64_t ticks = 0;
        bool hasServerId = false;
        int64_t serverId = NO_SERVER_ID;
        if (isPass) {
            if (intentType != "PASS") {
                return unrecorded(invalidRequest("play_mode PASS requires intent.type PASS"));
            }
            if (!readIntInRange(request, "ticks", 1, MAX_ADVANCE_TICKS, ticks)) {
                return unrecorded(invalidRequest("ticks must be an integer from 1 to 16"));
            }
        } else {
            if (!isValidIntentType(intentType)) {
                return unrecorded(invalidRequest("Unknown intent.type " + intentType));
            }
            if (intent.contains("server_id")) {
                if (!readIntInRange(intent, "server_id", 0, MAX_SERVERS - 1, serverId)) {
                    return unrecorded(invalidRequest("intent.server_id must be an integer from 0 to 15"));
                }
                hasServerId = true;
            }
            // CONN-API-001: CONNECT intent without server_id should return INVALID_REQUEST
            if (intentType == "CONNECT" && !hasServerId) {
                return unrecorded(invalidRequest("CONNECT requires intent.server_id"));
            }
            if (intentType == "REMOVE_SERVER" && !hasServerId) {
                return unrecorded(invalidRequest("REMOVE_SERVER requires intent.server_id"));
            }
        }

        const int slot = findAccountSlot(record, account);
        if (slot < 0) {
            return unrecorded(errorResponse(403, "NOT_IN_MATCH", "This account has not joined the match"));
        }
        MatchState& state = record.state;
        const TickActionKind kind = isPass ? TickActionKind::HIDER_ACTION : intentKind(intentType);
        const uint8_t occupiedServer = state.hiders[slot].server_id;
        // CONN-API-004: For CONNECT intent, don't apply SERVER_MISMATCH check
        if (!isPass && kind == TickActionKind::HIDER_ACTION && hasServerId && serverId != occupiedServer 
            && intentType != "CONNECT") {
            return unrecorded(errorResponse(400, "SERVER_MISMATCH",
                                            "Hider actions target the hider's own occupied server"));
        }

        // Dispatch into the turn engine.
        NullSeekerPolicy seekerPolicy;
        const std::vector<uint32_t> pendingBefore = pendingMoveIds(state);
        const uint32_t moveNumber = record.next_move_id;
        TurnResult result;
        if (isPass) {
            result = passTicks(state, static_cast<uint8_t>(ticks), seekerPolicy);
        } else {
            MoveRequest move{};
            move.move_id = moveNumber;
            move.hider_id = static_cast<uint8_t>(slot);
            move.action_type = static_cast<uint8_t>(kind);
            // Hider actions act on the occupied server implicitly; only
            // REMOVE_SERVER and CONNECT name a server. goal_id is accepted and ignored.
            move.server_id = (kind == TickActionKind::REMOVE_SERVER || kind == TickActionKind::CONNECT) 
                           ? static_cast<uint8_t>(serverId) : NO_SERVER_ID;
            move.target_id = NO_SERVER_ID;
            move.server_mode = static_cast<uint8_t>(intentMode(intentType));
            result = playMode == "PLAY" ? playMove(state, move, seekerPolicy) : queueMove(state, move, seekerPolicy);
        }

        if (result.status == MoveStatus::REJECTED) {
            // The engine leaves state untouched on REJECTED.
            return recordedOnly(ApiResponse{422, {
                {"status", moveStatusName(result.status)},
                {"rule_outcome", ruleOutcomeField(result)},
                {"revision", state.revision},
                {"tick", state.tick}
            }});
        }

        appendTurnPublicEvents(record, result);
        json responseBody = turnBody(record, result, revisionBefore, pendingBefore);
        if (!isPass) {
            record.next_move_id += 1;
            responseBody["intent_id"] = formatMoveId(moveNumber);
            if (result.status == MoveStatus::QUEUED) {
                record.moves.push_back(
                    MoveOwnershipEntry{formatMoveId(moveNumber), account, static_cast<uint8_t>(slot)});
            }
        }
        pruneMoves(record);
        return logged(ApiResponse{200, responseBody}, request);
    });
}

ApiResponse HiderSeekerService::cancelMove(const std::string& matchId, const std::string& intentId,
                                           const std::string& body) {
    json request;
    std::string account;
    ApiResponse failure;
    if (!parseAccountRequest(body, matchId, request, account, failure)) {
        return failure;
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    return runGuarded(store_, matchId, request, account, "cancel", intentId,
                      [&](MatchRecord& record, uint64_t revisionBefore) -> GuardedOutcome {
        uint32_t moveNumber = 0;
        if (!parseMoveId(intentId, moveNumber)) {
            return unrecorded(invalidRequest("intent id must look like mv-<n>"));
        }
        const auto owned = std::find_if(record.moves.begin(), record.moves.end(),
                                        [&](const MoveOwnershipEntry& entry) {
                                            return entry.move_id == intentId && entry.account == account;
                                        });
        if (owned == record.moves.end()) {
            return unrecorded(errorResponse(404, "INTENT_NOT_FOUND", "No pending move " + intentId + " for this account"));
        }

        MatchState& state = record.state;
        const std::vector<uint32_t> pendingBefore = pendingMoveIds(state);
        const TurnResult result = cancelPendingMove(state, owned->hider_id, moveNumber);
        if (result.status != MoveStatus::CANCELLED) {
            // REJECTED or NOT_FOUND: the engine left state untouched.
            return unrecorded(errorResponse(404, "INTENT_NOT_FOUND", "No pending move " + intentId + " for this account"));
        }

        appendTurnPublicEvents(record, result);
        json responseBody = turnBody(record, result, revisionBefore, pendingBefore);
        responseBody["intent_id"] = intentId;
        pruneMoves(record);

        json logBody = request;
        logBody["intent_id"] = intentId;
        return logged(ApiResponse{200, responseBody}, logBody);
    });
}

ApiResponse HiderSeekerService::getLatestView(const std::string& matchId, const std::string& audience,
                                              const std::string& account, const std::string& ifNoneMatch) {
    if (!isValidMatchId(matchId)) {
        return invalidRequest("Invalid match id");
    }
    if (!isValidAudience(audience)) {
        return invalidRequest("audience must be hider or public");
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    MatchRecord record;
    int slot = -1;
    std::string audienceKey;
    ApiResponse failure;
    if (!resolveViewRequest(store_, matchId, audience, account, record, slot, audienceKey, failure)) {
        return failure;
    }

    const uint64_t revision = record.state.revision;
    json view;
    if (!loadSnapshot(store_, matchId, audienceKey, revision, view)) {
        // Rebuild on demand (latest only); storing it is best-effort.
        const ViewContext ctx = viewContextOf(record);
        view = slot >= 0 ? buildHiderView(record.state, ctx, static_cast<uint8_t>(slot))
                         : buildPublicView(record.state, ctx);
        writeSnapshotIfAbsent(store_, matchId, audienceKey, revision, view);
    }
    return viewResponse(view, LATEST_CACHE_CONTROL, ifNoneMatch);
}

ApiResponse HiderSeekerService::getRevisionView(const std::string& matchId, const std::string& audience,
                                                const std::string& revision, const std::string& account,
                                                const std::string& ifNoneMatch) {
    if (!isValidMatchId(matchId)) {
        return invalidRequest("Invalid match id");
    }
    if (!isValidAudience(audience)) {
        return invalidRequest("audience must be hider or public");
    }
    uint64_t revisionNumber = 0;
    bool outOfRange = false;
    if (!parseRevision(revision, revisionNumber, outOfRange)) {
        return invalidRequest("revision must be 1 to 20 digits");
    }

    std::lock_guard<std::mutex> lock(mutexFor(matchId));
    MatchRecord record;
    int slot = -1;
    std::string audienceKey;
    ApiResponse failure;
    if (!resolveViewRequest(store_, matchId, audience, account, record, slot, audienceKey, failure)) {
        return failure;
    }

    // Stored snapshots only: a revision view is never rebuilt from state.
    json view;
    if (outOfRange || !loadSnapshot(store_, matchId, audienceKey, revisionNumber, view)) {
        return errorResponse(404, "VIEW_NOT_FOUND", "No stored view of revision " + revision);
    }
    return viewResponse(view, REVISION_CACHE_CONTROL, ifNoneMatch);
}

} // namespace hider_seeker
