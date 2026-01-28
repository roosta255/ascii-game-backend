#include "ChannelEnum.hpp"
#include <unordered_map>

// functions
const char* channel_to_text(ChannelEnum index) {
    static const char* names[] = {
        #define CHANNEL_DECL( name_text ) #name_text,
        #include "Channel.enum"
        #undef CHANNEL_DECL
    };
    if (index < 0) {
        return "CHANNEL_INDEX_NEGATIVE";
    }
    if (index >= CHANNEL_COUNT) {
        return "CHANNEL_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}

bool text_to_channel(const std::string& name, ChannelEnum& output) {
    static const auto names = [](){
        std::unordered_map<std::string, ChannelEnum> names;
        for(int i = 0; i < CHANNEL_COUNT; i++) {
            names[channel_to_text(static_cast<ChannelEnum>(i))] = static_cast<ChannelEnum>(i);
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
std::ostream& operator<<(std::ostream& os, ChannelEnum code) {
    return os << std::string(channel_to_text(code));
}
