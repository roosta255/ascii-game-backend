#pragma once

#include <cstdint>

namespace hider_seeker {

constexpr uint8_t MAX_HIDERS = 8;
constexpr uint8_t MAX_SERVERS = 16;
constexpr uint8_t MAX_GOALS = 16;
constexpr uint8_t MAX_REQUIREMENTS_PER_GOAL = 16;
constexpr uint8_t MAX_EVENTS = 64;
constexpr uint8_t HISTORY_TICKS = 16;
constexpr uint8_t MAX_INTENTS_PER_TICK = 64;
constexpr uint8_t MAX_SEEKER_TARGETS = 16;
constexpr uint8_t MAX_HIDER_PLAN_STEPS = 16;
constexpr uint8_t RESOURCE_TYPE_COUNT = 8; // placeholder value for this slice

enum RewardType { REWARD_NONE, REWARD };

enum RequirementType {
    REQ_RESOURCE_MIN,
    REQ_RESOURCE_SPEND,
    REQ_ACTIVE_SERVER_COUNT,
    REQ_SERVER_METRIC_THRESHOLD,
    REQ_ACTION_COUNT,
    REQ_CONFIGURED_SERVER,
    REQ_CUSTOM_RULE
};

enum MatchPhase { PHASE_LOBBY, PHASE_ACTIVE, PHASE_COMPLETE };

struct MetricFrame {
    uint8_t cpu, memory, disk, network, heat, errors, users;
};

struct ServerState {
    uint8_t active;
    uint8_t owner_hider_id; // 0xFF == unowned
    uint8_t public_flags;
    uint8_t trace_strength;
    uint8_t trace_age;
    MetricFrame current;
    MetricFrame history[HISTORY_TICKS];
    uint8_t history_head;
};

struct HiderState {
    uint8_t active;
    uint8_t status;
    uint8_t action_points;
    uint8_t server_id;
    uint16_t resources[RESOURCE_TYPE_COUNT];
    uint8_t goal_progress[MAX_GOALS];
};

struct SeekerState {
    uint8_t connection_tokens;
    uint8_t max_tokens;
    uint8_t heat;
    uint8_t belief[MAX_SERVERS];
    uint8_t target_ids[MAX_SEEKER_TARGETS];
    uint8_t target_count;
    uint8_t target_head;
    uint8_t last_probe_tick[MAX_SERVERS];
};

struct Requirement {
    uint8_t type; // RequirementType
    uint8_t subject_id;
    uint16_t amount;
    uint16_t threshold;
};

struct GoalState {
    uint8_t active;
    uint8_t goal_id;
    uint8_t status;
    uint8_t reward_type; // RewardType
    uint16_t reward_amount;
    uint8_t requirement_count; // <= MAX_REQUIREMENTS_PER_GOAL
    Requirement requirements[MAX_REQUIREMENTS_PER_GOAL];
    uint8_t completion_action;
    uint16_t deadline_tick;
};

struct Event {
    uint8_t type;
    uint32_t tick;
};

struct MatchState {
    uint32_t schema_version;
    uint32_t ruleset_version;
    uint64_t revision;
    uint32_t tick;
    uint64_t rng_state;
    uint8_t phase; // MatchPhase
    uint8_t hider_count;
    uint8_t server_count;
    HiderState hiders[MAX_HIDERS];
    ServerState servers[MAX_SERVERS];
    SeekerState seeker;
    GoalState goals[MAX_GOALS];
    Event events[MAX_EVENTS];
    uint8_t event_head;
    uint8_t event_count;
};

static_assert(sizeof(MatchState) <= 8192, "MatchState exceeds its documented 8192-byte size budget — see state.hpp comment for the breakdown");

} // namespace hider_seeker