#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "Location.hpp"
#include "Maybe.hpp"

bool Keyframe::isAvailable()const{
    Timestamp now;
    return animation <= ANIMATION_NIL || animation >= ANIMATION_COUNT || now >= t1;
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const Cardinal wall1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_WALL_TO_WALL,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall0, (int)wall1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const int floorId1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_WALL_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)wall0, floorId1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const Cardinal wall1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_WALL,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{floorId0, (int)wall1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const int floorId1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{floorId0, floorId1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Location& location0, const Location& location1, Codeset& codeset) {
    const auto coalesceLocation = [&](const Location& location){
        switch(location.type){
            case LOCATION_DOOR:
            case LOCATION_DOOR_SHARED:
                return std::make_pair(Maybe<LocationEnum>(LOCATION_DOOR), room0 != location.roomId ? (location.data + 2) % 4 : location.data);
            case LOCATION_SHAFT_BOTTOM:
            case LOCATION_SHAFT_TOP:
                return std::make_pair(Maybe<LocationEnum>(LOCATION_DOOR), location.data);

            case LOCATION_FLOOR:
                return std::make_pair(Maybe<LocationEnum>(LOCATION_FLOOR), location.data);
        }
        return std::make_pair(Maybe<LocationEnum>(), 0);
    };

    const auto coalesced0 = coalesceLocation(location0);
    if (coalesced0.first.isEmpty()) {
        // invalid location detected
        codeset.addError(CODE_ANIMATION_WALKING_BUT_LOCATION_1_NOT_DOOR_NOR_FLOOR);
        return buildWalking(start, duration, location0.roomId, 0, 1);
    }

    const auto coalesced1 = coalesceLocation(location1);
    if (coalesced1.first.isEmpty()) {
        // invalid location detected
        codeset.addError(CODE_ANIMATION_WALKING_BUT_LOCATION_2_NOT_DOOR_NOR_FLOOR);
        return buildWalking(start, duration, location0.roomId, 0, 1);
    }

    AnimationEnum animation;
    if (coalesced0.first == LOCATION_DOOR) {
        if (coalesced1.first == LOCATION_DOOR) {
            // door -> door
            animation = ANIMATION_WALKING_FROM_WALL_TO_WALL;
        } else if (coalesced1.first == LOCATION_FLOOR) {
            // door -> floor
            animation = ANIMATION_WALKING_FROM_WALL_TO_FLOOR;
        }
    } else if (coalesced0.first == LOCATION_FLOOR) {
        if (coalesced1.first == LOCATION_DOOR) {
            // floor -> door
            animation = ANIMATION_WALKING_FROM_FLOOR_TO_WALL;
        } else if (coalesced1.first == LOCATION_FLOOR) {
            // floor -> floor
            animation = ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR;
        }
    }

    return Keyframe{
        .t0 = start,
        .t1 = start + duration,
        .animation = animation,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{location0.data, coalesced1.second})
    };
}

Keyframe Keyframe::buildHurtling(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_HURTLING,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1]})
    };
}

Keyframe Keyframe::buildSmacking(const Timestamp& start, long duration, const int room0, const int2 xy0, const int2 xy1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_SMACKING,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{xy0[0], xy0[1]})
    };
}

Keyframe Keyframe::buildCasting(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_CASTING,
        .room0 = room0
    };
}

Keyframe Keyframe::buildSleeping(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_SLEEPING,
        .room0 = room0
    };
}

Keyframe Keyframe::buildHurting(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_HURTING,
        .room0 = room0
    };
}

Keyframe Keyframe::buildDying(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_DYING,
        .room0 = room0
    };
}

Keyframe Keyframe::buildJump(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_JUMP,
        .room0 = room0
    };
}

Keyframe Keyframe::buildCritterBite(const Timestamp& start, long duration, const int room0) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_CRITTER_BITE,
        .room0 = room0
    };
}

Keyframe Keyframe::buildBounceLock(const Timestamp& start, long duration, const int room0, const int floorId, const Cardinal direction) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_DOOR_LOCK_BOUNCE_FROM_FLOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{floorId, (int)direction})
    };
}

Keyframe Keyframe::buildBounceLock(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const Cardinal direction) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_DOOR_LOCK_BOUNCE_FROM_DOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)characterDoor, (int)direction})
    };
}

Keyframe Keyframe::buildBounceFloor(const Timestamp& start, long duration, const int room0, const int floorId, const int targetFloorId) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_DOOR_FLOOR_BOUNCE_FROM_FLOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{floorId, targetFloorId})
    };
}

Keyframe Keyframe::buildBounceFloor(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const int targetFloorId) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_DOOR_FLOOR_BOUNCE_FROM_DOOR,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{(int)characterDoor, targetFloorId})
    };
}

static Keyframe buildTraitToggle(const Timestamp& start, long duration, const int room0, const AnimationEnum animation, bool begins) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = animation,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,Keyframe::DATA_ARRAY_SIZE>{begins ? 1 : 0, 0})
    };
}

Keyframe Keyframe::buildSleep(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_SLEEP, begins);
}
Keyframe Keyframe::buildEnchain(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_ENCHAIN, begins);
}
Keyframe Keyframe::buildEncage(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_ENCAGE, begins);
}
Keyframe Keyframe::buildConfuse(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_CONFUSE, begins);
}
Keyframe Keyframe::buildExcite(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_EXCITE, begins);
}
Keyframe Keyframe::buildEnrage(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_ENRAGE, begins);
}
Keyframe Keyframe::buildFrozen(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_FROZEN, begins);
}
Keyframe Keyframe::buildEnflame(const Timestamp& start, long duration, const int room0, bool begins) {
    return buildTraitToggle(start, duration, room0, ANIMATION_ENFLAME, begins);
}

Keyframe Keyframe::buildTransition(const Timestamp& start, long duration, const int room0, const AnimationEnum animation, const int fromType, const int toType) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = animation,
        .room0 = room0,
        .data = Array<int,Keyframe::DATA_ARRAY_SIZE>(std::array<int,DATA_ARRAY_SIZE>{fromType, toType})
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

// TODO: put this as data in enum
bool Keyframe::isMovement(int animation) {
    return animation == ANIMATION_WALKING_FROM_WALL_TO_WALL
        || animation == ANIMATION_WALKING_FROM_WALL_TO_FLOOR
        || animation == ANIMATION_WALKING_FROM_FLOOR_TO_WALL
        || animation == ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR
        || animation == ANIMATION_HURTLING
        || animation == ANIMATION_SMACKING;
}

Timestamp Keyframe::getLatestMovementEnd(Rack<Keyframe> rack, const Timestamp& fallback) {
    Timestamp latest = fallback;
    for (const auto& keyframe: rack) {
        if (isMovement(keyframe.animation) && keyframe.t1 > latest) {
            latest = keyframe.t1;
        }
    }
    return latest;
}
