#pragma once
#include <cstdint>
#include "Array.hpp"
#include "FieldEnum.hpp"
#include "FieldState.hpp"

class Match;

struct FieldController {
    // Below this, a room's OXYGEN field counts as hypoxic (see isRoomHypoxic).
    static constexpr int16_t HYPOXIA_THRESHOLD = 30;

    void recompute(const Match& match);
    int16_t get(FieldEnum field, int roomId) const;

    // Cheap single-room, single-field check that does not require a recomputed
    // FieldController instance — OXYGEN is a ROOM_ACCUMULATION field, so its value
    // for one room can be read directly from the source provider. This lets
    // Character::endTurn query hypoxia without paying for a full recompute, which
    // matters because it also runs during pathfinding's speculative turn advances
    // (see MatchController's static advanceTitanTurnState/advanceBuilderTurnState).
    static bool isRoomHypoxic(const Match& match, int roomId);

private:
    Array<FieldState, FIELD_COUNT> states;
};
