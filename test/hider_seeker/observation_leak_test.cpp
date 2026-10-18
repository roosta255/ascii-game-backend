#include <catch2/catch_test_macros.hpp>
#include "feature_extract.hpp"
#include "state.hpp"
#include <cstring>

using namespace hider_seeker;

TEST_CASE("ObservationLeakTest: observation does not leak unrelated state", "[observation_leak]") {
    MatchState state1 = {};
    MatchState state2 = {};
    
    // Set up identical base state
    state1.servers[0].active = 1;
    state1.seeker_connection_mask = 1; // Connected to server 0
    state1.servers[0].current.cpu = 50;
    state1.servers[0].history_head = 3;
    state1.servers[0].history[1].cpu = 40;
    state1.servers[0].history[0].cpu = 30;
    
    // Copy the base state for second test
    state2 = state1;
    
    // Test that changing server trace_strength doesn't affect observation
    state1.servers[0].trace_strength = 42;
    state2.servers[0].trace_strength = 100;
    auto obs1 = buildSeekerObservation(state1);
    auto obs2 = buildSeekerObservation(state2);
    
    // We can't directly compare the arrays, so we'll check each element individually
    bool equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing server trace_age doesn't affect observation
    state1 = state2;
    state1.servers[0].trace_age = 42;
    state2.servers[0].trace_age = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing server owner_hider_id doesn't affect observation
    state1 = state2;
    state1.servers[0].owner_hider_id = 42;
    state2.servers[0].owner_hider_id = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing server public_flags doesn't affect observation
    state1 = state2;
    state1.servers[0].public_flags = 42;
    state2.servers[0].public_flags = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing hider's server_id doesn't affect observation
    state1 = state2;
    state1.hiders[0].server_id = 42;
    state2.hiders[0].server_id = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing hider's AP doesn't affect observation
    state1 = state2;
    state1.hiders[0].action_points = 42;
    state2.hiders[0].action_points = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing hider's resources doesn't affect observation
    state1 = state2;
    state1.hiders[0].resources[0] = 42;
    state2.hiders[0].resources[0] = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing hider_connection_pool doesn't affect observation
    state1 = state2;
    state1.hider_connection_pool = 42;
    state2.hider_connection_pool = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing pending move ring doesn't affect observation
    state1 = state2;
    state1.pending[0].hider_id = 42;
    state2.pending[0].hider_id = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing rng_state doesn't affect observation
    state1 = state2;
    state1.rng_state = 42;
    state2.rng_state = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
    
    // Test that changing metrics/history of a server the seeker is NOT connected to doesn't affect observation
    state1 = state2;
    state1.servers[1].current.cpu = 42;  // Server 1 not connected to
    state2.servers[1].current.cpu = 100;
    obs1 = buildSeekerObservation(state1);
    obs2 = buildSeekerObservation(state2);
    
    equal = true;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        Array<int16_t, SEEKER_SERVER_FEATURES> row1, row2;
        obs1.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r1) {
            row1 = r1;
        });
        obs2.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& r2) {
            row2 = r2;
        });
        
        // Use range-based for loop instead of index-based loop
        int i = 0;
        for (const auto& v1 : row1) {
            const auto& v2 = row2.getOrDefault(i, int16_t{0});
            if (v1 != v2) {
                equal = false;
                break;
            }
            ++i;
        }
        if (!equal) break;
    }
    REQUIRE(equal);
}