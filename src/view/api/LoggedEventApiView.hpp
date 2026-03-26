#pragma once

#include "EventFlyweight.hpp"
#include "LoggedEvent.hpp"
#include <nlohmann/json.hpp>
#include <string>

struct LoggedEventApiView
{
    std::string    action;
    nlohmann::json details;

    inline LoggedEventApiView() = default;

    inline LoggedEventApiView(const LoggedEvent& model)
    {
        this->action = event_action_name(model.action);

        auto encodeComponent = [](const EventComponent& c) -> nlohmann::json {
            return nlohmann::json{
                {"typename", event_component_typename(c.kind)},
                {"name",     event_component_name(c)}
            };
        };

        if (!model.actor.isEmpty())  this->details["actor"]     = encodeComponent(model.actor);
        if (!model.tool.isEmpty())   this->details["tool"]      = encodeComponent(model.tool);
        if (!model.target.isEmpty()) this->details["target"]    = encodeComponent(model.target);
        if (model.direction >= 0)    this->details["direction"] = event_resolve_direction(model.direction);
    }
};

inline void to_json(nlohmann::json& j, const LoggedEventApiView& v) {
    j = {{"action", v.action}};
    for (auto& [key, val] : v.details.items()) {
        j[key] = val;
    }
}

inline void from_json(const nlohmann::json&, LoggedEventApiView&) {}
