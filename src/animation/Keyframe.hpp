#pragma once

#include "AnimationEnum.hpp"
#include "Array.hpp"
#include "Cardinal.hpp"
#include "Rack.hpp"
#include "Timestamp.hpp"
#include "int2.hpp"

struct Codeset;
struct Location;

struct Keyframe
{
    Timestamp t0 = Timestamp::nil();
    Timestamp t1 = Timestamp::nil();
    static constexpr int DATA_ARRAY_SIZE = 4;
    int animation = ANIMATION_NIL;
    int room0 = 0;
    Array<int, DATA_ARRAY_SIZE> data = {};
    // (walking, hurtling, smacking), (casting, sleeping), (hurting, dying)

    // WALKING_FROM_WALL_TO_WALL: [w0, .., w1, ..]
    // WALKING_FROM_WALL_TO_FLOOR: [w0, .., x1, y1]
    // WALKING_FROM_FLOOR_TO_WALL: [x0, y0, w1, ..]
    // WALKING_FROM_FLOOR_TO_FLOOR: [x0, y0, x1, y1]
    // hurtling: [x0, y0, x1, y1]
    // smacking: [x0, y0, x1, y1]
    // casting: []
    // sleeping: []
    // hurting: []
    // dying: []

    bool isAvailable()const;

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

    static Keyframe buildHurtling(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1);

    static Keyframe buildSmacking(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1);

    static Keyframe buildCasting(const Timestamp& start, long duration, const int room0);

    static Keyframe buildSleeping(const Timestamp& start, long duration, const int room0);

    static Keyframe buildHurting(const Timestamp& start, long duration, const int room0);

    static Keyframe buildDying(const Timestamp& start, long duration, const int room0);

    static bool insertKeyframe(Rack<Keyframe> rack, const Keyframe& insertion);
};