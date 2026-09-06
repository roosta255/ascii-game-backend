#pragma once

#include <variant>

// activator
#include "ActivatorAlterTraitAffliction.hpp"
#include "ActivatorEndTurn.hpp"
#include "ActivatorFindInventoryItemByTraits.hpp"
#include "ActivatorLogResolvedItemEvent.hpp"
#include "ActivatorMoveToDoor.hpp"
#include "ActivatorMoveToFloor.hpp"
#include "ActivatorPushBehaviorTrigger.hpp"
#include "ActivatorRuleExecution.hpp"
#include "ActivatorTransferItem.hpp"
#include "ActivatorUseCharacter.hpp"
#include "ActivatorUseDoor.hpp"
#include "ActivatorUseInventoryItem.hpp"
#include "ActivatorUseLock.hpp"
// chest
#include "ActivatorChestLockKey.hpp"
#include "ActivatorCritterBite.hpp"
#include "ActivatorLootChest.hpp"
#include "ActivatorUseChestLock.hpp"
#include "DungeonChestLockTransformer.hpp"
// damage
#include "ActivatorAttack.hpp"
#include "ActivatorDamage.hpp"
#include "ActivatorDamageBite.hpp"
#include "ActivatorDamageCold.hpp"
#include "ActivatorDamageCrush.hpp"
#include "ActivatorDamageElectric.hpp"
#include "ActivatorDamageFire.hpp"
#include "ActivatorDamagePierce.hpp"
// activator (additional)
#include "ActivationAddTargetKeyframe.hpp"
#include "ActivatorDeposit.hpp"
// door
#include "ActivatorExitDungeon.hpp"
#include "ActivatorSetAllDoorsByTrait.hpp"
#include "ActivatorCleanseAfflictions.hpp"
#include "ActivatorElevator.hpp"
#include "ActivatorGiveItem.hpp"
#include "ActivatorInactiveDoor.hpp"
#include "ActivatorLadder.hpp"
#include "ActivatorLightningRod.hpp"
#include "ActivatorPole.hpp"
#include "ActivatorSetDoor.hpp"
#include "ActivatorSetSharedDoors.hpp"
#include "ActivatorSetSharedDoorsByTrait.hpp"
#include "ActivatorTimeGate.hpp"
#include "ActivatorTimeGateCube.hpp"
#include "ActivatorValidateNotOccupied.hpp"
// inventory
#include "ActivatorInactiveItem.hpp"
// role
#include "ActivatorToggler.hpp"
#include "DungeonRoleTransformer.hpp"

#include "ActivationContext.hpp"

struct NoEffect {
    bool activate(ActivationContext&) const { return true; }
};

using ActivatorEffect = std::variant<
    NoEffect,
    // activator
    ActivatorAlterTraitAffliction,
    ActivatorDeposit,
    ActivatorEndTurn,
    ActivatorFindInventoryItemByTraits,
    ActivatorLogResolvedItemEvent,
    ActivatorMoveToDoor,
    ActivatorMoveToFloor,
    ActivatorPushBehaviorTrigger,
    ActivatorRuleExecution,
    ActivatorTransferItem,
    ActivatorUseCharacter,
    ActivatorUseDoor,
    ActivatorUseInventoryItem,
    ActivatorUseLock,
    // chest
    ActivatorChestLockKey,
    ActivatorCritterBite,
    ActivatorLootChest,
    ActivatorUseChestLock,
    DungeonChestLockTransformer,
    // damage
    ActivatorAttack,
    ActivatorDamage,
    ActivatorDamageBite,
    ActivatorDamageCold,
    ActivatorDamageCrush,
    ActivatorDamageElectric,
    ActivatorDamageFire,
    ActivatorDamagePierce,
    // activator (additional)
    ActivationAddTargetKeyframe,
    // door
    ActivatorExitDungeon,
    ActivatorSetAllDoorsByTrait,
    ActivatorCleanseAfflictions,
    ActivatorElevator,
    ActivatorGiveItem,
    ActivatorInactiveDoor,
    ActivatorLadder,
    ActivatorLightningRod,
    ActivatorPole,
    ActivatorSetDoor,
    ActivatorSetSharedDoors,
    ActivatorSetSharedDoorsByTrait,
    ActivatorTimeGate,
    ActivatorTimeGateCube,
    ActivatorValidateNotOccupied,
    // inventory
    ActivatorInactiveItem,
    // role
    ActivatorToggler,
    DungeonRoleTransformer
>;
