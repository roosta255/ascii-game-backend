#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullBuilder::activate(Activation& activation) const {
    return activation.match.turner.endBuilderTurn(activation.match);
} 