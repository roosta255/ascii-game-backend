#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/domain/serialization.hpp"

using namespace hider_seeker;

TEST_CASE("hider_seeker serialization round-trip", "[hider_seeker][serialization]") {
    MatchState original;
    
    // Initialize with non-default values
    original.schema_version = 12345;
    original.ruleset_version = 67890;
    original.revision = 9876543210ULL;
    original.tick = 50000;
    original.rng_state = 123456789012345ULL;
    original.phase = PHASE_ACTIVE;
    original.hider_count = 5;
    original.server_count = 12;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        original.hiders[i].active = (i % 2 == 0) ? 1 : 0;
        original.hiders[i].status = static_cast<uint8_t>(i + 10);
        original.hiders[i].action_points = static_cast<uint8_t>(i + 5);
        original.hiders[i].server_id = static_cast<uint8_t>(i * 2);
        for (int j = 0; j < RESOURCE_TYPE_COUNT; ++j) {
            original.hiders[i].resources[j] = static_cast<uint16_t>(i * 10 + j);
        }
        for (int j = 0; j < MAX_GOALS; ++j) {
            original.hiders[i].goal_progress[j] = static_cast<uint8_t>(i + j);
        }
    }
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        original.servers[i].active = (i % 3 == 0) ? 1 : 0;
        original.servers[i].owner_hider_id = static_cast<uint8_t>(i * 3);
        original.servers[i].public_flags = static_cast<uint8_t>(i + 100);
        original.servers[i].trace_strength = static_cast<uint8_t>(i + 20);
        original.servers[i].trace_age = static_cast<uint8_t>(i + 5);
        original.servers[i].current.cpu = static_cast<uint8_t>(i + 1);
        original.servers[i].current.memory = static_cast<uint8_t>(i + 2);
        original.servers[i].current.disk = static_cast<uint8_t>(i + 3);
        original.servers[i].current.network = static_cast<uint8_t>(i + 4);
        original.servers[i].current.heat = static_cast<uint8_t>(i + 5);
        original.servers[i].current.errors = static_cast<uint8_t>(i + 6);
        original.servers[i].current.users = static_cast<uint8_t>(i + 7);
        for (int j = 0; j < HISTORY_TICKS; ++j) {
            original.servers[i].history[j].cpu = static_cast<uint8_t>(i + j + 1);
            original.servers[i].history[j].memory = static_cast<uint8_t>(i + j + 2);
            original.servers[i].history[j].disk = static_cast<uint8_t>(i + j + 3);
            original.servers[i].history[j].network = static_cast<uint8_t>(i + j + 4);
            original.servers[i].history[j].heat = static_cast<uint8_t>(i + j + 5);
            original.servers[i].history[j].errors = static_cast<uint8_t>(i + j + 6);
            original.servers[i].history[j].users = static_cast<uint8_t>(i + j + 7);
        }
        original.servers[i].history_head = static_cast<uint8_t>(i % HISTORY_TICKS);
    }
    
    // Initialize seeker
    original.seeker.connection_tokens = 5;
    original.seeker.max_tokens = 10;
    original.seeker.heat = 25;
    for (int i = 0; i < MAX_SERVERS; ++i) {
        original.seeker.belief[i] = static_cast<uint8_t>(i + 10);
    }
    for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        original.seeker.target_ids[i] = static_cast<uint8_t>(i * 2);
    }
    original.seeker.target_count = 8;
    original.seeker.target_head = 3;
    for (int i = 0; i < MAX_SERVERS; ++i) {
        original.seeker.last_probe_tick[i] = static_cast<uint8_t>(i + 50);
    }
    
    // Initialize goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        original.goals[i].active = (i % 2 == 0) ? 1 : 0;
        original.goals[i].goal_id = static_cast<uint8_t>(i + 1);
        original.goals[i].status = static_cast<uint8_t>(i + 5);
        original.goals[i].reward_type = (i % 2 == 0) ? REWARD : REWARD_NONE;
        original.goals[i].reward_amount = static_cast<uint16_t>(i * 100);
        original.goals[i].requirement_count = static_cast<uint8_t>(i + 2);
        original.goals[i].completion_action = static_cast<uint8_t>(i + 3);
        original.goals[i].deadline_tick = static_cast<uint16_t>(i * 500);
        
        for (int j = 0; j < MAX_REQUIREMENTS_PER_GOAL; ++j) {
            original.goals[i].requirements[j].type = static_cast<uint8_t>(j % 7);
            original.goals[i].requirements[j].subject_id = static_cast<uint8_t>(j + i);
            original.goals[i].requirements[j].amount = static_cast<uint16_t>(j * 10 + i);
            original.goals[i].requirements[j].threshold = static_cast<uint16_t>(j * 20 + i);
        }
    }
    
    // Initialize events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        original.events[i].type = static_cast<uint8_t>(i + 1);
        original.events[i].tick = static_cast<uint32_t>(i * 1000);
    }
    original.event_head = 5;
    original.event_count = 10;
    
    // Serialize and deserialize
    std::vector<uint8_t> buffer;
    saveMatchState(original, buffer);
    
    MatchState roundtripped;
    bool result = loadMatchState(buffer, roundtripped);
    
    REQUIRE(result == true);
    
    // Check all fields match
    REQUIRE(roundtripped.schema_version == original.schema_version);
    REQUIRE(roundtripped.ruleset_version == original.ruleset_version);
    REQUIRE(roundtripped.revision == original.revision);
    REQUIRE(roundtripped.tick == original.tick);
    REQUIRE(roundtripped.rng_state == original.rng_state);
    REQUIRE(roundtripped.phase == original.phase);
    REQUIRE(roundtripped.hider_count == original.hider_count);
    REQUIRE(roundtripped.server_count == original.server_count);
    
    // Check hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        REQUIRE(roundtripped.hiders[i].active == original.hiders[i].active);
        REQUIRE(roundtripped.hiders[i].status == original.hiders[i].status);
        REQUIRE(roundtripped.hiders[i].action_points == original.hiders[i].action_points);
        REQUIRE(roundtripped.hiders[i].server_id == original.hiders[i].server_id);
        for (int j = 0; j < RESOURCE_TYPE_COUNT; ++j) {
            REQUIRE(roundtripped.hiders[i].resources[j] == original.hiders[i].resources[j]);
        }
        for (int j = 0; j < MAX_GOALS; ++j) {
            REQUIRE(roundtripped.hiders[i].goal_progress[j] == original.hiders[i].goal_progress[j]);
        }
    }
    
    // Check servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(roundtripped.servers[i].active == original.servers[i].active);
        REQUIRE(roundtripped.servers[i].owner_hider_id == original.servers[i].owner_hider_id);
        REQUIRE(roundtripped.servers[i].public_flags == original.servers[i].public_flags);
        REQUIRE(roundtripped.servers[i].trace_strength == original.servers[i].trace_strength);
        REQUIRE(roundtripped.servers[i].trace_age == original.servers[i].trace_age);
        REQUIRE(roundtripped.servers[i].current.cpu == original.servers[i].current.cpu);
        REQUIRE(roundtripped.servers[i].current.memory == original.servers[i].current.memory);
        REQUIRE(roundtripped.servers[i].current.disk == original.servers[i].current.disk);
        REQUIRE(roundtripped.servers[i].current.network == original.servers[i].current.network);
        REQUIRE(roundtripped.servers[i].current.heat == original.servers[i].current.heat);
        REQUIRE(roundtripped.servers[i].current.errors == original.servers[i].current.errors);
        REQUIRE(roundtripped.servers[i].current.users == original.servers[i].current.users);
        for (int j = 0; j < HISTORY_TICKS; ++j) {
            REQUIRE(roundtripped.servers[i].history[j].cpu == original.servers[i].history[j].cpu);
            REQUIRE(roundtripped.servers[i].history[j].memory == original.servers[i].history[j].memory);
            REQUIRE(roundtripped.servers[i].history[j].disk == original.servers[i].history[j].disk);
            REQUIRE(roundtripped.servers[i].history[j].network == original.servers[i].history[j].network);
            REQUIRE(roundtripped.servers[i].history[j].heat == original.servers[i].history[j].heat);
            REQUIRE(roundtripped.servers[i].history[j].errors == original.servers[i].history[j].errors);
            REQUIRE(roundtripped.servers[i].history[j].users == original.servers[i].history[j].users);
        }
        REQUIRE(roundtripped.servers[i].history_head == original.servers[i].history_head);
    }
    
    // Check seeker
    REQUIRE(roundtripped.seeker.connection_tokens == original.seeker.connection_tokens);
    REQUIRE(roundtripped.seeker.max_tokens == original.seeker.max_tokens);
    REQUIRE(roundtripped.seeker.heat == original.seeker.heat);
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(roundtripped.seeker.belief[i] == original.seeker.belief[i]);
    }
    for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        REQUIRE(roundtripped.seeker.target_ids[i] == original.seeker.target_ids[i]);
    }
    REQUIRE(roundtripped.seeker.target_count == original.seeker.target_count);
    REQUIRE(roundtripped.seeker.target_head == original.seeker.target_head);
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(roundtripped.seeker.last_probe_tick[i] == original.seeker.last_probe_tick[i]);
    }
    
    // Check goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        REQUIRE(roundtripped.goals[i].active == original.goals[i].active);
        REQUIRE(roundtripped.goals[i].goal_id == original.goals[i].goal_id);
        REQUIRE(roundtripped.goals[i].status == original.goals[i].status);
        REQUIRE(roundtripped.goals[i].reward_type == original.goals[i].reward_type);
        REQUIRE(roundtripped.goals[i].reward_amount == original.goals[i].reward_amount);
        REQUIRE(roundtripped.goals[i].requirement_count == original.goals[i].requirement_count);
        REQUIRE(roundtripped.goals[i].completion_action == original.goals[i].completion_action);
        REQUIRE(roundtripped.goals[i].deadline_tick == original.goals[i].deadline_tick);
        
        for (int j = 0; j < MAX_REQUIREMENTS_PER_GOAL; ++j) {
            REQUIRE(roundtripped.goals[i].requirements[j].type == original.goals[i].requirements[j].type);
            REQUIRE(roundtripped.goals[i].requirements[j].subject_id == original.goals[i].requirements[j].subject_id);
            REQUIRE(roundtripped.goals[i].requirements[j].amount == original.goals[i].requirements[j].amount);
            REQUIRE(roundtripped.goals[i].requirements[j].threshold == original.goals[i].requirements[j].threshold);
        }
    }
    
    // Check events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        REQUIRE(roundtripped.events[i].type == original.events[i].type);
        REQUIRE(roundtripped.events[i].tick == original.events[i].tick);
    }
    REQUIRE(roundtripped.event_head == original.event_head);
    REQUIRE(roundtripped.event_count == original.event_count);
}

TEST_CASE("loadMatchState empty buffer returns false", "[hider_seeker][serialization]") {
    std::vector<uint8_t> empty_buffer;
    MatchState state;
    
    bool result = loadMatchState(empty_buffer, state);
    
    REQUIRE(result == false);
}

TEST_CASE("loadMatchState corrupted schema version returns false", "[hider_seeker][serialization]") {
    std::vector<uint8_t> buffer(4);
    // Write an invalid schema version
    buffer[0] = 0xFF;
    buffer[1] = 0xFF;
    buffer[2] = 0xFF;
    buffer[3] = 0xFF;
    
    MatchState state;
    bool result = loadMatchState(buffer, state);
    
    REQUIRE(result == false);
}