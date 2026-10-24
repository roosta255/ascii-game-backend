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

} // namespace hider_seeker