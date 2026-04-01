#pragma once

#include <variant>
#include "Item.hpp"
#include "Pointer.hpp"

class Character;
struct Wall;

// Thin Wall wrappers so DoorTarget and LockTarget are distinct variant arms
struct DoorTarget { Pointer<Wall> wall; };
struct LockTarget { Pointer<Wall> wall; };

struct NoTarget {};

using TargetEntity = std::variant<
    NoTarget,
    Pointer<Character>,
    Pointer<Item>,
    DoorTarget,
    LockTarget
>;
