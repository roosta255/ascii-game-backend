#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "hider_policy.hpp"

using json = nlohmann::json;

namespace hider_seeker {

// Helper functions for test setup
json server(int serverId, int cpu, int network) {
    return json{{"server_id", serverId},
                {"metrics", {{"cpu", cpu}, {"memory", 0}, {"disk", 0}, {"network", network},
                             {"heat", 0}, {"errors", 0}, {"users", 0}}}};
}

json metricRequirement(int serverId, const char* metric, int threshold) {
    return json{{"type", "SERVER_METRIC_THRESHOLD"}, {"server_id", serverId}, {"metric", metric}, {"threshold", threshold}};
}

json countRequirement(int threshold) {
    return json{{"type", "ACTIVE_SERVER_COUNT"}, {"server_id", nullptr}, {"metric", nullptr}, {"threshold", threshold}};
}

json goal(int goalId, json requirement, const char* status = "OPEN", int reward = 1) {
    return json{{"goal_id", goalId}, {"status", status}, {"reward_amount", reward},
                {"requirements", json::array({requirement})}};
}

// Hider 1, disconnected, 0 AP, 1 LOA; servers 0 and 1 active; the four placeholder goals.
json baseView() {
    return json{
        {"phase", "ACTIVE"},
        {"self", {{"hider_id", 1}, {"status", "ACTIVE"}, {"action_points", 0}, {"server_id", nullptr}, {"loa", 1}}},
        {"servers", json::array({server(0, 0, 0), server(1, 0, 0)})},
        {"pending", json::array()},
        {"team_connections", 4},
        {"goals", json::array({goal(0, metricRequirement(0, "CPU", 40)), goal(1, metricRequirement(0, "NETWORK", 20)),
                               goal(2, countRequirement(3)), goal(3, metricRequirement(0, "CPU", 80))})}};
}

TEST_CASE("NPC-010 requirementMet tests", "[hider_policy]") {
    json view = baseView();
    
    // Test metricRequirement(0, "CPU", 40) is not met initially
    REQUIRE(!requirementMet(view, metricRequirement(0, "CPU", 40)));
    
    // After setting view["servers"][0]["metrics"]["cpu"] = 40; it should be met
    view["servers"][0]["metrics"]["cpu"] = 40;
    REQUIRE(requirementMet(view, metricRequirement(0, "CPU", 40)));
    
    // metricRequirement(0, "NETWORK", 20) should still not be met
    REQUIRE(!requirementMet(view, metricRequirement(0, "NETWORK", 20)));
    
    // metricRequirement(5, "CPU", 0) should not be met: server 5 is not in the view
    REQUIRE(!requirementMet(view, metricRequirement(5, "CPU", 0)));
    
    // countRequirement(2) should be met (we have 2 servers)
    REQUIRE(requirementMet(view, countRequirement(2)));
    
    // countRequirement(3) should not be met (we have 2 servers)
    REQUIRE(!requirementMet(view, countRequirement(3)));
    
    // A requirement with type "ACTION_COUNT" should not be met
    json actionCountReq = {{"type", "ACTION_COUNT"}, {"server_id", nullptr}, {"metric", nullptr}, {"threshold", 1}};
    REQUIRE(!requirementMet(view, actionCountReq));
}

TEST_CASE("NPC-011 goalSupported tests", "[hider_policy]") {
    // These should be supported
    REQUIRE(goalSupported(goal(0, metricRequirement(0, "CPU", 40))));
    REQUIRE(goalSupported(goal(1, metricRequirement(0, "NETWORK", 20))));
    REQUIRE(goalSupported(goal(2, countRequirement(3))));
    
    // These should not be supported
    
    // Status not OPEN
    REQUIRE(!goalSupported(goal(0, metricRequirement(0, "CPU", 40), "COMPLETED")));
    
    // Reward 0
    REQUIRE(!goalSupported(goal(0, metricRequirement(0, "CPU", 40), "OPEN", 0)));
    
    // Metric "HEAT" (not CPU or NETWORK)
    json heatReq = {{"type", "SERVER_METRIC_THRESHOLD"}, {"server_id", 0}, {"metric", "HEAT"}, {"threshold", 10}};
    REQUIRE(!goalSupported(goal(0, heatReq)));
    
    // Requirement type "RESOURCE_MIN"
    json resourceMinReq = {{"type", "RESOURCE_MIN"}, {"server_id", nullptr}, {"metric", nullptr}, {"threshold", 10}};
    REQUIRE(!goalSupported(goal(0, resourceMinReq)));
    
    // Empty requirements array
    json emptyGoal = {{"goal_id", 0}, {"status", "OPEN"}, {"reward_amount", 1}, {"requirements", json::array()}};
    REQUIRE(!goalSupported(emptyGoal));
}

TEST_CASE("NPC-012 goalCost tests", "[hider_policy]") {
    json view = baseView();
    
    // goalCost of view["goals"][0] should be 16
    REQUIRE(goalCost(view, view["goals"][0]) == 16);
    
    // goalCost of view["goals"][1] should be 26
    REQUIRE(goalCost(view, view["goals"][1]) == 26);
    
    // goalCost of view["goals"][2] should be 10
    REQUIRE(goalCost(view, view["goals"][2]) == 10);
    
    // goalCost of view["goals"][3] should be 22
    REQUIRE(goalCost(view, view["goals"][3]) == 22);
    
    // After setting view["servers"][0]["metrics"]["cpu"] = 50;
    view["servers"][0]["metrics"]["cpu"] = 50;
    
    // Goal 0 should cost 10
    REQUIRE(goalCost(view, view["goals"][0]) == 10);
    
    // Goal 3 should cost 14
    REQUIRE(goalCost(view, view["goals"][3]) == 14);
    
    // goalCost(view, goal(4, countRequirement(4))) should be GOAL_INFEASIBLE
    REQUIRE(goalCost(view, goal(4, countRequirement(4))) == GOAL_INFEASIBLE);
    
    // goalCost(view, goal(5, metricRequirement(7, "CPU", 10))) should be GOAL_INFEASIBLE
    REQUIRE(goalCost(view, goal(5, metricRequirement(7, "CPU", 10))) == GOAL_INFEASIBLE);
}

} // namespace hider_seeker