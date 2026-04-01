#pragma once

#include <variant>
#include "Cardinal.hpp"

// Raw request-layer identifiers, set by the ApiController before activation resolves them.
// MatchController::activate converts these into the concrete TargetEntity pointers.

struct NoPreactivationTarget {};
struct PreactivationTargetCharacter { int characterId; };
struct PreactivationTargetItem      { int itemIndex;   };
struct PreactivationTargetDoor      { Cardinal direction; };
struct PreactivationTargetLock      { Cardinal direction; };

using TargetPreactivationEntity = std::variant<
    NoPreactivationTarget,
    PreactivationTargetCharacter,
    PreactivationTargetItem,
    PreactivationTargetDoor,
    PreactivationTargetLock
>;
