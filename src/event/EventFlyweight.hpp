#pragma once

#include "ActionFlyweight.hpp"
#include "Cardinal.hpp"
#include "DamageFlyweight.hpp"
#include "DamageTypeEnum.hpp"
#include "DoorFlyweight.hpp"
#include "EventEnum.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "LoggedEvent.hpp"
#include "RoleFlyweight.hpp"
#include "RoomFlyweight.hpp"

// ---------------------------------------------------------------------------
// Slot type suffix -> C++ type
// ---------------------------------------------------------------------------
#define EVENT_SLOT_TYPE_ROLE      RoleEnum
#define EVENT_SLOT_TYPE_ITEM      ItemEnum
#define EVENT_SLOT_TYPE_DIRECTION Cardinal
#define EVENT_SLOT_TYPE_LOCK      LockEnum
#define EVENT_SLOT_TYPE_DAMAGE    DamageTypeEnum
#define EVENT_SLOT_TYPE_ROOM      RoomEnum
#define EVENT_SLOT_TYPE_ACTION    ActionEnum
#define EVENT_SLOT_TYPE_DOOR      DoorEnum
#define EVENT_SLOT_TYPE_NONE      int

// ---------------------------------------------------------------------------
// Helpers to unpack a (TypeSuffix, Name) tuple
// ---------------------------------------------------------------------------
#define EVENT_SLOT_CPP_TYPE_(type_, name_) EVENT_SLOT_TYPE_##type_
#define EVENT_SLOT_CPP_TYPE(tup_)          EVENT_SLOT_CPP_TYPE_ tup_

#define EVENT_SLOT_RESOLVER_(type_, name_) event_resolve_##type_
#define EVENT_SLOT_RESOLVER(tup_)          EVENT_SLOT_RESOLVER_ tup_

#define EVENT_SLOT_NAME_STR_(type_, name_) #name_
#define EVENT_SLOT_NAME_STR(tup_)          EVENT_SLOT_NAME_STR_ tup_

// ---------------------------------------------------------------------------
// Per-type resolver functions: int -> const char*
// The flyweight data is static, so returned pointers are permanently valid.
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

inline const char* event_resolve_DIRECTION(int v) {
    static const char* dirs[] = { "NORTH", "EAST", "SOUTH", "WEST" };
    return (v >= 0 && v < 4) ? dirs[v] : "";
}

inline const char* event_resolve_LOCK(int v) {
    const char* name = "";
    LockFlyweight::getFlyweights().accessConst(v, [&](const LockFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

inline const char* event_resolve_DAMAGE(int v) {
    const char* name = "";
    DamageFlyweight::getFlyweights().accessConst(v, [&](const DamageFlyweight& fw) { name = fw.name; });
    return name;
}

inline const char* event_resolve_ROOM(int v) {
    const char* name = "";
    RoomFlyweight::getFlyweights().accessConst(v, [&](const RoomFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

inline const char* event_resolve_ACTION(int v) {
    const char* name = "";
    ActionFlyweight::getFlyweights().accessConst(v, [&](const ActionFlyweight& fw) { name = fw.name.c_str(); });
    return name;
}

inline const char* event_resolve_DOOR(int v) {
    const char* name = "";
    DoorFlyweight::getFlyweights().accessConst(v, [&](const DoorFlyweight& fw) { name = fw.name; });
    return name;
}

inline const char* event_resolve_NONE(int) { return ""; }

// ---------------------------------------------------------------------------
// Flyweight metadata struct
// ---------------------------------------------------------------------------
using EventSlotResolver = const char*(*)(int);

struct EventSlotMeta
{
    const char*       name;    // slot field name, e.g. "SUBJECT"
    EventSlotResolver resolve; // converts data[i] int to a display name
};

struct EventFlyweight
{
    const char*    name;
    bool           isSource;
    EventSlotMeta  slots[3];

    static const EventFlyweight& get(EventEnum type)
    {
        static const EventFlyweight table[EVENT_COUNT] = {
            // EVENT_NIL
            { "NIL", false, { {"", event_resolve_NONE}, {"", event_resolve_NONE}, {"", event_resolve_NONE} } },
            // Each EVENT_DECL expands to two consecutive entries: SOURCE then RESULT
#define EVENT_DECL(name_, d0_, d1_, d2_) \
            { #name_ "_SOURCE", true, \
              { { EVENT_SLOT_NAME_STR(d0_), EVENT_SLOT_RESOLVER(d0_) }, \
                { EVENT_SLOT_NAME_STR(d1_), EVENT_SLOT_RESOLVER(d1_) }, \
                { EVENT_SLOT_NAME_STR(d2_), EVENT_SLOT_RESOLVER(d2_) } } }, \
            { #name_ "_RESULT", false, \
              { { EVENT_SLOT_NAME_STR(d0_), EVENT_SLOT_RESOLVER(d0_) }, \
                { EVENT_SLOT_NAME_STR(d1_), EVENT_SLOT_RESOLVER(d1_) }, \
                { EVENT_SLOT_NAME_STR(d2_), EVENT_SLOT_RESOLVER(d2_) } } },
#include "Event.enum"
#undef EVENT_DECL
        };
        return table[type];
    }
};

// ---------------------------------------------------------------------------
// Strongly-typed builder functions.
// Callers always build the SOURCE variant and pass it to appendEventLog().
// ---------------------------------------------------------------------------
#define EVENT_DECL(name_, d0_, d1_, d2_) \
    inline LoggedEvent build_##name_##_SOURCE( \
        EVENT_SLOT_CPP_TYPE(d0_) p0, \
        EVENT_SLOT_CPP_TYPE(d1_) p1, \
        EVENT_SLOT_CPP_TYPE(d2_) p2 \
    ) { \
        return LoggedEvent{ EVENT_##name_##_SOURCE, {(int)p0, (int)p1, (int)p2} }; \
    } \
    inline LoggedEvent build_##name_##_RESULT( \
        EVENT_SLOT_CPP_TYPE(d0_) p0, \
        EVENT_SLOT_CPP_TYPE(d1_) p1, \
        EVENT_SLOT_CPP_TYPE(d2_) p2 \
    ) { \
        return LoggedEvent{ EVENT_##name_##_RESULT, {(int)p0, (int)p1, (int)p2} }; \
    }
#include "Event.enum"
#undef EVENT_DECL

// ---------------------------------------------------------------------------
// Cleanup — don't let these escape the header
// ---------------------------------------------------------------------------
#undef EVENT_SLOT_CPP_TYPE_
#undef EVENT_SLOT_CPP_TYPE
#undef EVENT_SLOT_RESOLVER_
#undef EVENT_SLOT_RESOLVER
#undef EVENT_SLOT_NAME_STR_
#undef EVENT_SLOT_NAME_STR
#undef EVENT_SLOT_TYPE_ROLE
#undef EVENT_SLOT_TYPE_ITEM
#undef EVENT_SLOT_TYPE_DIRECTION
#undef EVENT_SLOT_TYPE_LOCK
#undef EVENT_SLOT_TYPE_DAMAGE
#undef EVENT_SLOT_TYPE_ROOM
#undef EVENT_SLOT_TYPE_ACTION
#undef EVENT_SLOT_TYPE_DOOR
#undef EVENT_SLOT_TYPE_NONE
