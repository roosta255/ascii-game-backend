#pragma once

#include "DoorFlyweight.hpp"
#include "EventEnum.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "LoggedEvent.hpp"
#include "RoleFlyweight.hpp"
#include "TraitFlyweight.hpp"

// ---------------------------------------------------------------------------
// Per-kind name resolvers: int id -> const char*
// ---------------------------------------------------------------------------
inline const char* event_resolve_ROLE(int v) {
    const char* name = "";
    RoleFlyweight::getFlyweights().accessConst(v, [&](const RoleFlyweight& fw) { name = fw.name; });
    return name;
}

inline const char* event_resolve_ITEM(int v) {
    const char* name = "";
    ItemFlyweight::getFlyweights().accessConst(v, [&](const ItemFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

inline const char* event_resolve_DOOR(int v) {
    const char* name = "";
    DoorFlyweight::getFlyweights().accessConst(v, [&](const DoorFlyweight& fw) { name = fw.name; });
    return name;
}

inline const char* event_resolve_TRAIT(int v) {
    const char* name = "";
    TraitFlyweight::getFlyweights().accessConst(v, [&](const TraitFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

inline const char* event_resolve_CHEST_LOCK(int v) {
    const char* name = "";
    LockFlyweight::getFlyweights().accessConst(v, [&](const LockFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

// ---------------------------------------------------------------------------
// Direction resolver: Cardinal int -> "NORTH" / "EAST" / "SOUTH" / "WEST"
// ---------------------------------------------------------------------------
inline const char* event_resolve_direction(int v) {
    static const char* dirs[] = { "NORTH", "EAST", "SOUTH", "WEST" };
    return (v >= 0 && v < 4) ? dirs[v] : "";
}

// ---------------------------------------------------------------------------
// Action name resolver: EventEnum -> string from Event.enum
// ---------------------------------------------------------------------------
inline const char* event_action_name(EventEnum e)
{
    static const char* names[EVENT_COUNT] = {
        "NIL",
#define EVENT_DECL(name_) #name_,
#include "Event.enum"
#undef EVENT_DECL
    };
    return (e >= 0 && e < EVENT_COUNT) ? names[e] : "";
}

// ---------------------------------------------------------------------------
// Component typename string: EventComponentKind -> "ROLE" / "ITEM" / etc.
// ---------------------------------------------------------------------------
inline const char* event_component_typename(EventComponentKind k)
{
    switch (k) {
        case EventComponentKind::ROLE:       return "ROLE";
        case EventComponentKind::ITEM:       return "ITEM";
        case EventComponentKind::DOOR:       return "DOOR";
        case EventComponentKind::TRAIT:      return "TRAIT";
        case EventComponentKind::CHEST_LOCK: return "CHEST_LOCK";
        default:                             return "";
    }
}

// ---------------------------------------------------------------------------
// Component name: dispatches by kind to the appropriate resolver
// ---------------------------------------------------------------------------
inline const char* event_component_name(const EventComponent& c)
{
    switch (c.kind) {
        case EventComponentKind::ROLE:       return event_resolve_ROLE(c.id);
        case EventComponentKind::ITEM:       return event_resolve_ITEM(c.id);
        case EventComponentKind::DOOR:       return event_resolve_DOOR(c.id);
        case EventComponentKind::TRAIT:      return event_resolve_TRAIT(c.id);
        case EventComponentKind::CHEST_LOCK: return event_resolve_CHEST_LOCK(c.id);
        default:                             return "";
    }
}
