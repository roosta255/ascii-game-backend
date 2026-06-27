#pragma once

#include <array>
#include <map>
#include <optional>
#include <sstream>

#include <catch2/catch_all.hpp>

#include "BehaviorEnum.hpp"
#include "Conduct.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"

// ── conduct_to_text ───────────────────────────────────────────────────────────

inline const char* conduct_to_text(ConductEnum c) {
    switch (c) {
    #define CONDUCT_DECL(name_) case CONDUCT_##name_: return #name_;
    #include "Conduct.enum"
    #undef CONDUCT_DECL
    default: return "CONDUCT_UNKNOWN";
    }
}

// ── ConductExpect ─────────────────────────────────────────────────────────────
// Sparse specification: unspecified fields are wildcards that always pass.
// Build one with chained setters or direct aggregate initialization.

struct ConductExpect {
    // When set, asserts conduct.memory[slot].state equals the value.
    // Indexed by ConductEnum (CONDUCT_NIL entry is ignored).
    std::array<std::optional<BehaviorEnum>, CONDUCT_COUNT> memoryState = {};

    // When a key is present, asserts conduct.get(var) equals the mapped value.
    std::map<ConductMemoryVariableEnum, int> vars;

    // When set, asserts conduct.characterId equals the value.
    std::optional<int> characterId;

    ConductExpect& expectState(ConductEnum slot, BehaviorEnum state) {
        memoryState[slot] = state;
        return *this;
    }
    ConductExpect& expectVar(ConductMemoryVariableEnum var, int value) {
        vars[var] = value;
        return *this;
    }
    ConductExpect& expectCharId(int id) {
        characterId = id;
        return *this;
    }
};

// ── Full-state dump ───────────────────────────────────────────────────────────

inline std::string formatConduct(const Conduct& c) {
    std::ostringstream ss;
    ss << "Conduct{\n";
    ss << "  characterId = " << c.characterId << "\n";
    ss << "  memory:\n";
    for (int i = CONDUCT_NIL + 1; i < CONDUCT_COUNT; i++) {
        const ConductEnum slot = ConductEnum(i);
        c.memory.accessConst(slot, [&](const ConductMemory& mem) {
            ss << "    [" << conduct_to_text(slot) << "] = "
               << behavior_to_text(mem.state) << "\n";
        });
    }
    ss << "  vars:\n";
    for (int i = 0; i < CONDUCT_MEMORY_NPC_VARIABLE_COUNT; i++) {
        const ConductMemoryVariableEnum var =
            ConductMemoryVariableEnum(CONDUCT_MEMORY_NPC_VARIABLE_BASE + i);
        ss << "    " << conduct_memory_variable_to_text(var)
           << " = " << c.get(var) << "\n";
    }
    ss << "}";
    return ss.str();
}

// Let Catch2 print the full dump whenever a Conduct appears in a REQUIRE_THAT expansion.
namespace Catch {
template<>
struct StringMaker<Conduct> {
    static std::string convert(const Conduct& c) { return formatConduct(c); }
};
} // namespace Catch

// ── Catch2 Matcher ────────────────────────────────────────────────────────────

class ConductMatcher : public Catch::Matchers::MatcherBase<Conduct> {
    ConductExpect _expect;
public:
    explicit ConductMatcher(ConductExpect expect) : _expect(std::move(expect)) {}

    bool match(const Conduct& actual) const override {
        if (_expect.characterId && actual.characterId != *_expect.characterId)
            return false;
        for (int i = CONDUCT_NIL + 1; i < CONDUCT_COUNT; i++) {
            if (!_expect.memoryState[i]) continue;
            bool failed = false;
            actual.memory.accessConst(ConductEnum(i), [&](const ConductMemory& mem) {
                failed = (mem.state != *_expect.memoryState[i]);
            });
            if (failed) return false;
        }
        for (const auto& [var, expectedVal] : _expect.vars) {
            if (actual.get(var) != expectedVal)
                return false;
        }
        return true;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "satisfies ConductExpect{\n";
        if (_expect.characterId)
            ss << "  characterId = " << *_expect.characterId << "\n";
        for (int i = CONDUCT_NIL + 1; i < CONDUCT_COUNT; i++) {
            if (!_expect.memoryState[i]) continue;
            ss << "  memory[" << conduct_to_text(ConductEnum(i)) << "] = "
               << behavior_to_text(*_expect.memoryState[i]) << "\n";
        }
        for (const auto& [var, val] : _expect.vars)
            ss << "  var[" << conduct_memory_variable_to_text(var) << "] = " << val << "\n";
        ss << "}";
        return ss.str();
    }
};

inline ConductMatcher MatchesConductExpect(ConductExpect expect) {
    return ConductMatcher(std::move(expect));
}
