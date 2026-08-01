#include "Player.hpp"
#include "Dungeon.hpp"
#include "Inventory.hpp"
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

Inventory Player::getInventory(Dungeon& dungeon) const {
    if (itemStartIndex < 0) return Inventory(nullptr, 0);
    return Inventory(dungeon.items.begin() + itemStartIndex, INVENTORY_SIZE);
}

bool Player::isEmpty() const
{
    return this->account.isEmpty();
}

bool Player::isHuman() const
{
    return this->account.toString().substr(0, 3) != "NPC";
}
