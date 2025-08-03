#pragma once

#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "Pointer.hpp"
#include "Activation.hpp"

class Player;
class Character;
class Room;
class Match;

class iActivator {
public:
    virtual CodeEnum activate(Activation& activation) const { return CODE_NOOP; }
    virtual ~iActivator() = default;
}; 