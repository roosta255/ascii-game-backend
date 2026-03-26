#pragma once

#include "EventEnum.hpp"

// ---------------------------------------------------------------------------
// The "typename" for actor/tool/target slots.
// Corresponds to: role / item / door / trait / chest-lock.
// ---------------------------------------------------------------------------
enum class EventComponentKind
{
    NONE,
    ROLE,
    ITEM,
    DOOR,
    TRAIT,
    CHEST_LOCK
};

struct EventComponent
{
    EventComponentKind kind = EventComponentKind::NONE;
    int                id   = 0;

    bool isEmpty() const { return kind == EventComponentKind::NONE; }
};

// ---------------------------------------------------------------------------
// A single semantic event recorded in the room log.
// actor/tool/target each carry their own typename so no external schema is
// needed to interpret them.
// ---------------------------------------------------------------------------
struct LoggedEvent
{
    EventEnum      action    = EVENT_NIL;
    EventComponent actor;
    EventComponent tool;
    EventComponent target;
    int            direction = -1;   // Cardinal int value; -1 = absent
};
