#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "service_test_support.hpp"
#include "HiderSeekerService.hpp"

using json = nlohmann::json;

namespace hider_seeker {

using hs_test::ServiceFixture;

namespace {

ApiResponse createWithNpcs(ServiceFixture& fx, int hiderCount, const json& npcHiders) {
    const ApiResponse response = fx.service.createMatch(
        json{{"account", "host"}, {"hider_count", hiderCount}, {"seed", 42}, {"npc_hiders", npcHiders}}.dump());
    if (response.status == 201) {
        fx.matchId = response.body.at("match_id").get<std::string>();
    }
    return response;
}

} // namespace

TEST_CASE("NPC-030 (slots bound at creation)") {
    hs_test::ServiceFixture fx;
    const ApiResponse response = createWithNpcs(fx, 3, 2);
    REQUIRE(response.status == 201);

    const auto& match = fx.getMatch().body;
    const auto& hiders = match.at("hiders");
    REQUIRE(hiders.size() == 3);
    
    // Slot 0 should be free (null account)
    REQUIRE(hiders[0].at("account").is_null());
    
    // Slot 1 should be bound to npc_1
    REQUIRE(hiders[1].at("account") == "npc_1");
    
    // Slot 2 should be bound to npc_2
    REQUIRE(hiders[2].at("account") == "npc_2");

    // Check internal state
    const auto& state = fx.record().state;
    REQUIRE(state.revision == 0);
    REQUIRE(state.hiders[0].active == 0);
    
    // Slots 1 and 2 should be active with correct properties (using constants from match_setup.hpp)
    REQUIRE(state.hiders[1].active == 1);
    REQUIRE(state.hiders[1].status == 0);  // HIDER_STATUS_ACTIVE = 0
    REQUIRE(state.hiders[1].action_points == 0);  // PLACEHOLDER_START_ACTION_POINTS = 0
    REQUIRE(state.hiders[1].resources[0] == 1);   // LOA_RESOURCE_INDEX = 0, PLACEHOLDER_START_LOA = 1
    
    REQUIRE(state.hiders[2].active == 1);
    REQUIRE(state.hiders[2].status == 0);  // HIDER_STATUS_ACTIVE = 0
    REQUIRE(state.hiders[2].action_points == 0);  // PLACEHOLDER_START_ACTION_POINTS = 0
    REQUIRE(state.hiders[2].resources[0] == 1);   // LOA_RESOURCE_INDEX = 0, PLACEHOLDER_START_LOA = 1
}

TEST_CASE("NPC-031 (range)") {
    hs_test::ServiceFixture fx;
    
    // Test invalid npc_hiders values
    REQUIRE(createWithNpcs(fx, 2, 2).status == 400);      // Too many NPCs
    REQUIRE(createWithNpcs(fx, 2, -1).status == 400);     // Negative value
    REQUIRE(createWithNpcs(fx, 2, "1").status == 400);    // Non-integer
    
    // Test valid case
    const ApiResponse response = createWithNpcs(fx, 2, 0);
    REQUIRE(response.status == 201);
    
    // Slot 1 should be free (null account) when no NPCs
    const auto& match = fx.getMatch().body;
    const auto& hiders = match.at("hiders");
    REQUIRE(hiders[1].at("account").is_null());
}

TEST_CASE("NPC-032 (joining and starting)") {
    hs_test::ServiceFixture fx;
    
    // Create a match with 1 NPC
    createWithNpcs(fx, 2, 1);
    
    // Joining an NPC account should fail
    REQUIRE(fx.join("npc_0").status == 400);
    
    // Starting the match should fail because only NPCs are bound
    REQUIRE(fx.start("host").status == 409);
    
    // Joining a human account should succeed
    const ApiResponse joinResponse = fx.join("alice");
    REQUIRE(joinResponse.status == 200);
    REQUIRE(joinResponse.body.at("hider_id") == 0);
    
    // Joining another human should fail (match full)
    REQUIRE(fx.join("bob").status == 409);
    
    // Starting the match should now succeed
    const ApiResponse startResponse = fx.start("host");
    REQUIRE(startResponse.status == 200);
    REQUIRE(startResponse.body.at("phase") == "ACTIVE");
}

} // namespace hider_seeker