#include "Keyframe.hpp"

bool Keyframe::isAvailable()const{
    Timestamp now;
    return animation <= ANIMATION_NIL || animation >= ANIMATION_COUNT || now >= t1;
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const Cardinal wall1) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_WALL_TO_WALL,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall0, 0, (int)wall1, 0})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const int2 xy1) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_WALL_TO_FLOOR,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall0, 0, xy1[0], xy1[1]})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int2 xy0, const Cardinal wall1) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_WALL,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1], (int)wall1, 0})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1], xy1[0], xy1[1]})
    };
}

Keyframe Keyframe::buildHurtling(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1) {
    return Keyframe {
        .animation = ANIMATION_HURTLING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1], xy1[0], xy1[1]})
    };
}

Keyframe Keyframe::buildSmacking(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1) {
    return Keyframe {
        .animation = ANIMATION_SMACKING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1], xy1[0], xy1[1]})
    };
}

Keyframe Keyframe::buildCasting(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .animation = ANIMATION_CASTING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0
    };
}

Keyframe Keyframe::buildSleeping(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .animation = ANIMATION_SLEEPING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0
    };
}

Keyframe Keyframe::buildHurting(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .animation = ANIMATION_HURTING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0
    };
}

Keyframe Keyframe::buildDying(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .animation = ANIMATION_DYING,
        .t0 = start,
        .t1 = start + duration,
        .room0 = room0
    };
}

bool Keyframe::insertKeyframe(Rack<Keyframe> rack, const Keyframe& insertion) {
    for (auto& keyframe: rack) {
        if (keyframe.isAvailable()) {
            keyframe = insertion;
            return true;
        }
    }
    return false;
}