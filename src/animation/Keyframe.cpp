#include "ActivationContext.hpp"
#include "AnimationConfig.hpp"
#include "AnimationFlyweight.hpp"
#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "Location.hpp"
#include "Maybe.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Keyframe Keyframe::removeOffset() const {
    Keyframe result = *this;
    result.t1 = Timestamp::buildTimestamp((int64_t)t1 - (int64_t)t0);
    result.t0 = Timestamp::nil();
    return result;
}

bool Keyframe::isAvailable()const{
    Timestamp now;
    return animation <= ANIMATION_NIL || animation >= ANIMATION_COUNT || now >= t1;
}

Maybe<Keyframe> Keyframe::buildTargetKeyframe(const AnimationConfig& cfg, const ActivationContext& ctx) {
    Array<int, ANIMATION_SEMANTIC_COUNT> semanticData{};
    const auto characterSemantic = ctx.character.location.type == LOCATION_FLOOR ? ANIMATION_SEMANTIC_FROM_FLOOR : ctx.character.location.type == LOCATION_DOOR || ctx.character.location.type == LOCATION_DOOR_SHARED ? ANIMATION_SEMANTIC_FROM_DOOR : ANIMATION_SEMANTIC_NONE;
    const auto targetSemantic = std::visit(overloaded{
            [](const Pointer<Character>& cp) {
                return cp.mapConst<AnimationSemantic>([&](const Character& c){
                    return c.location.type == LOCATION_FLOOR ? ANIMATION_SEMANTIC_TO_FLOOR
                        : c.location.type == LOCATION_DOOR || c.location.type == LOCATION_DOOR_SHARED ? ANIMATION_SEMANTIC_TO_DOOR
                        : ANIMATION_SEMANTIC_NONE;
                }).orElse(ANIMATION_SEMANTIC_NONE);
            },
            [](const DoorTarget&) { return ANIMATION_SEMANTIC_TO_DOOR; },
            [](const LockTarget&) { return ANIMATION_SEMANTIC_TO_LOCK; },
            [](const auto&) { return ANIMATION_SEMANTIC_NONE; },
        }, ctx.targetEntity);

    const auto isFlipping = cfg.act == ANIMATION_ACT_TARGET || cfg.act == ANIMATION_ACT_TARGET_TO_SUBJECT;
    const auto effectiveCharSemantic = isFlipping
        ? animation_semantic_flipped(characterSemantic) : characterSemantic;
    const auto effectiveTargetSemantic = isFlipping
        ? animation_semantic_flipped(targetSemantic) : targetSemantic;

    semanticData.access((int)effectiveCharSemantic, [&](int& val) {
        val = ctx.character.location.data;
    });
    std::visit(overloaded{
        [&](const Pointer<Character>& cp) {
            semanticData.access((int)effectiveTargetSemantic, [&](int& val) {
                cp.accessConst([&](const Character& c){
                    val = c.location.data;
                });
            });
        },
        [&](const DoorTarget&) {
            ctx.direction.accessConst([&](const Cardinal& dir) {
                semanticData.access((int)effectiveTargetSemantic, [&](int& val) { val = (int)dir; });
            });
        },
        [&](const LockTarget&) {
            ctx.direction.accessConst([&](const Cardinal& dir) {
                semanticData.access((int)effectiveTargetSemantic, [&](int& val) { val = (int)dir; });
            });
        },
        [](const auto&) {},
    }, ctx.targetEntity);

    AnimationTypes semantics;
    const auto addSemantic = [&](AnimationSemantic s) {
        if (s != ANIMATION_SEMANTIC_NONE)
            semantics.setIndexOn((size_t)semantic_to_type(s));
    };
    switch (cfg.act) {
        case ANIMATION_ACT_SUBJECT:
            addSemantic(characterSemantic);
            break;
        case ANIMATION_ACT_TARGET:
            addSemantic(animation_semantic_flipped(targetSemantic));
            break;
        case ANIMATION_ACT_SUBJECT_TO_TARGET:
            addSemantic(characterSemantic);
            addSemantic(targetSemantic);
            break;
        case ANIMATION_ACT_TARGET_TO_SUBJECT:
            addSemantic(animation_semantic_flipped(targetSemantic));
            addSemantic(animation_semantic_flipped(characterSemantic));
            break;
    }

    Maybe<Keyframe> result;

    AnimationFlyweight::queryAnimation(cfg.animation, semantics).accessConst([&](const AnimationEnum& animation){
        AnimationFlyweight::getFlyweights().accessConst((int)animation, [&](const AnimationFlyweight& flyweight){
            result = Keyframe{
                .t0 = ctx.time,
                .t1 = ctx.time + flyweight.baseDuration,
                .animation = animation,
                .room0 = ctx.room.roomId,
                .data = flyweight.data.transform([&](const AnimationSemantic& s) {
                    return semanticData[(int)s].orElse(0);
                })
            };
        });
        ctx.codeset.addFailure(!result.isPresent());
    });
    ctx.codeset.addFailure(!result.isPresent());

    return result;
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const Cardinal wall1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_WALL_TO_WALL,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{(int)wall0, (int)wall1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const Cardinal wall0, const int floorId1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_WALL_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{(int)wall0, floorId1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const Cardinal wall1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_WALL,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{floorId0, (int)wall1})
    };
}

Keyframe Keyframe::buildWalking(const Timestamp& start, long duration, const int room0, const int floorId0, const int floorId1) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{floorId0, floorId1})
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
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{location0.data, coalesced1.second})
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
        .animation = ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{floorId, (int)direction})
    };
}

Keyframe Keyframe::buildBounceLock(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const Cardinal direction) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_BOUNCE_FROM_DOOR_TO_LOCK,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{(int)characterDoor, (int)direction})
    };
}

Keyframe Keyframe::buildBounceFloor(const Timestamp& start, long duration, const int room0, const int floorId, const int targetFloorId) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_BOUNCE_FROM_FLOOR_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{floorId, targetFloorId})
    };
}

Keyframe Keyframe::buildBounceFloor(const Timestamp& start, long duration, const int room0, const Cardinal characterDoor, const int targetFloorId) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = ANIMATION_BOUNCE_FROM_DOOR_TO_FLOOR,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{(int)characterDoor, targetFloorId})
    };
}

static Keyframe buildTraitToggle(const Timestamp& start, long duration, const int room0, const AnimationEnum animation, bool begins) {
    return Keyframe {
        .t0 = start,
        .t1 = start + duration,
        .animation = animation,
        .room0 = room0,
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{begins ? 1 : 0, 0})
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
        .data = Array<int,KEYFRAME_DATA_ARRAY_SIZE>(std::array<int,KEYFRAME_DATA_ARRAY_SIZE>{fromType, toType})
    };
}

bool Keyframe::insertKeyframe(Rack<Keyframe> rack, const Keyframe& input) {
    Keyframe insertion = input;

    // find oldest translation
    Maybe<Timestamp> foundTranslationTime;
    for (const auto& keyframe: rack) {
        AnimationFlyweight::getFlyweights().accessConst(keyframe.animation, [&](const AnimationFlyweight& flyweight){
            if (flyweight.types[ANIMATION_TYPE_IS_TRANSLATING].orElse(false)) {
                foundTranslationTime = keyframe.t1 > foundTranslationTime.orElse(Timestamp::nil()) ? keyframe.t1 : foundTranslationTime;
            }
        });
    }

    // use the oldest translation to sequence the start time of input translations
    foundTranslationTime.accessConst([&](const Timestamp& translationTime){
        AnimationFlyweight::getFlyweights().accessConst(insertion.animation, [&](const AnimationFlyweight& flyweight){
            if (flyweight.types[ANIMATION_TYPE_IS_TRANSLATING].orElse(false) && translationTime > insertion.t0) {
                const auto duration = insertion.t1 - insertion.t0;
                insertion.t0 = translationTime;
                insertion.t1 = translationTime + duration;
            }
        });
    });

    Pointer<Keyframe> evict;
    for (auto& keyframe: rack) {
        if (keyframe.isAvailable()) {
            keyframe = insertion;
            return true;
        }
        const auto evictionTime = evict.map<Timestamp>([&](const Keyframe& keyframeEviction){return keyframeEviction.t0;});
        if (keyframe.t0 < evictionTime.orElse(Timestamp::max())) {
            evict = keyframe;
        }
    }

    evict.access([&](Keyframe& keyframe){
        keyframe = insertion;
    });

    return evict.isPresent();
}

std::ostream& operator<<(std::ostream& os, const Keyframe& kf) {
    os << "Keyframe{ animation=" << (AnimationEnum)kf.animation
       << ", room0=" << kf.room0
       << ", t0=" << (int64_t)kf.t0
       << ", t1=" << (int64_t)kf.t1
       << ", data=[";
    for (int i = 0; i < KEYFRAME_DATA_ARRAY_SIZE; i++) {
        if (i > 0) os << ", ";
        os << kf.data.begin()[i];
    }
    os << "] }";
    return os;
}
