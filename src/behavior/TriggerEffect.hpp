#pragma once

#include <variant>

// BehaviorEnum is forward-declared to avoid a circular include.
// By the time Behavior.enum is expanded in BehaviorFlyweight.cpp, BehaviorEnum
// is already fully defined via BehaviorFlyweight.hpp → BehaviorEnum.hpp.
enum BehaviorEnum : int;

// Effects applied to ConductMemory (and optionally character state) when a
// TriggerWrapper fires. "Scanned" variants read from the populated scanned*Id
// locals set by a preceding scan-match; "FromContext" variants read directly
// from the ActivationContext.

struct TriggerEffectSetBehavior              { BehaviorEnum behavior; };  // character.behavior = behavior
struct TriggerEffectSetTargetCharacter       {};  // scannedCharacterId → memory.targetCharacterId
struct TriggerEffectSetTargetCharacterFromContext {};  // activation.targetCharacter() → memory.targetCharacterId
struct TriggerEffectClearTargetCharacter     {};  // memory.targetCharacterId = -1
struct TriggerEffectSetTargetObject          {};  // scannedObjectId → memory.targetObjectId
struct TriggerEffectClearTargetObject        {};  // memory.targetObjectId = -1
struct TriggerEffectSetTargetRoom            {};  // scannedRoomId → memory.targetRoomId
struct TriggerEffectClearTargetRoom          {};  // memory.targetRoomId = -1

using TriggerEffect = std::variant<
    std::monostate,                              // empty slot — terminates iteration
    TriggerEffectSetBehavior,
    TriggerEffectSetTargetCharacter,
    TriggerEffectSetTargetCharacterFromContext,
    TriggerEffectClearTargetCharacter,
    TriggerEffectSetTargetObject,
    TriggerEffectClearTargetObject,
    TriggerEffectSetTargetRoom,
    TriggerEffectClearTargetRoom
>;
