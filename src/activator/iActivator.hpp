#pragma once

#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "Pointer.hpp"
#include "ActivationContext.hpp"

class Player;
class Character;
class Room;
class Match;

class iActivator {
public:
    virtual bool activate(ActivationContext& activation) const { return CODE_NOOP; }
    virtual ~iActivator() = default;
}; 