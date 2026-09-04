#include "Map.hpp"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

// Covers Map's crash-proof surface: reads return copies, update() mutates a detached copy,
// forEach() snapshots keys and re-looks-up. The point is that no call sequence - including a
// callback that restructures the very Map it is walking - can dangle or corrupt iteration.

TEST_CASE("get / getOr return copies, never a reference into storage", "[map]") {
    Map<int, int> m;
    m.set(1, 10);

    Maybe<int> hit = m.get(1);
    REQUIRE(hit.isPresent());
    REQUIRE(hit.orElse(-1) == 10);

    REQUIRE(m.get(2).isEmpty());
    REQUIRE(m.getOr(2, 99) == 99);
    REQUIRE(m.getOr(1, 99) == 10);

    // Mutating what get() handed back does not touch the Map.
    hit.access([](int& v) { v = 12345; });
    REQUIRE(m.getOr(1, -1) == 10);
}

TEST_CASE("update mutates in place via a detached copy", "[map]") {
    Map<int, int> m;
    m.set(1, 10);

    REQUIRE(m.update(1, [](int& v) { v += 5; }));
    REQUIRE(m.getOr(1, -1) == 15);

    REQUIRE_FALSE(m.update(2, [](int&) { FAIL("callback must not run for an absent key"); }));
}

TEST_CASE("update is safe when the callback restructures the Map", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 20; i++) m.set(i, i);

    // Callback inserts (forcing rehashes) and erases other keys while holding its copy.
    REQUIRE(m.update(0, [&](int& v) {
        v = 500;
        for (int j = 100; j < 140; j++) m.set(j, j);
        for (int j = 1; j < 10; j++) m.erase(j);
    }));
    REQUIRE(m.getOr(0, -1) == 500);            // write-back landed
    REQUIRE(m.size() == 1 + 10 + 40);          // key 0, keys 10..19, keys 100..139
}

TEST_CASE("update skips write-back if the callback removed its own key", "[map]") {
    Map<int, int> m;
    m.set(1, 10);

    REQUIRE_FALSE(m.update(1, [&](int& v) { v = 999; m.erase(1); }));
    REQUIRE_FALSE(m.containsKey(1));           // not resurrected
}

TEST_CASE("forEach visits every key present at entry exactly once", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 8; i++) m.set(i, i * 10);

    std::multiset<int> seen;
    int total = 0;
    m.forEach([&](int k, const int& v) {
        REQUIRE(v == k * 10);
        seen.insert(k);
        total += v;
    });

    REQUIRE(seen.size() == 8);
    for (int i = 0; i < 8; i++) REQUIRE(seen.count(i) == 1);
    REQUIRE(total == 280);
}

TEST_CASE("forEach lets the body erase the current key without corrupting iteration", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 50; i++) m.set(i, i);

    int visited = 0;
    m.forEach([&](int k, const int&) { visited++; m.erase(k); });

    REQUIRE(visited == 50);
    REQUIRE(m.empty());
}

TEST_CASE("forEach lets the body insert freely; new keys are not visited this pass", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 16; i++) m.set(i, i);

    int visits = 0;
    m.forEach([&](int k, const int&) {
        visits++;
        for (int j = 0; j < 4; j++) m.set(1000 + k * 4 + j, 0);   // force rehashes
    });

    REQUIRE(visits == 16);
    REQUIRE(m.size() == 16 + 16 * 4);
}

TEST_CASE("forEach skips keys the body removed before they were reached", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 10; i++) m.set(i, i);

    // Order is unspecified, so the only order-independent claim: the first key seen is
    // visited, and nothing after it (everything else was erased on that first visit).
    std::vector<int> visited;
    m.forEach([&](int k, const int&) {
        visited.push_back(k);
        if (visited.size() == 1)
            for (int i = 0; i < 10; i++) if (i != k) m.erase(i);
    });

    REQUIRE(visited.size() == 1);
}

TEST_CASE("forEach tolerates a full clear() from inside the body", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 20; i++) m.set(i, i);

    int visits = 0;
    m.forEach([&](int, const int&) { visits++; m.clear(); });

    REQUIRE(visits == 1);
    REQUIRE(m.empty());
}

TEST_CASE("forEach with a bool-returning callback stops early on false", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 100; i++) m.set(i, i);

    int visits = 0;
    m.forEach([&](int, const int&) -> bool { return ++visits < 5; });

    REQUIRE(visits == 5);
}

TEST_CASE("forEach hands a copy - writing the argument does nothing; set() is the way", "[map]") {
    Map<int, int> m;
    for (int i = 0; i < 5; i++) m.set(i, i);

    m.forEach([&](int k, const int& v) { m.set(k, v + 100); });

    for (int i = 0; i < 5; i++) REQUIRE(m.getOr(i, -1) == i + 100);
}

TEST_CASE("nested Map: inner walk is safe while the body rewrites the inner map", "[map]") {
    // Mirrors floors[roomId]: Map<int, Map<int2,int>> whose inner map is rewritten mid-walk.
    Map<int, Map<int, int>> outer;
    {
        Map<int, int> inner;
        for (int i = 0; i < 10; i++) inner.set(i, i);
        outer.set(0, inner);
    }

    int visits = 0;
    outer.update(0, [&](Map<int, int>& inner) {
        inner.forEach([&](int k, const int&) {
            visits++;
            inner.set(k, -1);        // rewrite the cell we're on
            inner.set(500 + k, k);   // and grow the map (rehash)
        });
    });

    REQUIRE(visits == 10);
    outer.get(0).access([](const Map<int, int>& inner) {
        for (int i = 0; i < 10; i++) REQUIRE(inner.getOr(i, 999) == -1);
    });
}
