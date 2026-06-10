#include "BehaviorFlyweight.hpp"
#include "iProposer.hpp"
#include "Array.hpp"
#include "TriggerConfig.hpp"
#include "TriggerWrapper.hpp"

// Heavy includes for concrete proposer definitions below.
#include "ActivationContext.hpp"
#include "ActionEnum.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "RequestContext.hpp"
#include "TraitEnum.hpp"

#include <functional>
#include <unordered_map>
#include <utility>

// ─── Concrete proposers ──────────────────────────────────────────────────────

struct ProposerPickpocketSearch final : public iProposer {
    ConductEnum conduct() const override { return CONDUCT_PICKPOCKET; }

    Maybe<std::pair<int, std::function<void(ActivationContext&)>>> buildProposal(ActivationContext& ctx) const override {
        Maybe<std::pair<int, std::function<void(ActivationContext&)>>> result;
        ctx.request.access([&](RequestContext& req) {
            auto& actor      = ctx.character;
            const int roomId = actor.location.roomId;
            const auto traits = req.controller.getTraitsComputed(actor.characterId).final;
            if (!traits[TRAIT_PICKPOCKETER].orElse(false)) return;
            if (!traits[TRAIT_ACTION_READY].orElse(false)) return;

            int targetId = -1;
            for (auto& builder : req.match.builders) {
                bool inRoom = false;
                req.controller.isCharacterWithinRoom(builder.character.characterId, roomId, inRoom);
                if (!inRoom) continue;
                const auto vt = req.controller.getTraitsComputed(builder.character.characterId).final;
                if (!vt[TRAIT_PICKPOCKETABLE].orElse(false)) continue;
                if (vt[TRAIT_ACTION_READY].orElse(false)) continue;
                targetId = builder.character.characterId;
                break;
            }
            if (targetId == -1) return;

            result = std::make_pair(3, std::function<void(ActivationContext&)>([targetId](ActivationContext& ctx) {
                ctx.request.access([&](RequestContext& req) {
                    req.controller.getConductByCharacterId(ctx.character.characterId).access([&](Conduct& conduct) {
                        conduct.memory.access(CONDUCT_PICKPOCKET, [&](ConductMemory& mem) {
                            mem.targetCharacterId = targetId;
                        });
                    });
                    req.controller.activate(Preactivation{
                        .action = {
                            .type              = ACTION_PICKPOCKET,
                            .characterId       = ctx.character.characterId,
                            .roomId            = ctx.character.location.roomId,
                            .targetCharacterId = targetId,
                        },
                        .playerId             = req.player.account.toString(),
                        .isSkippingAnimations = req.isSkippingAnimations,
                    });
                });
            }));
        });
        return result;
    }
};

// ─── Flyweight table ─────────────────────────────────────────────────────────

Maybe<TriggerWrapper> BehaviorFlyweight::getActivatorForEvent(BehaviorEventEnum event) const {
    switch (event) {
        case BEHAVIOR_EVENT_MOVE:        return onMove;
        case BEHAVIOR_EVENT_ATTACK:      return onAttack;
        case BEHAVIOR_EVENT_DAMAGE:      return onDamage;
        case BEHAVIOR_EVENT_LOOT:        return onLoot;
        case BEHAVIOR_EVENT_DEATH:       return onDeath;
        case BEHAVIOR_EVENT_PICKPOCKET:  return onPickpocket;
        case BEHAVIOR_EVENT_DEPOSIT:     return onDeposit;
        default:                         return Maybe<TriggerWrapper>::empty();
    }
}

const Array<BehaviorFlyweight, BEHAVIOR_COUNT>& BehaviorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<BehaviorFlyweight, BEHAVIOR_COUNT> flyweights;
        BehaviorEnum lastBehavior = BEHAVIOR_NIL;

        #define BEHAVIOR_DECL(name_) \
            lastBehavior = BEHAVIOR_##name_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.name = #name_; \
            });
        #define BEHAVIOR_ON_MOVE_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onMove = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_ATTACK_DECL(config_)
        #define BEHAVIOR_ON_DAMAGE_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDamage = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_LOOT_DECL(config_)
        #define BEHAVIOR_ON_DEATH_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeath = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_PICKPOCKET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onPickpocket = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DEPOSIT_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeposit = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_ACT_DECL(config_)
        #define BEHAVIOR_PROPOSER_DECL(type_) \
            { \
                static const type_ s_proposer{}; \
                flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                    fw.proposer = s_proposer; \
                }); \
            }

        #include "Behavior.enum"

        #undef BEHAVIOR_DECL
        #undef BEHAVIOR_ON_MOVE_DECL
        #undef BEHAVIOR_ON_ATTACK_DECL
        #undef BEHAVIOR_ON_DAMAGE_DECL
        #undef BEHAVIOR_ON_LOOT_DECL
        #undef BEHAVIOR_ON_DEATH_DECL
        #undef BEHAVIOR_ON_PICKPOCKET_DECL
        #undef BEHAVIOR_ON_DEPOSIT_DECL
        #undef BEHAVIOR_ON_ACT_DECL
        #undef BEHAVIOR_PROPOSER_DECL

        return flyweights;
    }();
    return flyweights;
}

bool BehaviorFlyweight::indexByString(const std::string& name, BehaviorEnum& output) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, BehaviorEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for (const BehaviorFlyweight& fw : flyweights) {
            nameMap[fw.name] = BehaviorEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
