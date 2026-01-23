#include "CodeEnum.hpp"

const char* code_to_text(int index) {
    static const char* names[] = {
        #define MOVEMENT_DECL( name_text, type ) #name_text,
        #include "Code.enum"
        #undef MOVEMENT_DECL
    };
    if (index < 0) {
        return "CODE_INDEX_NEGATIVE";
    }
    if (index >= CODE_COUNT) {
        return "CODE_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}

const CodeType code_to_type(int index) {
    static const CodeType types[] = {
        #define MOVEMENT_DECL( name_text, type ) type,
        #include "Code.enum"
        #undef MOVEMENT_DECL
    };
    if (index < 0) {
        return CodeType::CODE_INDEX_NEGATIVE;
    }
    if (index >= CODE_COUNT) {
        return CodeType::CODE_INDEX_OUT_OF_BOUNDS;
    }
    return types[index];
}

const std::string code_to_message(int code, const std::string prefix) {
    return prefix + code_to_text(code);
}