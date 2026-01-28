#include "LocationEnum.hpp"
#include <unordered_map>

// functions
const char* location_to_text(LocationEnum index) {
    static const char* names[] = {
        #define LOCATION_DECL( name_text ) #name_text,
        #include "Location.enum"
        #undef LOCATION_DECL
    };
    if (index < 0) {
        return "LOCATION_INDEX_NEGATIVE";
    }
    if (index >= LOCATION_COUNT) {
        return "LOCATION_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}

bool text_to_location(const std::string& name, LocationEnum& output) {
    static const auto names = [](){
        std::unordered_map<std::string, LocationEnum> names;
        for(int i = 0; i < LOCATION_COUNT; i++) {
            names[location_to_text(static_cast<LocationEnum>(i))] = static_cast<LocationEnum>(i);
        }
        return names;
    }();
    if (auto search = names.find(name); search != names.end()) {
        output = search->second;
        return true;
    }
    return false;
}

// operators
std::ostream& operator<<(std::ostream& os, LocationEnum code) {
    return os << std::string(location_to_text(code));
}
