#pragma once

#include "EventFlyweight.hpp"
#include "LoggedEvent.hpp"
#include <nlohmann/json.hpp>
#include <string>

struct LoggedEventApiView
{
    std::string    event;
    nlohmann::json details;

    inline LoggedEventApiView() = default;

    inline LoggedEventApiView(const LoggedEvent& model)
    {
        const EventFlyweight& fw = EventFlyweight::get(model.type);
        this->event = fw.name;

        for (int i = 0; i < 3; ++i) {
            const EventSlotMeta& slot = fw.slots[i];
            if (slot.name[0] == '\0') continue;
            const char* value = slot.resolve(model.data[i]);
            if (value[0] != '\0') this->details[slot.name] = value;
        }
    }
};

inline void to_json(nlohmann::json& j, const LoggedEventApiView& v) {
    j = {{"event", v.event}};
    for (auto& [key, val] : v.details.items()) {
        j[key] = val;
    }
}

inline void from_json(const nlohmann::json&, LoggedEventApiView&) {}
