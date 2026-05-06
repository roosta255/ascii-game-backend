#pragma once

#include <variant>

#include "ElevatorContent.hpp"
#include "RemodelAuthor.hpp"
#include "RemodelAuthorSetBuilderStartingRoomId.hpp"
#include "RemodelGroup.hpp"
#include "RemodelJailer.hpp"
#include "RemodelKeeper.hpp"
#include "RemodelShifter.hpp"
#include "RemodelTogglerBlue.hpp"
#include "RemodelTogglerOrange.hpp"
#include "RemodelTogglerSwitch.hpp"

using GeneratorRemodelVariant = std::variant<
    ElevatorContentAtom,
    ElevatorContentBlank,
    ElevatorContentHandcrafted,
    RemodelAuthorChestSeeder,
    RemodelAuthorRunGeneratorLoop,
    RemodelAuthorSetup4x1Room,
    RemodelAuthorSetup2x5Room,
    RemodelAuthorSetup3x3Room,
    RemodelAuthorSetBuilderStartingRoomId,
    RemodelAuthorSetupBuilderRole,
    RemodelAuthorSetupCovenantDoor,
    RemodelAuthorSetupElevatorColumn,
    RemodelAuthorSetupDoorway,
    RemodelAuthorSetupExitDoor,
    RemodelAuthorSetupHorizontalWalls,
    RemodelAuthorSetupJailer,
    RemodelAuthorSetupKeeper,
    RemodelAuthorSetupLadderUp,
    RemodelAuthorSetupLightningRodRoom,
    RemodelAuthorSetupPoleUp,
    RemodelAuthorSetupPowerGeneratorRoom,
    RemodelAuthorSetupSacramentForgiveness,
    RemodelAuthorSetupShifter,
    RemodelAuthorSetupTimeGateRoomToFuture,
    RemodelAuthorSetupTogglerBlue,
    RemodelAuthorSetupTogglerOrange,
    RemodelAuthorSetupTogglerSwitch,
    RemodelAuthorSetupVerticalWalls,
    RemodelAuthorApply,
    RemodelGroup,
    RemodelJailer,
    RemodelKeeper,
    RemodelKeeperKeyed,
    RemodelShifter,
    RemodelShifterKeyed,
    RemodelTogglerBlue,
    RemodelTogglerOrange,
    RemodelTogglerSwitch
>;
