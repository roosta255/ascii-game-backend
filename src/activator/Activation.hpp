#pragma once

#include "Cardinal.hpp"
#include "DamageTypeBits.hpp"
#include "Item.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "RequestContext.hpp"

class Character;
class Inventory;

struct ActivationContext {
    Pointer<RequestContext> request;

    Character& character;

    Pointer<Character> target;
    Pointer<Item> sourceItem;
    Pointer<Item> targetItem;

    Pointer<Inventory> sourceInventory;
    Pointer<Inventory> targetInventory;

    Maybe<Cardinal> direction;

    DamageTypeBits damageTypes;

    bool isSortingState = false;
};

// Backward-compatibility alias — remove once all sites migrated
using Activation = ActivationContext;
