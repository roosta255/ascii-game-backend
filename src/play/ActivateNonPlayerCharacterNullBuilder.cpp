#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullBuilder::activate(Activation& activation) const {
    return activation.request->match.turner.endBuilderTurn(activation.request->match);
} 