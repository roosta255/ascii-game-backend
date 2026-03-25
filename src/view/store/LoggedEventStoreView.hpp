#pragma once

#include "LoggedEvent.hpp"
#include "EventEnum.hpp"
#include <nlohmann/json.hpp>

struct LoggedEventStoreView
{
    int type = 0;
    int data[3] = {0, 0, 0};

    inline LoggedEventStoreView() = default;

    inline LoggedEventStoreView(const LoggedEvent& model)
    : type((int)model.type)
    {
        data[0] = model.data[0];
        data[1] = model.data[1];
        data[2] = model.data[2];
    }

    inline operator LoggedEvent() const
    {
        return LoggedEvent{
            (EventEnum)type,
            {data[0], data[1], data[2]}
        };
    }
};

inline void to_json(nlohmann::json& j, const LoggedEventStoreView& v) {
    j = {
        {"type", v.type},
        {"data", {v.data[0], v.data[1], v.data[2]}}
    };
}

inline void from_json(const nlohmann::json& j, LoggedEventStoreView& v) {
    j.at("type").get_to(v.type);
    const auto& arr = j.at("data");
    v.data[0] = arr[0];
    v.data[1] = arr[1];
    v.data[2] = arr[2];
}
