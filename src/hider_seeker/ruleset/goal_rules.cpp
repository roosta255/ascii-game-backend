#include "goal_rules.hpp"
#include "server_metrics.hpp"
#include <cstdint>

namespace hider_seeker {

bool requirementMet(MatchState& state, const Requirement& requirement) {
    switch (requirement.type) {
        case REQ_ACTIVE_SERVER_COUNT: {
            // Count servers with active != 0
            uint16_t count = 0;
            for (const auto& server : state.servers) {
                if (server.active != 0) {
                    count++;
                }
            }
            return count >= requirement.threshold;
        }
        case REQ_SERVER_METRIC_THRESHOLD: {
            // Check that subject_id is valid
            if (requirement.subject_id >= MAX_SERVERS) {
                return false;
            }
            // Check that the server is active
            if (!state.servers[requirement.subject_id].active) {
                return false;
            }
            // Check the metric threshold
            ServerState& server = state.servers[requirement.subject_id];
            ServerMetric metric = static_cast<ServerMetric>(requirement.amount);
            uint8_t value = serverMetricField(server, metric).orElse(0);
            return value >= requirement.threshold;
        }
        default:
            return false;
    }
}

bool goalRequirementsMet(MatchState& state, const GoalState& goal) {
    // If there are no requirements, the goal is not met
    if (goal.requirement_count == 0) {
        return false;
    }
    
    // Check each requirement
    for (int i = 0; i < goal.requirement_count; ++i) {
        if (!requirementMet(state, goal.requirements[i])) {
            return false;
        }
    }
    return true;
}

int completableGoal(MatchState& state) {
    // Iterate goal slots from 0 upward
    for (int g = 0; g < MAX_GOALS; ++g) {
        // Check if the goal is active, open, and has all requirements met
        if (state.goals[g].active != 0 && 
            state.goals[g].status == GOAL_STATUS_OPEN && 
            goalRequirementsMet(state, state.goals[g])) {
            return g;
        }
    }
    return -1;
}

uint16_t teamReward(const MatchState& state) {
    uint16_t total = 0;
    for (int g = 0; g < MAX_GOALS; ++g) {
        // Only count active goals with completed status
        if (state.goals[g].active != 0 && 
            state.goals[g].status == GOAL_STATUS_COMPLETED) {
            total += state.goals[g].reward_amount;
        }
    }
    return total;
}

void seedPlaceholderGoals(MatchState& state) {
    // Zero-initialize every element of state.goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        state.goals[i] = GoalState{};
    }
    
    // Write placeholder goals for slots 0-3
    // Slot 0: REQ_SERVER_METRIC_THRESHOLD, subject_id = 0, amount = SERVER_METRIC_CPU, threshold = 40
    state.goals[0].active = 1;
    state.goals[0].goal_id = 0;
    state.goals[0].status = GOAL_STATUS_OPEN;
    state.goals[0].reward_type = REWARD;
    state.goals[0].reward_amount = 1;
    state.goals[0].requirement_count = 1;
    state.goals[0].completion_action = 0;
    state.goals[0].deadline_tick = 0;
    state.goals[0].requirements[0].type = REQ_SERVER_METRIC_THRESHOLD;
    state.goals[0].requirements[0].subject_id = 0;
    state.goals[0].requirements[0].amount = SERVER_METRIC_CPU;
    state.goals[0].requirements[0].threshold = 40;
    
    // Slot 1: REQ_SERVER_METRIC_THRESHOLD, subject_id = 0, amount = SERVER_METRIC_NETWORK, threshold = 20
    state.goals[1].active = 1;
    state.goals[1].goal_id = 1;
    state.goals[1].status = GOAL_STATUS_OPEN;
    state.goals[1].reward_type = REWARD;
    state.goals[1].reward_amount = 1;
    state.goals[1].requirement_count = 1;
    state.goals[1].completion_action = 0;
    state.goals[1].deadline_tick = 0;
    state.goals[1].requirements[0].type = REQ_SERVER_METRIC_THRESHOLD;
    state.goals[1].requirements[0].subject_id = 0;
    state.goals[1].requirements[0].amount = SERVER_METRIC_NETWORK;
    state.goals[1].requirements[0].threshold = 20;
    
    // Slot 2: REQ_ACTIVE_SERVER_COUNT, subject_id = 0, amount = 0, threshold = 3
    state.goals[2].active = 1;
    state.goals[2].goal_id = 2;
    state.goals[2].status = GOAL_STATUS_OPEN;
    state.goals[2].reward_type = REWARD;
    state.goals[2].reward_amount = 1;
    state.goals[2].requirement_count = 1;
    state.goals[2].completion_action = 0;
    state.goals[2].deadline_tick = 0;
    state.goals[2].requirements[0].type = REQ_ACTIVE_SERVER_COUNT;
    state.goals[2].requirements[0].subject_id = 0;
    state.goals[2].requirements[0].amount = 0;
    state.goals[2].requirements[0].threshold = 3;
    
    // Slot 3: REQ_SERVER_METRIC_THRESHOLD, subject_id = 0, amount = SERVER_METRIC_CPU, threshold = 80
    state.goals[3].active = 1;
    state.goals[3].goal_id = 3;
    state.goals[3].status = GOAL_STATUS_OPEN;
    state.goals[3].reward_type = REWARD;
    state.goals[3].reward_amount = 1;
    state.goals[3].requirement_count = 1;
    state.goals[3].completion_action = 0;
    state.goals[3].deadline_tick = 0;
    state.goals[3].requirements[0].type = REQ_SERVER_METRIC_THRESHOLD;
    state.goals[3].requirements[0].subject_id = 0;
    state.goals[3].requirements[0].amount = SERVER_METRIC_CPU;
    state.goals[3].requirements[0].threshold = 80;
}

} // namespace hider_seeker