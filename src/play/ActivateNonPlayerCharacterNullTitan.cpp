#include "ActivateNonPlayerCharacterNullTitan.hpp"
#include "ActivationContext.hpp"
#include "ActionEnum.hpp"
#include "Character.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "TraitEnum.hpp"

bool ActivateNonPlayerCharacterNullTitan::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        const std::string playerId = req.player.account.toString();

        for (auto& character : req.match.dungeon.characters) {
            if (character.location.roomId < 0) continue;
            if (req.controller.getConductByCharacterId(character.characterId).isEmpty()) continue;

            constexpr int MAX_ACTIVATIONS = 8;
            for (int i = 0; i < MAX_ACTIVATIONS; ++i) {
                const auto traits = req.controller.getTraitsComputed(character.characterId).final;
                if (!traits[TRAIT_ACTION_READY].orElse(false) && !traits[TRAIT_MOVEMENT_READY].orElse(false)) break;
            }
        }

        result = (req.controller.endTitanTurn() == CODE_SUCCESS);
    });
    return result;
}
