#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "hider_policy.hpp"

using json = nlohmann::json;

namespace hider_seeker {

namespace {
// Helper functions for test setup (copied from hider_policy_eval_test.cpp)
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
} // anonymous namespace


TEST_CASE("NPC-020 (no move)", "[hider_policy_choice]") {
    json view = baseView();
    
    // When view["phase"] = "COMPLETE", the result's has_move is false
    view["phase"] = "COMPLETE";
    REQUIRE(chooseHiderMove(view).has_move == false);
    
    // When view["self"]["status"] = "CAPTURED", the result's has_move is false
    view["phase"] = "ACTIVE";
    view["self"]["status"] = "CAPTURED";
    REQUIRE(chooseHiderMove(view).has_move == false);
    
    // When view["pending"] contains a move with hider_id = 1, the result's has_move is false
    view["self"]["status"] = "ACTIVE";
    view["pending"] = json::array({json{{"hider_id", 1}, {"intent_id", "mv-3"}}});
    REQUIRE(chooseHiderMove(view).has_move == false);
    
    // When view["pending"] contains a teammate's move (hider_id = 0), the result's has_move is true
    view["pending"] = json::array({json{{"hider_id", 0}, {"intent_id", "mv-3"}}});
    REQUIRE(chooseHiderMove(view).has_move == true);
}

TEST_CASE("NPC-021 (cheapest goal)", "[hider_policy_choice]") {
    json view = baseView();
    
    // Should select goal with cheapest cost and intent ADD_SERVER
    auto choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "ADD_SERVER"}});
    REQUIRE(choice.goal_id == 2); // Goal 2 (countRequirement(3)) is cheapest at 10 AP
}

TEST_CASE("NPC-022 (pay a met goal)", "[hider_policy_choice]") {
    json view = baseView();
    
    // Add a third server to make goal 2 met
    view["servers"].push_back(server(2, 0, 0));
    
    // Should select goal 2 and connect to server 0
    auto choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "CONNECT"}, {"server_id", 0}});
    REQUIRE(choice.goal_id == 2);
    
    // With team_connections = 0, there should be no move
    view["team_connections"] = 0;
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == false);
    
    // With team_connections = 4 and self connected to server 1, should pay goal
    view["team_connections"] = 4;
    view["self"]["server_id"] = 1;
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "PAY_GOAL"}});
    REQUIRE(choice.goal_id == 2);
    
    // After setting server 0's CPU to 45, goal 0 should be met too
    view["servers"][0]["metrics"]["cpu"] = 45;
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.goal_id == 0); // Lower index goal should be chosen
}

TEST_CASE("NPC-023 (metric goal handling)", "[hider_policy_choice]") {
    json view = baseView();
    
    // With loa = 0, goal 2 becomes infeasible, so goal 0 at 16 AP is cheapest
    view["self"]["loa"] = 0;
    auto choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "CONNECT"}, {"server_id", 0}});
    REQUIRE(choice.goal_id == 0);
    
    // With self connected to server 1, should connect to server 0 (since that's where the goal requires metric improvement)
    view["self"]["server_id"] = 1;
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "CONNECT"}, {"server_id", 0}});
    
    // With self connected to server 0, should boost CPU
    view["self"]["server_id"] = 0;
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "BOOST_CPU"}});
    
    // Mark goals 0, 2 and 3 as COMPLETED
    view["goals"][0]["status"] = "COMPLETED";
    view["goals"][2]["status"] = "COMPLETED";
    view["goals"][3]["status"] = "COMPLETED";
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == true);
    REQUIRE(choice.intent == json{{"type", "ROTATE_ROUTE"}});
    REQUIRE(choice.goal_id == 1); // Goal 1 is cheapest among remaining
    
    // Mark goal 1 as COMPLETED too, should result in no move
    view["goals"][1]["status"] = "COMPLETED";
    choice = chooseHiderMove(view);
    REQUIRE(choice.has_move == false);
}

} // namespace hider_seeker