#pragma once

#include "EventEnum.hpp"
#include "LoggedEvent.hpp"
#include <nlohmann/json.hpp>

struct EventComponentView
{
    int kind = 0;
    int id   = 0;
};

struct LoggedEventStoreView
{
    int                action    = 0;
    EventComponentView actor;
    EventComponentView tool;
    EventComponentView target;
    int                direction = -1;

    inline LoggedEventStoreView() = default;

    inline LoggedEventStoreView(const LoggedEvent& model)
    : action((int)model.action)
    , actor{ (int)model.actor.kind, model.actor.id }
    , tool{ (int)model.tool.kind, model.tool.id }
    , target{ (int)model.target.kind, model.target.id }
    , direction(model.direction)
    {}

    inline operator LoggedEvent() const
    {
        LoggedEvent e;
        e.action    = (EventEnum)action;
        e.actor     = { (EventComponentKind)actor.kind,  actor.id  };
        e.tool      = { (EventComponentKind)tool.kind,   tool.id   };
        e.target    = { (EventComponentKind)target.kind, target.id };
        e.direction = direction;
        return e;
    }
};

inline void to_json(nlohmann::json& j, const EventComponentView& c) {
    j = {{"kind", c.kind}, {"id", c.id}};
}

inline void from_json(const nlohmann::json& j, EventComponentView& c) {
    j.at("kind").get_to(c.kind);
    j.at("id").get_to(c.id);
}

inline void to_json(nlohmann::json& j, const LoggedEventStoreView& v) {
    j = {
        {"action",    v.action},
        {"actor",     v.actor},
        {"tool",      v.tool},
        {"target",    v.target},
        {"direction", v.direction}
    };
}

inline void from_json(const nlohmann::json& j, LoggedEventStoreView& v) {
    j.at("action").get_to(v.action);
    j.at("actor").get_to(v.actor);
    j.at("tool").get_to(v.tool);
    j.at("target").get_to(v.target);
    j.at("direction").get_to(v.direction);
}
