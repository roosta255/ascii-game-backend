#include "Builder.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>

void Builder::startTurn(Match& match) {
    character.startTurn(match);
}

void Builder::endTurn(Match& match) {
    character.endTurn(match);
}
