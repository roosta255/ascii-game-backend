#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "DamageTypeBits.hpp"
#include "Item.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "Timestamp.hpp"

class Player;
class Character;
class Inventory;
class Room;
class Match;
class MatchController;
class Codeset;

struct Activation {
    Player& player;
    Character& character;
    Room& room;
    Pointer<Character> target;
    Pointer<Item> sourceItem;
    Pointer<Item> targetItem;
    Maybe<Cardinal> direction;
    Match& match;
    Codeset& codeset;
    Timestamp time;
    MatchController& controller;
    Maybe<int> floorId;
    bool isSkippingAnimations = false;
    bool isSortingState = false;
    Pointer<Inventory> sourceInventory;
    Pointer<Inventory> targetInventory;
    DamageTypeBits damageTypes;
    bool isSkippingLogging = false;
    Array<LoggedEvent, 32> eventLog;
};