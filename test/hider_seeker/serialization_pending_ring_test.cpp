#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "state.hpp"
#include "serialization.hpp"
#include "rule_types.hpp"
#include "probe_rules.hpp"

using namespace hider_seeker;

TEST_CASE("Round-trip MatchState with wrapped pending ring", "[serialization]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.ruleset_version = 1;
    state.revision = 100;
    state.tick = 50;
    state.rng_state = 123456789;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 4;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        state.hiders[i].active = (i < 2);
        state.hiders[i].status = (i == 1) ? HIDER_STATUS_CAPTURED : HIDER_STATUS_ACTIVE;
        state.hiders[i].action_points = 5 + i;
        state.hiders[i].server_id = i;
        for (int j = 0; j < RESOURCE_TYPE_COUNT; ++j) {
            state.hiders[i].resources[j] = 10 + i * j;
        }
        for (int j = 0; j < MAX_GOALS; ++j) {
            state.hiders[i].goal_progress[j] = 2 + i + j;
        }
    }
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.servers[i].active = (i % 2 == 0);
        state.servers[i].owner_hider_id = i % 3;
        state.servers[i].public_flags = i;
        state.servers[i].trace_strength = i + 1;
        state.servers[i].trace_age = i + 2;
        state.servers[i].current.cpu = i + 10;
        state.servers[i].current.memory = i + 20;
        state.servers[i].current.disk = i + 30;
        state.servers[i].current.network = i + 40;
        state.servers[i].current.heat = i + 50;
        state.servers[i].current.errors = i + 60;
        state.servers[i].current.users = i + 70;
        state.servers[i].history_head = i % HISTORY_TICKS;
        for (int j = 0; j < HISTORY_TICKS; ++j) {
            state.servers[i].history[j].cpu = i + j * 10;
            state.servers[i].history[j].memory = i + j * 20;
            state.servers[i].history[j].disk = i + j * 30;
            state.servers[i].history[j].network = i + j * 40;
            state.servers[i].history[j].heat = i + j * 50;
            state.servers[i].history[j].errors = i + j * 60;
            state.servers[i].history[j].users = i + j * 70;
        }
    }
    
    // Initialize seeker
    state.seeker.connection_tokens = 8;
    state.seeker.max_tokens = 12;
    state.seeker.heat = 3;
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.seeker.belief[i] = i + 5;
        state.seeker.target_ids[i] = i + 1;
    }
    state.seeker.target_count = 4;
    state.seeker.target_head = 2;
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.seeker.last_probe_tick[i] = i + 3;
    }
    
    // Initialize goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        state.goals[i].active = (i % 2 == 0);
        state.goals[i].goal_id = i;
        state.goals[i].status = i % 3;
        state.goals[i].reward_type = REWARD;
        state.goals[i].reward_amount = 100 + i;
        state.goals[i].requirement_count = 3;
        for (int j = 0; j < MAX_REQUIREMENTS_PER_GOAL; ++j) {
            state.goals[i].requirements[j].type = j % 7;
            state.goals[i].requirements[j].subject_id = j + i;
            state.goals[i].requirements[j].amount = 50 + j;
            state.goals[i].requirements[j].threshold = 25 + j;
        }
        state.goals[i].completion_action = i % 4;
        state.goals[i].deadline_tick = 1000 + i;
    }
    
    // Initialize events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        state.events[i].type = i % 5;
        state.events[i].tick = 100 + i;
    }
    state.event_head = 3;
    state.event_count = 5;
    
    // Set up pending ring with wraparound
    // Fill the ring with some moves, then wrap around
    for (int i = 0; i < MAX_PENDING_MOVES; ++i) {
        state.pending[i].move_id = i + 1000;
        state.pending[i].submit_seq = i + 2000;
        state.pending[i].hider_id = i % MAX_HIDERS;
        state.pending[i].action_type = (i % 3);
        state.pending[i].server_id = i % MAX_SERVERS;
        state.pending[i].target_id = NO_SERVER_ID;
        state.pending[i].server_mode = i % 4;
    }
    
    // Set up a wraparound scenario
    state.pending_head = 10;  // Start at index 10
    state.pending_count = 16; // Full ring
    state.next_submit_seq = 5000;
    
    // Serialize the state
    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);
    
    // Deserialize it back
    MatchState loaded_state{};
    bool success = loadMatchState(buffer, loaded_state);
    
    REQUIRE(success == true);
    
    // Check that all fields match
    REQUIRE(loaded_state.schema_version == state.schema_version);
    REQUIRE(loaded_state.ruleset_version == state.ruleset_version);
    REQUIRE(loaded_state.revision == state.revision);
    REQUIRE(loaded_state.tick == state.tick);
    REQUIRE(loaded_state.rng_state == state.rng_state);
    REQUIRE(loaded_state.phase == state.phase);
    REQUIRE(loaded_state.hider_count == state.hider_count);
    REQUIRE(loaded_state.server_count == state.server_count);
    
    // Check hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        REQUIRE(loaded_state.hiders[i].active == state.hiders[i].active);
        REQUIRE(loaded_state.hiders[i].status == state.hiders[i].status);
        REQUIRE(loaded_state.hiders[i].action_points == state.hiders[i].action_points);
        REQUIRE(loaded_state.hiders[i].server_id == state.hiders[i].server_id);
        for (int j = 0; j < RESOURCE_TYPE_COUNT; ++j) {
            REQUIRE(loaded_state.hiders[i].resources[j] == state.hiders[i].resources[j]);
        }
        for (int j = 0; j < MAX_GOALS; ++j) {
            REQUIRE(loaded_state.hiders[i].goal_progress[j] == state.hiders[i].goal_progress[j]);
        }
    }
    
    // Check servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(loaded_state.servers[i].active == state.servers[i].active);
        REQUIRE(loaded_state.servers[i].owner_hider_id == state.servers[i].owner_hider_id);
        REQUIRE(loaded_state.servers[i].public_flags == state.servers[i].public_flags);
        REQUIRE(loaded_state.servers[i].trace_strength == state.servers[i].trace_strength);
        REQUIRE(loaded_state.servers[i].trace_age == state.servers[i].trace_age);
        REQUIRE(loaded_state.servers[i].current.cpu == state.servers[i].current.cpu);
        REQUIRE(loaded_state.servers[i].current.memory == state.servers[i].current.memory);
        REQUIRE(loaded_state.servers[i].current.disk == state.servers[i].current.disk);
        REQUIRE(loaded_state.servers[i].current.network == state.servers[i].current.network);
        REQUIRE(loaded_state.servers[i].current.heat == state.servers[i].current.heat);
        REQUIRE(loaded_state.servers[i].current.errors == state.servers[i].current.errors);
        REQUIRE(loaded_state.servers[i].current.users == state.servers[i].current.users);
        REQUIRE(loaded_state.servers[i].history_head == state.servers[i].history_head);
        for (int j = 0; j < HISTORY_TICKS; ++j) {
            REQUIRE(loaded_state.servers[i].history[j].cpu == state.servers[i].history[j].cpu);
            REQUIRE(loaded_state.servers[i].history[j].memory == state.servers[i].history[j].memory);
            REQUIRE(loaded_state.servers[i].history[j].disk == state.servers[i].history[j].disk);
            REQUIRE(loaded_state.servers[i].history[j].network == state.servers[i].history[j].network);
            REQUIRE(loaded_state.servers[i].history[j].heat == state.servers[i].history[j].heat);
            REQUIRE(loaded_state.servers[i].history[j].errors == state.servers[i].history[j].errors);
            REQUIRE(loaded_state.servers[i].history[j].users == state.servers[i].history[j].users);
        }
    }
    
    // Check seeker
    REQUIRE(loaded_state.seeker.connection_tokens == state.seeker.connection_tokens);
    REQUIRE(loaded_state.seeker.max_tokens == state.seeker.max_tokens);
    REQUIRE(loaded_state.seeker.heat == state.seeker.heat);
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(loaded_state.seeker.belief[i] == state.seeker.belief[i]);
        REQUIRE(loaded_state.seeker.target_ids[i] == state.seeker.target_ids[i]);
    }
    REQUIRE(loaded_state.seeker.target_count == state.seeker.target_count);
    REQUIRE(loaded_state.seeker.target_head == state.seeker.target_head);
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(loaded_state.seeker.last_probe_tick[i] == state.seeker.last_probe_tick[i]);
    }
    
    // Check goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        REQUIRE(loaded_state.goals[i].active == state.goals[i].active);
        REQUIRE(loaded_state.goals[i].goal_id == state.goals[i].goal_id);
        REQUIRE(loaded_state.goals[i].status == state.goals[i].status);
        REQUIRE(loaded_state.goals[i].reward_type == state.goals[i].reward_type);
        REQUIRE(loaded_state.goals[i].reward_amount == state.goals[i].reward_amount);
        REQUIRE(loaded_state.goals[i].requirement_count == state.goals[i].requirement_count);
        for (int j = 0; j < MAX_REQUIREMENTS_PER_GOAL; ++j) {
            REQUIRE(loaded_state.goals[i].requirements[j].type == state.goals[i].requirements[j].type);
            REQUIRE(loaded_state.goals[i].requirements[j].subject_id == state.goals[i].requirements[j].subject_id);
            REQUIRE(loaded_state.goals[i].requirements[j].amount == state.goals[i].requirements[j].amount);
            REQUIRE(loaded_state.goals[i].requirements[j].threshold == state.goals[i].requirements[j].threshold);
        }
        REQUIRE(loaded_state.goals[i].completion_action == state.goals[i].completion_action);
        REQUIRE(loaded_state.goals[i].deadline_tick == state.goals[i].deadline_tick);
    }
    
    // Check events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        REQUIRE(loaded_state.events[i].type == state.events[i].type);
        REQUIRE(loaded_state.events[i].tick == state.events[i].tick);
    }
    REQUIRE(loaded_state.event_head == state.event_head);
    REQUIRE(loaded_state.event_count == state.event_count);
    
    // Check pending ring
    REQUIRE(loaded_state.pending_head == state.pending_head);
    REQUIRE(loaded_state.pending_count == state.pending_count);
    REQUIRE(loaded_state.next_submit_seq == state.next_submit_seq);
    
    for (int i = 0; i < MAX_PENDING_MOVES; ++i) {
        REQUIRE(loaded_state.pending[i].move_id == state.pending[i].move_id);
        REQUIRE(loaded_state.pending[i].submit_seq == state.pending[i].submit_seq);
        REQUIRE(loaded_state.pending[i].hider_id == state.pending[i].hider_id);
        REQUIRE(loaded_state.pending[i].action_type == state.pending[i].action_type);
        REQUIRE(loaded_state.pending[i].server_id == state.pending[i].server_id);
        REQUIRE(loaded_state.pending[i].target_id == state.pending[i].target_id);
        REQUIRE(loaded_state.pending[i].server_mode == state.pending[i].server_mode);
    }
}

TEST_CASE("loadMatchState returns false for version-1 header", "[serialization]") {
    MatchState state{};
    state.schema_version = 1;

    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);
    REQUIRE(buffer.size() >= 4);

    // Rewrite the leading little-endian schema-version marker to 1
    buffer[0] = 1;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;

    // Must be rejected without touching the destination state
    MatchState loaded_state{};
    loaded_state.tick = 777;
    loaded_state.pending_count = 3;
    bool success = loadMatchState(buffer, loaded_state);

    REQUIRE(success == false);
    REQUIRE(loaded_state.tick == 777);
    REQUIRE(loaded_state.pending_count == 3);
}

TEST_CASE("loadMatchState returns false for pending_count > MAX_PENDING_MOVES", "[serialization]") {
    MatchState state{};
    state.schema_version = 2;
    state.tick = 5;
    state.pending_count = MAX_PENDING_MOVES + 1; // 17: invalid count

    // Serialize with invalid pending count
    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);

    // Try to load it back - should fail because of invalid pending count,
    // without partially populating the destination
    MatchState loaded_state{};
    loaded_state.tick = 777;
    bool success = loadMatchState(buffer, loaded_state);

    REQUIRE(success == false);
    REQUIRE(loaded_state.tick == 777);
    REQUIRE(loaded_state.pending_count == 0);
}

TEST_CASE("loadMatchState returns false for pending_head >= MAX_PENDING_MOVES", "[serialization]") {
    MatchState state{};
    state.schema_version = 2;
    state.pending_head = MAX_PENDING_MOVES; // Invalid head
    
    // Serialize with invalid pending head
    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);
    
    // Try to load it back - should fail because of invalid pending head
    MatchState loaded_state{};
    bool success = loadMatchState(buffer, loaded_state);
    
    REQUIRE(success == false);
}