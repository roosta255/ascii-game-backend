#pragma once

#include "Array.hpp"
#include "LoggedEvent.hpp"
#include "Maybe.hpp"
#include "Timestamp.hpp"

class Player;
class Room;
class Match;
class MatchController;
class Codeset;

struct RequestContext {
    Player& player;
    Room& room;

    Match& match;
    Codeset& codeset;
    MatchController& controller;

    Timestamp time;
    Maybe<int> floorId;

    bool isSkippingAnimations = false;
    bool isSkippingLogging = false;

    Array<LoggedEvent, 32> eventLog;
    int eventLogHead = 0;
    int eventLogSize = 0;
};
