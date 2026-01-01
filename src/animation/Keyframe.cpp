#include "Keyframe.hpp"

bool Keyframe::isAvailable()const{
    Timestamp now;
    return animation <= ANIMATION_NIL || animation >= ANIMATION_COUNT || now >= t1;
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const Cardinal wall, const Cardinal wall2) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_WALL_TO_WALL,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall, 0, (int)wall2, 0})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const Cardinal wall, const int2 xy2) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_WALL_TO_FLOOR,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall, -666, xy2[0], xy2[1]})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int2 xy, const Cardinal wall2) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_WALL,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy[0], xy[1], (int)wall2, -666})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int2 xy, const int2 xy2) {
    return Keyframe {
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy[0], xy[1], xy2[0], xy2[1]})
    };
}

Keyframe Keyframe::buildHurtling(const Timestamp& start, long duration, const int2 xy, const int2 xy2) {
    return Keyframe {
        .animation = ANIMATION_HURTLING,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy[0], xy[1], xy2[0], xy2[1]})
    };
}

Keyframe Keyframe::buildSmacking(const Timestamp& start, long duration, const int2 xy, const int2 xy2) {
    return Keyframe {
        .animation = ANIMATION_SMACKING,
        .t0 = start,
        .t1 = start + duration,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy[0], xy[1], xy2[0], xy2[1]})
    };
}

Keyframe Keyframe::buildCasting(const Timestamp& start, long duration) {
    return Keyframe {
        .animation = ANIMATION_CASTING,
        .t0 = start,
        .t1 = start + duration
    };
}

Keyframe Keyframe::buildSleeping(const Timestamp& start, long duration) {
    return Keyframe {
        .animation = ANIMATION_SLEEPING,
        .t0 = start,
        .t1 = start + duration
    };
}

Keyframe Keyframe::buildHurting(const Timestamp& start, long duration) {
    return Keyframe {
        .animation = ANIMATION_HURTING,
        .t0 = start,
        .t1 = start + duration
    };
}

Keyframe Keyframe::buildDying(const Timestamp& start, long duration) {
    return Keyframe {
        .animation = ANIMATION_DYING,
        .t0 = start,
        .t1 = start + duration
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