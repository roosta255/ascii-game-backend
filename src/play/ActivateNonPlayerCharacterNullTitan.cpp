#include "ActivateNonPlayerCharacterNullTitan.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullTitan::activate(Activation& activation) const {
    return activation.match.turner.endTitanTurn(activation.match);
} 