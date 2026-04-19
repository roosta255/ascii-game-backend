#pragma once

#include <ostream>
#include "AnimationEnum.hpp"
#include "Array.hpp"
#include "Cardinal.hpp"
#include "Rack.hpp"
#include "Timestamp.hpp"
#include "int2.hpp"

struct ActivationContext;
struct AnimationConfig;
struct Codeset;
struct Location;

struct Keyframe
{
    Timestamp t0 = Timestamp::nil();
    Timestamp t1 = Timestamp::nil();
    int animation = ANIMATION_NIL;
    int room0 = 0;
    Array<int, KEYFRAME_DATA_ARRAY_SIZE> data = {0,0};

    bool operator==(const Keyframe& rhs) const {
        return t0 == rhs.t0 && t1 == rhs.t1 && animation == rhs.animation && room0 == rhs.room0
            && is_equal(data.begin(), data.end(), rhs.data.begin());
    }

    // Returns a copy with t0 zeroed and t1 set to the duration (t1 - t0).
    Keyframe removeOffset() const;

    bool isAvailable()const;

    static Maybe<Keyframe> buildTargetKeyframe(const AnimationConfig& cfg, const ActivationContext& ctx);

    // coincidentally, the api guarantees movement only ever happens the room in the parameters.
    // wall -> wall, character passed 3 rooms, only room0 renders movement.
    static Keyframe buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const Cardinal wall1);
    // wall -> floor, character passes 2 rooms, only room0 renders movement.
    static Keyframe buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const int floorId1);
    // floor -> wall, character passes 2 rooms, only room0 renders movement.
    static Keyframe buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const Cardinal wall1);
    // floor -> floor, character passes 1 room, only room0 renders movement.
    static Keyframe buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const int floorId1);

    static Keyframe buildWalking(const Timestamp& start, long duration, const int room0, const Location& location0, const Location& location1, Codeset& codeset);

    static Keyframe buildJump(const Timestamp& start, long duration, const int room0);

    static Keyframe buildCritterBite(const Timestamp& start, long duration, const int room0);

    // DOOR_LOCK_BOUNCE_FROM_FLOOR: data[0] = floorId, data[1] = target direction
    static Keyframe buildBounceLock(const Timestamp& start, long duration, const int room0, const int floorId, const Cardinal direction);
    // DOOR_LOCK_BOUNCE_FROM_DOOR: data[0] = character's door direction, data[1] = target direction
    static Keyframe buildBounceLock(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const Cardinal direction);

    // DOOR_FLOOR_BOUNCE_FROM_FLOOR: data[0] = origin floorId, data[1] = target floorId (cellId)
    static Keyframe buildBounceFloor(const Timestamp& start, long duration, const int room0, const int floorId, const int targetFloorId);
    // DOOR_FLOOR_BOUNCE_FROM_DOOR: data[0] = character's door direction, data[1] = target floorId (cellId)
    static Keyframe buildBounceFloor(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const int targetFloorId);

    // Trait-toggle animations: begins=true sets data[0]=1 (effect starts), begins=false sets data[0]=0 (effect ends)
    static Keyframe buildSleep(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildEnchain(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildEncage(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildConfuse(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildExcite(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildEnrage(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildFrozen(const Timestamp& start, long duration, const int room0, bool begins);
    static Keyframe buildEnflame(const Timestamp& start, long duration, const int room0, bool begins);

    // TRANSITION: [fromType, toType, 0, 0]
    static Keyframe buildTransition(const Timestamp& start, long duration, const int room0, const AnimationEnum animation, const int fromType, const int toType);

    static bool insertKeyframe(Rack<Keyframe> rack, const Keyframe& insertion);
};

std::ostream& operator<<(std::ostream& os, const Keyframe& kf);