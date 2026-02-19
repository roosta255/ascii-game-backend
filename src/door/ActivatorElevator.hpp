#pragma once

#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "iActivator.hpp"

class ActivatorElevator : public iActivator {
public:

    // functions
    static constexpr bool isKeyed(const DoorEnum type) {
        return type == DOOR_ELEVATOR_CLOSED_KEYED
        || type == DOOR_ELEVATOR_CLOSED_KEYED_BUTTON
        || type == DOOR_ELEVATOR_OPEN_KEYED
        || type == DOOR_ELEVATOR_OPEN_KEYED_BUTTON
        || type == DOOR_ELEVATOR_CLOSED_CALL_BUTTON;
    }

    static constexpr bool isKeyless(const DoorEnum type) {
        return type == DOOR_ELEVATOR_CLOSED_KEYLESS
        || type == DOOR_ELEVATOR_CLOSED_KEYLESS_BUTTON
        || type == DOOR_ELEVATOR_OPEN_KEYLESS
        || type == DOOR_ELEVATOR_OPEN_KEYLESS_BUTTON;
    }

    static constexpr bool isPayingButton(const DoorEnum type) {
        return type == DOOR_ELEVATOR_CLOSED_KEYLESS_BUTTON
        || type == DOOR_ELEVATOR_OPEN_KEYLESS_BUTTON;
    }

    static constexpr bool isMovingButton(const DoorEnum type) {
        return type == DOOR_ELEVATOR_CLOSED_KEYED_BUTTON
        || type == DOOR_ELEVATOR_OPEN_KEYED_BUTTON;
    }

    static constexpr DoorEnum getType(const bool isOpen, const bool isKeyed, const bool isButton) {
        return isOpen
        // open
        ? (isKeyed
            // open,keyed
            ? (isButton ? (DOOR_ELEVATOR_OPEN_KEYED_BUTTON) : (DOOR_ELEVATOR_OPEN_KEYED))
            // open,keyless
            : (isButton ? (DOOR_ELEVATOR_OPEN_KEYLESS_BUTTON) : (DOOR_ELEVATOR_OPEN_KEYLESS)))
        // closed
        : (isKeyed
            // closed,keyed
            ? (isButton ? (DOOR_ELEVATOR_CLOSED_KEYED_BUTTON) : (DOOR_ELEVATOR_CLOSED_KEYED))
            // closed,keyless
            : (isButton ? (DOOR_ELEVATOR_CLOSED_KEYLESS_BUTTON) : (DOOR_ELEVATOR_CLOSED_KEYLESS)));
    }

    static constexpr DoorEnum getExteriorType
    ( const bool isElevatorPresent
    , const bool isPaid
    ) {
        // call the elevator if floor is paid and not on this floor
        // ELEVATOR_OPEN_KEYED         <= paid & present
        // ELEVATOR_CLOSED_CALL_BUTTON <= paid & not-present
        // ELEVATOR_CLOSED_KEYLESS     <= not-paid
        return !isPaid ? DOOR_ELEVATOR_CLOSED_KEYLESS : isElevatorPresent ? DOOR_ELEVATOR_OPEN_KEYED : DOOR_ELEVATOR_CLOSED_CALL_BUTTON;
    }

    static constexpr DoorEnum getInteriorType
    ( const Cardinal dir
    , const bool isPaid
    , const bool isDoorway
    , const bool isExistingHigher
    , const bool isExistingLower
    ) {
        // north interior and higher exists in any direction, then button else not | (if adjacent is doorway & shared & not-blocking, then open else closed)
        // south interior and  lower exists in any direction, then button else not | (if adjacent is doorway & shared & not-blocking, then open else closed)
        return getType
        ( isDoorway // isOpen
        , isPaid // isKeyed
        , dir == Cardinal::north() && isExistingHigher || dir == Cardinal::south() && isExistingLower); // isButton
    }

    bool activate(Activation& activation) const override;

    static bool setupElevatorLevel(const int elevatorRoomId, const bool isElevatorPresent, const bool isPaid, const bool isDoorway, const bool isExistingHigher, const bool isExistingLower);
};
