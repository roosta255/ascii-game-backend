#include "ConductMemoryVariableEnum.hpp"

// Each function uses a single #include pass with all three scope macros defined
// identically, so every variable in the .enum file emits exactly one case.

const char* conduct_memory_variable_to_text(ConductMemoryVariableEnum v) {
    switch (v) {
    #define MEMORY_VAR_TYPE_DECL(type_)
    #define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_) case CONDUCT_MEMORY_##name_: return #name_;
    #define MEMORY_NPC_VARIABLE_DECL(name_, type_)     case CONDUCT_MEMORY_##name_: return #name_;
    #define MEMORY_FSM_VARIABLE_DECL(name_, type_)     case CONDUCT_MEMORY_##name_: return #name_;
    #include "ConductMemoryVariable.enum"
    #undef MEMORY_VAR_TYPE_DECL
    #undef MEMORY_DUNGEON_VARIABLE_DECL
    #undef MEMORY_NPC_VARIABLE_DECL
    #undef MEMORY_FSM_VARIABLE_DECL
    default: return "NIL";
    }
}

const char* conduct_memory_variable_type_to_text(ConductMemoryVariableTypeEnum t) {
    switch (t) {
    #define MEMORY_VAR_TYPE_DECL(type_) case MEMORY_VAR_TYPE_##type_: return #type_;
    #define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_)
    #define MEMORY_NPC_VARIABLE_DECL(name_, type_)
    #define MEMORY_FSM_VARIABLE_DECL(name_, type_)
    #include "ConductMemoryVariable.enum"
    #undef MEMORY_VAR_TYPE_DECL
    #undef MEMORY_DUNGEON_VARIABLE_DECL
    #undef MEMORY_NPC_VARIABLE_DECL
    #undef MEMORY_FSM_VARIABLE_DECL
    default: return "NIL";
    }
}

ConductMemoryVariableTypeEnum conduct_memory_variable_type(ConductMemoryVariableEnum v) {
    switch (v) {
    #define MEMORY_VAR_TYPE_DECL(type_)
    #define MEMORY_DUNGEON_VARIABLE_DECL(name_, type_) case CONDUCT_MEMORY_##name_: return MEMORY_VAR_TYPE_##type_;
    #define MEMORY_NPC_VARIABLE_DECL(name_, type_)     case CONDUCT_MEMORY_##name_: return MEMORY_VAR_TYPE_##type_;
    #define MEMORY_FSM_VARIABLE_DECL(name_, type_)     case CONDUCT_MEMORY_##name_: return MEMORY_VAR_TYPE_##type_;
    #include "ConductMemoryVariable.enum"
    #undef MEMORY_VAR_TYPE_DECL
    #undef MEMORY_DUNGEON_VARIABLE_DECL
    #undef MEMORY_NPC_VARIABLE_DECL
    #undef MEMORY_FSM_VARIABLE_DECL
    default: return MEMORY_VAR_TYPE_COUNT;
    }
}
