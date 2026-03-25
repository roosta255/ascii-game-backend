#pragma once

#include "EventEnum.hpp"

struct LoggedEvent
{
    EventEnum type = EVENT_NIL;
    int data[3] = {0, 0, 0};
};
