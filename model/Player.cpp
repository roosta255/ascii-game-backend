#include "Player.hpp"
#include "Match.hpp"
#include "Character.hpp"
#include "Builder.hpp"
#include <json/json.h>

void Player::startTurn(Match& match)
{
}

void Player::endTurn(Match& match)
{
}

bool Player::isEmpty() const
{
    return this->account.empty();
}

bool Player::isHuman() const
{
    return this->account.substr(0, 3) != "NPC";
}
