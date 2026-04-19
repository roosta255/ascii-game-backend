#include "ActivationAddTargetKeyframe.hpp"
#include "AnimationConfig.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Item.hpp"
#include "Keyframe.hpp"
#include "Rack.hpp"
#include "RequestContext.hpp"
#include "TargetEntity.hpp"
#include "Wall.hpp"

bool ActivationAddTargetKeyframe::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        if (req.isSkippingAnimations) { result = true; return; }

        if (cfg.isDebug) {
            activation.codeset.addLog(CODE_ADD_KEYFRAME_SUBJECT, activation.character.characterId);
            std::visit([&](auto& t) {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, Pointer<Character>>) {
                    t.accessConst([&](const Character& c) {
                        activation.codeset.addLog(CODE_ADD_KEYFRAME_TARGET, c.characterId);
                    });
                }
            }, activation.targetEntity);
        }

        const auto kf = Keyframe::buildTargetKeyframe(cfg, activation);
        if (activation.codeset.addFailure(!kf.isPresent(), CODE_ACTIVATION_ADD_TARGET_KEYFRAME_FAILED_TO_BUILD)) return;

        const bool insertIntoTarget = cfg.act == ANIMATION_ACT_TARGET || cfg.act == ANIMATION_ACT_TARGET_TO_SUBJECT;

        bool inserted = false;
        kf.accessConst([&](const Keyframe& k) {
            if (insertIntoTarget) {
                std::visit([&](auto& t) {
                    using T = std::decay_t<decltype(t)>;
                    if constexpr (std::is_same_v<T, Pointer<Character>>) {
                        t.access([&](Character& c) {
                            if (cfg.isDebug) activation.codeset.addLog(CODE_ADD_KEYFRAME_INSERTION_CHARACTER, c.characterId);
                            inserted = Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(c.keyframes), k);
                        });
                    } else if constexpr (std::is_same_v<T, Pointer<Item>>) {
                        t.access([&](Item& item) {
                            inserted = Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Item::MAX_KEYFRAMES>(item.keyframes), k);
                        });
                    } else if constexpr (std::is_same_v<T, DoorTarget>) {
                        t.wall.access([&](Wall& wall) {
                            inserted = Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Wall::MAX_KEYFRAMES>(wall.keyframes), k);
                        });
                    } else if constexpr (std::is_same_v<T, LockTarget>) {
                        t.wall.access([&](Wall& wall) {
                            inserted = Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Wall::MAX_KEYFRAMES>(wall.lockKeyframes), k);
                        });
                    }
                }, activation.targetEntity);
            } else {
                if (cfg.isDebug) activation.codeset.addLog(CODE_ADD_KEYFRAME_INSERTION_CHARACTER, activation.character.characterId);
                inserted = Keyframe::insertKeyframe(
                    Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(activation.character.keyframes), k);
            }
        });
        if (activation.codeset.addFailure(!inserted, CODE_ACTIVATION_ADD_TARGET_KEYFRAME_FAILED_TO_INSERT)) return;

        result = true;
    });
    return result;
}
