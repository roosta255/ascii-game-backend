#include "MatchStateJson.hpp"

namespace hider_seeker {

namespace {

const char* phaseToString(uint8_t phase) {
    switch (phase) {
        case PHASE_LOBBY:    return "LOBBY";
        case PHASE_ACTIVE:   return "ACTIVE";
        case PHASE_COMPLETE: return "COMPLETE";
        default:             return "UNKNOWN";
    }
}

template <typename T, size_t N>
Json::Value byteArrayToJson(const T (&values)[N]) {
    Json::Value out(Json::arrayValue);
    for (size_t i = 0; i < N; ++i) out.append(static_cast<int>(values[i]));
    return out;
}

Json::Value metricFrameToJson(const MetricFrame& frame) {
    Json::Value out;
    out["cpu"] = frame.cpu;
    out["memory"] = frame.memory;
    out["disk"] = frame.disk;
    out["network"] = frame.network;
    out["heat"] = frame.heat;
    out["errors"] = frame.errors;
    out["users"] = frame.users;
    return out;
}

Json::Value serverStateToJson(const ServerState& server) {
    Json::Value out;
    out["active"] = server.active != 0;
    out["owner_hider_id"] = server.owner_hider_id;
    out["public_flags"] = server.public_flags;
    out["trace_strength"] = server.trace_strength;
    out["trace_age"] = server.trace_age;
    out["current"] = metricFrameToJson(server.current);
    Json::Value history(Json::arrayValue);
    for (uint8_t i = 0; i < HISTORY_TICKS; ++i) history.append(metricFrameToJson(server.history[i]));
    out["history"] = history;
    out["history_head"] = server.history_head;
    return out;
}

Json::Value hiderStateToJson(const std::string& account, const HiderState& hider) {
    Json::Value out;
    out["account"] = account;
    out["active"] = hider.active != 0;
    out["status"] = hider.status;
    out["action_points"] = hider.action_points;
    out["server_id"] = hider.server_id;
    out["resources"] = byteArrayToJson(hider.resources);
    out["goal_progress"] = byteArrayToJson(hider.goal_progress);
    return out;
}

Json::Value seekerStateToJson(const std::string& account, const SeekerState& seeker) {
    Json::Value out;
    out["account"] = account;
    out["connection_tokens"] = seeker.connection_tokens;
    out["max_tokens"] = seeker.max_tokens;
    out["heat"] = seeker.heat;
    out["belief"] = byteArrayToJson(seeker.belief);
    out["target_ids"] = byteArrayToJson(seeker.target_ids);
    out["target_count"] = seeker.target_count;
    out["target_head"] = seeker.target_head;
    out["last_probe_tick"] = byteArrayToJson(seeker.last_probe_tick);
    return out;
}

Json::Value requirementToJson(const Requirement& requirement) {
    Json::Value out;
    out["type"] = requirement.type;
    out["subject_id"] = requirement.subject_id;
    out["amount"] = requirement.amount;
    out["threshold"] = requirement.threshold;
    return out;
}

Json::Value goalStateToJson(const GoalState& goal) {
    Json::Value out;
    out["goal_id"] = goal.goal_id;
    out["status"] = goal.status;
    out["reward_type"] = goal.reward_type;
    out["reward_amount"] = goal.reward_amount;
    out["completion_action"] = goal.completion_action;
    out["deadline_tick"] = goal.deadline_tick;
    Json::Value requirements(Json::arrayValue);
    for (uint8_t i = 0; i < goal.requirement_count && i < MAX_REQUIREMENTS_PER_GOAL; ++i) {
        requirements.append(requirementToJson(goal.requirements[i]));
    }
    out["requirements"] = requirements;
    return out;
}

} // namespace

Json::Value matchRecordToJson(const std::string& matchId, const MatchRecord& record) {
    const MatchState& state = record.state;
    Json::Value out;
    out["match"] = matchId;
    out["host"] = record.host;
    out["phase"] = phaseToString(state.phase);
    out["schema_version"] = state.schema_version;
    out["ruleset_version"] = state.ruleset_version;
    out["revision"] = static_cast<Json::UInt64>(state.revision);
    out["tick"] = state.tick;

    Json::Value hiders(Json::arrayValue);
    for (uint8_t i = 0; i < state.hider_count && i < MAX_HIDERS; ++i) {
        hiders.append(hiderStateToJson(record.hiderAccounts[i], state.hiders[i]));
    }
    out["hiders"] = hiders;

    Json::Value servers(Json::arrayValue);
    for (uint8_t i = 0; i < state.server_count && i < MAX_SERVERS; ++i) {
        servers.append(serverStateToJson(state.servers[i]));
    }
    out["servers"] = servers;

    out["seeker"] = record.seekerAccount.empty()
        ? Json::Value()
        : seekerStateToJson(record.seekerAccount, state.seeker);

    Json::Value goals(Json::arrayValue);
    for (uint8_t i = 0; i < MAX_GOALS; ++i) {
        if (!state.goals[i].active) continue;
        goals.append(goalStateToJson(state.goals[i]));
    }
    out["goals"] = goals;

    out["event_count"] = state.event_count;

    return out;
}

} // namespace hider_seeker
