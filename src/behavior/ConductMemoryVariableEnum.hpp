#pragma once

#include <ostream>
#include <string>

// ── Per-scope variable counts ─────────────────────────────────────────────────
// Each counting enum includes ConductMemoryVariable.enum once with only that
// scope's macro expanding to +1; the others are silenced. This gives exact
// per-scope counts with no gap/sentinel values in the aggregate index space.

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_) +1
#define MEMORY_NPC_VARIABLE_DECL(name_, type_)
#define MEMORY_FSM_VARIABLE_DECL(name_, type_)
enum { CONDUCT_MEMORY_DUNGEON_VARIABLE_COUNT = 0
#include "ConductMemoryVariable.enum"
};
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
#define MEMORY_NPC_VARIABLE_DECL(name_, type_) +1
#define MEMORY_FSM_VARIABLE_DECL(name_, type_)
enum { CONDUCT_MEMORY_NPC_VARIABLE_COUNT = 0
#include "ConductMemoryVariable.enum"
};
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
#define MEMORY_NPC_VARIABLE_DECL(name_, type_)
#define MEMORY_FSM_VARIABLE_DECL(name_, type_) +1
enum { CONDUCT_MEMORY_FSM_VARIABLE_COUNT = 0
#include "ConductMemoryVariable.enum"
};
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

// ── Aggregate index-space bases ───────────────────────────────────────────────
// Dungeon occupies [BASE, BASE + COUNT), NPC follows immediately, FSM after that.
// With zero dungeon vars, NPC_BASE = 0 — existing variable values are unchanged.
enum {
    CONDUCT_MEMORY_DUNGEON_VARIABLE_BASE = 0,
    CONDUCT_MEMORY_NPC_VARIABLE_BASE     = CONDUCT_MEMORY_DUNGEON_VARIABLE_COUNT,
    CONDUCT_MEMORY_FSM_VARIABLE_BASE     = CONDUCT_MEMORY_NPC_VARIABLE_BASE
                                         + CONDUCT_MEMORY_NPC_VARIABLE_COUNT,
};

// ── Unified aggregate enum (dungeon → NPC → FSM, no gap sentinels) ────────────
// Three include passes — only the active scope's macro emits enumerators.
// CONDUCT_MEMORY_VARIABLE_COUNT remains the "unset" sentinel throughout.

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_) CONDUCT_MEMORY_##name_,
#define MEMORY_NPC_VARIABLE_DECL(name_, type_)
#define MEMORY_FSM_VARIABLE_DECL(name_, type_)
enum ConductMemoryVariableEnum : int {
#include "ConductMemoryVariable.enum"
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
#define MEMORY_NPC_VARIABLE_DECL(name_, type_) CONDUCT_MEMORY_##name_,
#define MEMORY_FSM_VARIABLE_DECL(name_, type_)
#include "ConductMemoryVariable.enum"
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

#define MEMORY_VAR_TYPE_DECL(type_)
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
#define MEMORY_NPC_VARIABLE_DECL(name_, type_)
#define MEMORY_FSM_VARIABLE_DECL(name_, type_) CONDUCT_MEMORY_##name_,
#include "ConductMemoryVariable.enum"
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

    CONDUCT_MEMORY_VARIABLE_COUNT,
};

// ── Type enum — one value per unique type name, declared via MEMORY_VAR_TYPE_DECL ──
// Variable macros are silenced so only the explicit type declarations emit values.
// Place MEMORY_VAR_TYPE_DECL(type_) once before the first variable of each type.

#define MEMORY_VAR_TYPE_DECL(type_)            MEMORY_VAR_TYPE_##type_,
#define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
#define MEMORY_NPC_VARIABLE_DECL(name_, type_)
#define MEMORY_FSM_VARIABLE_DECL(name_, type_)
enum ConductMemoryVariableTypeEnum : int {
#include "ConductMemoryVariable.enum"
    MEMORY_VAR_TYPE_COUNT,
};
#undef MEMORY_VAR_TYPE_DECL
#undef MEMORY_DUNGEON_VARIABLE_DECL
#undef MEMORY_NPC_VARIABLE_DECL
#undef MEMORY_FSM_VARIABLE_DECL

const char* conduct_memory_variable_to_text(ConductMemoryVariableEnum v);
const char* conduct_memory_variable_type_to_text(ConductMemoryVariableTypeEnum t);
ConductMemoryVariableTypeEnum conduct_memory_variable_type(ConductMemoryVariableEnum v);

inline std::ostream& operator<<(std::ostream& os, ConductMemoryVariableEnum v) {
    return os << std::string(conduct_memory_variable_to_text(v));
}
