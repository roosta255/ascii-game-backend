#include <catch2/catch_test_macros.hpp>
#include "server_metrics.hpp"
#include "state.hpp"
#include <string>

using namespace hider_seeker;

TEST_CASE("ServerMetric.enum generates eight metrics named CPU through TRACE", "[metrics]") {
    REQUIRE(SERVER_METRIC_COUNT == 8);
    const char* expected[] = {"CPU", "MEMORY", "DISK", "NETWORK", "HEAT", "ERRORS", "USERS", "TRACE"};
    // The index is the ServerMetric whose name is checked.
    for (int index = 0; index < SERVER_METRIC_COUNT; ++index) {
        const char* name = serverMetricName(static_cast<ServerMetric>(index));
        REQUIRE(name != nullptr);
        REQUIRE(std::string(name) == expected[index]);
    }
    const char* missing = serverMetricName(SERVER_METRIC_COUNT);
    REQUIRE(missing == nullptr);
}

TEST_CASE("every ServerMetricDef matches the placeholder table", "[metrics]") {
    // The index is the ServerMetric whose definition is checked.
    for (int index = 0; index < SERVER_METRIC_COUNT; ++index) {
        const ServerMetric metric = static_cast<ServerMetric>(index);
        const ServerMetricDef& def = serverMetricDef(metric);
        REQUIRE(def.min == 0);
        REQUIRE(def.max == 100);
        REQUIRE(def.rest == 0);
        REQUIRE(def.drift_per_tick == (metric == SERVER_METRIC_TRACE ? 2 : 1));
    }
}

TEST_CASE("serverMetricField points at the matching ServerState field", "[metrics]") {
    ServerState server = {};

    REQUIRE(serverMetricField(server, SERVER_METRIC_ERRORS).access([](uint8_t& value) { value = 42; }));
    REQUIRE(server.current.errors == 42);

    REQUIRE(serverMetricField(server, SERVER_METRIC_TRACE).access([](uint8_t& value) { value = 17; }));
    REQUIRE(server.trace_strength == 17);

    // Neither write leaked into another field.
    REQUIRE(server.current.cpu == 0);
    REQUIRE(server.current.users == 0);
}

TEST_CASE("applyMetricDelta clamps into the caller-supplied range", "[metrics]") {
    const ServerMetricDef def{10, 50, 10, 0};
    ServerState server = {};

    server.current.cpu = 45;
    applyMetricDelta(server, SERVER_METRIC_CPU, 10, def);
    REQUIRE(server.current.cpu == 50); // 45 + 10 = 55, clamped to max 50

    server.current.cpu = 15;
    applyMetricDelta(server, SERVER_METRIC_CPU, -10, def);
    REQUIRE(server.current.cpu == 10); // 15 - 10 = 5, clamped to min 10

    server.current.cpu = 30;
    applyMetricDelta(server, SERVER_METRIC_CPU, 0, def);
    REQUIRE(server.current.cpu == 30); // 30 + 0
}

TEST_CASE("driftMetric moves toward rest without passing it", "[metrics]") {
    const ServerMetricDef def{0, 100, 20, 3};
    ServerState server = {};

    server.current.heat = 30;
    driftMetric(server, SERVER_METRIC_HEAT, def);
    REQUIRE(server.current.heat == 27); // 30 - 3

    server.current.heat = 21;
    driftMetric(server, SERVER_METRIC_HEAT, def);
    REQUIRE(server.current.heat == 20); // 21 - 3 would pass rest 20, so stops at 20

    server.current.heat = 20;
    driftMetric(server, SERVER_METRIC_HEAT, def);
    REQUIRE(server.current.heat == 20); // already at rest

    server.current.heat = 10;
    driftMetric(server, SERVER_METRIC_HEAT, def);
    REQUIRE(server.current.heat == 13); // 10 + 3
}

TEST_CASE("driftMetric with zero drift leaves the value unchanged", "[metrics]") {
    const ServerMetricDef def{0, 100, 20, 0};
    ServerState server = {};
    server.current.memory = 70;
    driftMetric(server, SERVER_METRIC_MEMORY, def);
    REQUIRE(server.current.memory == 70);
}

TEST_CASE("driftMetric clamps an out-of-range value from an old match", "[metrics]") {
    const ServerMetricDef def{0, 100, 0, 1};
    ServerState server = {};
    server.current.network = 200;
    driftMetric(server, SERVER_METRIC_NETWORK, def);
    REQUIRE(server.current.network == 100); // 200 - 1 = 199, clamped to max 100
}
