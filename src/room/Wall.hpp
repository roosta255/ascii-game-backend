#pragma once

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "DoorEnum.hpp"
#include "Keyframe.hpp"
#include <functional>
#include "Timestamp.hpp"

class Codeset;
class DoorFlyweight;
class Player;
class Character;
class JsonParameters;
class Room;
class Match;
class MatchController;
class Cardinal;

struct Wall {
    static constexpr int MAX_KEYFRAMES = 2;

    DoorEnum door = DOOR_WALL;
    int adjacent = -1;
    Array<Keyframe, MAX_KEYFRAMES> keyframes;

    // functions
    bool accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)>) const;
    bool isWalkable(CodeEnum& error) const;
    bool readIsSharedDoorway(CodeEnum& error, bool& isSharedDoorway) const;
    bool setDoor(const DoorEnum& setting, const Timestamp& time, const bool isSkippingAnimation, const int roomId, const AnimationEnum animation);

};

// operators

std::ostream& operator<<(std::ostream& os, const Wall& rhs);
