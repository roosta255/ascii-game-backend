#pragma once

#include <variant>

// activator
#include "ActivatorEndTurn.hpp"
#include "ActivatorMoveToDoor.hpp"
#include "ActivatorMoveToFloor.hpp"
#include "ActivatorUseCharacter.hpp"
#include "ActivatorUseDoor.hpp"
#include "ActivatorUseInventoryItem.hpp"
#include "ActivatorUseLock.hpp"
// chest
#include "ActivatorChestLockKey.hpp"
#include "ActivatorCritterBite.hpp"
#include "ActivatorLootChest.hpp"
#include "ActivatorUseChestLock.hpp"
// damage
#include "ActivatorAttack.hpp"
#include "ActivatorDamage.hpp"
#include "ActivatorDamageBite.hpp"
#include "ActivatorDamageCold.hpp"
#include "ActivatorDamageCrush.hpp"
#include "ActivatorDamageElectric.hpp"
#include "ActivatorDamageFire.hpp"
#include "ActivatorDamagePierce.hpp"
// door
#include "ActivatorBounceLock.hpp"
#include "ActivatorCleanseAfflictions.hpp"
#include "ActivatorElevator.hpp"
#include "ActivatorGiveItem.hpp"
#include "ActivatorInactiveDoor.hpp"
#include "ActivatorJailer.hpp"
#include "ActivatorKeeper.hpp"
#include "ActivatorLadder.hpp"
#include "ActivatorLightningRod.hpp"
#include "ActivatorPole.hpp"
#include "ActivatorSetDoor.hpp"
#include "ActivatorSetSharedDoors.hpp"
#include "ActivatorShifter.hpp"
#include "ActivatorTimeGate.hpp"
#include "ActivatorTimeGateCube.hpp"
#include "ActivatorValidateNotOccupied.hpp"
// inventory
#include "ActivatorInactiveItem.hpp"
// role
#include "ActivatorToggler.hpp"

#include "Activation.hpp"

struct NoEffect {
    bool activate(Activation&) const { return true; }
};

using ActivatorEffect = std::variant<
    NoEffect,
    // activator
    ActivatorEndTurn,
    ActivatorMoveToDoor,
    ActivatorMoveToFloor,
    ActivatorUseCharacter,
    ActivatorUseDoor,
    ActivatorUseInventoryItem,
    ActivatorUseLock,
    // chest
    ActivatorChestLockKey,
    ActivatorCritterBite,
    ActivatorLootChest,
    ActivatorUseChestLock,
    // damage
    ActivatorAttack,
    ActivatorDamage,
    ActivatorDamageBite,
    ActivatorDamageCold,
    ActivatorDamageCrush,
    ActivatorDamageElectric,
    ActivatorDamageFire,
    ActivatorDamagePierce,
    // door
    ActivatorBounceLock,
    ActivatorCleanseAfflictions,
    ActivatorElevator,
    ActivatorGiveItem,
    ActivatorInactiveDoor,
    ActivatorJailer,
    ActivatorKeeper,
    ActivatorLadder,
    ActivatorLightningRod,
    ActivatorPole,
    ActivatorSetDoor,
    ActivatorSetSharedDoors,
    ActivatorShifter,
    ActivatorTimeGate,
    ActivatorTimeGateCube,
    ActivatorValidateNotOccupied,
    // inventory
    ActivatorInactiveItem,
    // role
    ActivatorToggler
>;
