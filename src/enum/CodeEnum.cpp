#include "CodeEnum.hpp"
#include <sstream>
#include <unordered_map>

#include "BehaviorEnum.hpp"
#include "ConductEnum.hpp"
#include "DoorEnum.hpp"
#include "ItemEnum.hpp"
#include "LocationEnum.hpp"
#include "RoleEnum.hpp"
#include "RoomEnum.hpp"

const char* code_to_text(int index) {
    static const char* names[] = {
        #define MOVEMENT_DECL( name_text, type, valueType ) #name_text,
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
        #define MOVEMENT_DECL( name_text, type, valueType ) type,
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

ConductMemoryVariableTypeEnum code_to_value_type(int index) {
    static const ConductMemoryVariableTypeEnum types[] = {
        #define MOVEMENT_DECL( name_text, type, valueType ) MEMORY_VAR_TYPE_##valueType,
        #include "Code.enum"
        #undef MOVEMENT_DECL
    };
    if (index < 0 || index >= CODE_COUNT) {
        return MEMORY_VAR_TYPE_COUNT;
    }
    return types[index];
}

const std::string code_to_message(int code, const std::string prefix) {
    return prefix + code_to_text(code);
}

// Renders a code's stored table/log value using the underlying type declared
// in Code.enum's third column, so e.g. a DoorEnum value prints its name
// instead of a bare integer.
const std::string code_to_value_text(int code, int value) {
    switch (code_to_value_type(code)) {
    case MEMORY_VAR_TYPE_BOOL:
        return value ? "true" : "false";
    case MEMORY_VAR_TYPE_ROLE_ENUM: {
        std::ostringstream ss;
        ss << static_cast<RoleEnum>(value);
        return ss.str();
    }
    case MEMORY_VAR_TYPE_DOOR_ENUM: {
        std::ostringstream ss;
        ss << static_cast<DoorEnum>(value);
        return ss.str();
    }
    case MEMORY_VAR_TYPE_ROOM_ENUM: {
        std::ostringstream ss;
        ss << static_cast<RoomEnum>(value);
        return ss.str();
    }
    case MEMORY_VAR_TYPE_CONDUCT_ENUM: {
        std::ostringstream ss;
        ss << static_cast<ConductEnum>(value);
        return ss.str();
    }
    case MEMORY_VAR_TYPE_ITEM_ENUM:
        return item_to_text(value);
    case MEMORY_VAR_TYPE_BEHAVIOR_ENUM:
        return behavior_to_text(value);
    case MEMORY_VAR_TYPE_LOCATION_ENUM:
        return location_to_text(static_cast<LocationEnum>(value));
    default:
        return std::to_string(value);
    }
}

bool code_from_text(const std::string& name, CodeEnum& output) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, CodeEnum> nameMap;
        for (int i = 0; i < CODE_COUNT; i++) nameMap[code_to_text(i)] = CodeEnum(i);
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}