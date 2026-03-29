#include "ActivateNonPlayerCharacterNullTitan.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullTitan::activate(Activation& activation) const {
    return activation.request->match.turner.endTitanTurn(activation.request->match);
} 