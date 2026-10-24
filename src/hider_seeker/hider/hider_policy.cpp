#include "hider_policy.hpp"
#include "../domain/state.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/server_metrics.hpp"
#include <cmath>

namespace hider_seeker {

bool requirementMet(const nlohmann::json& view, const nlohmann::json& requirement) {
    if (requirement["type"] == "ACTIVE_SERVER_COUNT") {
        return view["servers"].size() >= requirement["threshold"];
    } else if (requirement["type"] == "SERVER_METRIC_THRESHOLD") {
        int serverId = requirement["server_id"];
        // Look for the server in the view
        for (const auto& server : view["servers"]) {
            if (server["server_id"] == serverId) {
                // Found the server, check its metric
                std::string metric = requirement["metric"].get<std::string>();
                // Convert to lowercase for JSON key lookup
                if (metric == "CPU") {
                    return server["metrics"]["cpu"] >= requirement["threshold"];
                } else if (metric == "NETWORK") {
                    return server["metrics"]["network"] >= requirement["threshold"];
                }
                // Unknown metric type - should not happen according to spec
                return false;
            }
        }
        // Server not found in view
        return false;
    }
    // Unknown requirement type
    return false;
}

bool goalSupported(const nlohmann::json& goal) {
    // Check status is OPEN
    if (goal["status"] != "OPEN") {
        return false;
    }
    
    // Check reward_amount > 0
    if (goal["reward_amount"] <= 0) {
        return false;
    }
    
    // Check requirements is not empty array
    if (!goal["requirements"].is_array() || goal["requirements"].empty()) {
        return false;
    }
    
    // Check each requirement
    for (const auto& req : goal["requirements"]) {
        // Requirement type must be either ACTIVE_SERVER_COUNT or SERVER_METRIC_THRESHOLD
        if (req["type"] == "ACTIVE_SERVER_COUNT") {
            // This is valid
        } else if (req["type"] == "SERVER_METRIC_THRESHOLD") {
            // Check that metric is CPU or NETWORK
            std::string metric = req["metric"].get<std::string>();
            if (metric != "CPU" && metric != "NETWORK") {
                return false;
            }
        } else {
            // Invalid requirement type
            return false;
        }
    }
    
    return true;
}

int goalCost(const nlohmann::json& view, const nlohmann::json& goal) {
    // Start with the base cost of PAY_GOAL (10 AP)
    int total = hiderActionDef(HiderActionMode::PAY_GOAL).ap_cost;
    
    // Process each unmet requirement
    for (const auto& requirement : goal["requirements"]) {
        if (!requirementMet(view, requirement)) {
            if (requirement["type"] == "ACTIVE_SERVER_COUNT") {
                int threshold = requirement["threshold"];
                int serverCount = view["servers"].size();
                int loa = view["self"]["loa"];
                
                // If we need more servers than we can add with our LOA or exceed MAX_SERVERS
                if (threshold - serverCount > loa || threshold > MAX_SERVERS) {
                    return GOAL_INFEASIBLE;
                }
                // ADD_SERVER costs 0 AP, so we don't add anything to total
            } else if (requirement["type"] == "SERVER_METRIC_THRESHOLD") {
                int serverId = requirement["server_id"];
                std::string metric = requirement["metric"].get<std::string>();
                int threshold = requirement["threshold"];
                
                // Find the server in view
                int serverIndex = -1;
                for (size_t i = 0; i < view["servers"].size(); ++i) {
                    if (view["servers"][i]["server_id"] == serverId) {
                        serverIndex = i;
                        break;
                    }
                }
                
                // If server not found, goal is infeasible
                if (serverIndex == -1) {
                    return GOAL_INFEASIBLE;
                }
                
                // Determine the action to use based on metric
                HiderActionMode selectedAction;
                if (metric == "CPU") {
                    selectedAction = HiderActionMode::BOOST_CPU;
                } else if (metric == "NETWORK") {
                    selectedAction = HiderActionMode::ROTATE_ROUTE;
                } else {
                    // Should not happen according to goalSupported check
                    return GOAL_INFEASIBLE;
                }
                
                const HiderActionDef& actionDef = hiderActionDef(selectedAction);
                int ap_cost = actionDef.ap_cost;
                
                // Find the effect that matches our metric
                int16_t gain = 0;
                for (uint8_t i = 0; i < actionDef.effect_count; ++i) {
                    const ActionEffect& effect = actionDef.effects.getOrDefault(i, ActionEffect{});
                    if (metric == "CPU" && effect.metric == SERVER_METRIC_CPU) {
                        gain = effect.delta;
                        break;
                    } else if (metric == "NETWORK" && effect.metric == SERVER_METRIC_NETWORK) {
                        gain = effect.delta;
                        break;
                    }
                }
                
                // If no valid gain found or gain is not positive, goal is infeasible
                if (gain <= 0) {
                    return GOAL_INFEASIBLE;
                }
                
                // Get current metric value
                int current = view["servers"][serverIndex]["metrics"][metric == "CPU" ? "cpu" : "network"];
                
                // Calculate required executions using ceiling division
                int requiredExecutions = (threshold - current + gain - 1) / gain;
                
                // Add cost of required executions
                total += requiredExecutions * ap_cost;
            }
        }
    }
    
    return total;
}

namespace {
bool hasOwnPending(const nlohmann::json& view) {
    int selfId = view["self"]["hider_id"];
    for (const auto& pending : view["pending"]) {
        if (pending["hider_id"] == selfId) {
            return true;
        }
    }
    return false;
}

bool allRequirementsMet(const nlohmann::json& view, const nlohmann::json& goal) {
    for (const auto& requirement : goal["requirements"]) {
        if (!requirementMet(view, requirement)) {
            return false;
        }
    }
    return true;
}

HiderNpcChoice payGoal(const nlohmann::json& view, const nlohmann::json& goal) {
    HiderNpcChoice choice;
    choice.has_move = true;
    choice.goal_id = goal["goal_id"];
    
    if (!view["self"]["server_id"].is_null()) {
        // We're connected to a server, so we can pay the goal
        choice.intent = {{"type", "PAY_GOAL"}};
    } else {
        // We're disconnected, so we need to connect first
        if (view["team_connections"] > 0 && view["servers"].size() > 0) {
            choice.intent = {{"type", "CONNECT"}, {"server_id", view["servers"][0]["server_id"]}};
        } else {
            // No move possible due to no connections or no servers
            choice.has_move = false;
        }
    }
    
    return choice;
}

int cheapestGoal(const nlohmann::json& view) {
    int bestCost = GOAL_INFEASIBLE;
    int bestId = -1;
    
    for (const auto& goal : view["goals"]) {
        if (!goalSupported(goal)) {
            continue;
        }
        
        int cost = goalCost(view, goal);
        if (cost != GOAL_INFEASIBLE) {
            if (bestId == -1 || cost < bestCost || (cost == bestCost && goal["goal_id"] < bestId)) {
                bestCost = cost;
                bestId = goal["goal_id"];
            }
        }
    }
    
    return bestId;
}

int firstUnmetRequirement(const nlohmann::json& view, const nlohmann::json& goal) {
    for (size_t i = 0; i < goal["requirements"].size(); ++i) {
        if (!requirementMet(view, goal["requirements"][i])) {
            return i;
        }
    }
    return -1; // All requirements met
}

HiderNpcChoice nextMove(const nlohmann::json& view, const nlohmann::json& goal) {
    HiderNpcChoice choice;
    choice.has_move = true;
    choice.goal_id = goal["goal_id"];
    
    int unmetIndex = firstUnmetRequirement(view, goal);
    if (unmetIndex == -1) {
        // All requirements met - this shouldn't happen in step 3
        return choice;
    }
    
    const auto& requirement = goal["requirements"][unmetIndex];
    
    if (requirement["type"] == "ACTIVE_SERVER_COUNT") {
        choice.intent = {{"type", "ADD_SERVER"}};
    } else if (requirement["type"] == "SERVER_METRIC_THRESHOLD") {
        int serverId = requirement["server_id"];
        std::string metric = requirement["metric"].get<std::string>();
        
        // Check if hider is connected to this server
        bool isConnected = !view["self"]["server_id"].is_null() && 
                          view["self"]["server_id"] == serverId;
        
        if (isConnected) {
            // Hider is connected to the target server, so boost the metric
            if (metric == "CPU") {
                choice.intent = {{"type", hiderActionName(HiderActionMode::BOOST_CPU)}};
            } else if (metric == "NETWORK") {
                choice.intent = {{"type", hiderActionName(HiderActionMode::ROTATE_ROUTE)}};
            }
        } else if (view["team_connections"] > 0) {
            // Hider is disconnected or connected to a different server, so connect
            choice.intent = {{"type", "CONNECT"}, {"server_id", serverId}};
        } else {
            // No connections available, no move possible
            choice.has_move = false;
        }
    }
    
    return choice;
}

} // anonymous namespace

HiderNpcChoice chooseHiderMove(const nlohmann::json& view) {
    // Step 1: Return default if conditions are not met
    if (view["phase"] != "ACTIVE" || 
        view["self"]["status"] != "ACTIVE" || 
        hasOwnPending(view)) {
        return HiderNpcChoice{};
    }
    
    // Step 2: Try to pay a met goal
    for (const auto& goal : view["goals"]) {
        if (!goalSupported(goal)) {
            continue;
        }
        
        if (allRequirementsMet(view, goal)) {
            return payGoal(view, goal);
        }
    }
    
    // Step 3: Select cheapest feasible goal and map its first unmet requirement to an intent
    int cheapestId = cheapestGoal(view);
    if (cheapestId == -1) {
        // No feasible goal found
        return HiderNpcChoice{};
    }
    
    // Find the cheapest goal by ID
    for (const auto& goal : view["goals"]) {
        if (goal["goal_id"] == cheapestId) {
            return nextMove(view, goal);
        }
    }
    
    // Should not happen, but just in case
    return HiderNpcChoice{};
}

} // namespace hider_seeker