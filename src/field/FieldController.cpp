#include "FieldController.hpp"
#include <algorithm>
#include "DUNGEON_ROOM_COUNT.hpp"
#include "FieldFlyweight.hpp"
#include "iFieldResolver.hpp"
#include "iFieldSourceProvider.hpp"

void FieldController::recompute(const Match& match) {
    int i = 0;
    for (const FieldFlyweight& flyweight : FieldFlyweight::getFlyweights()) {
        states.access(i, [&](FieldState& state) {
            state = FieldState{};
            flyweight.resolver.accessConst([&](const iFieldResolver& resolver) {
                flyweight.sourceProvider.accessConst([&](const iFieldSourceProvider& source) {
                    resolver.resolve(match, source, flyweight, state);
                });
            });
        });
        i++;
    }
}

int16_t FieldController::get(FieldEnum field, int roomId) const {
    int16_t result = 0;
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) return result;
    states.accessConst(field, [&](const FieldState& state) {
        result = state.values[roomId];
    });
    return result;
}

bool FieldController::isRoomHypoxic(const Match& match, int roomId) {
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) return false;

    int16_t oxygen = 0;
    FieldFlyweight::getFlyweights().accessConst(FIELD_OXYGEN, [&](const FieldFlyweight& flyweight) {
        flyweight.sourceProvider.accessConst([&](const iFieldSourceProvider& source) {
            oxygen = std::clamp(source.getRoomBaseValue(match, roomId), flyweight.minValue, flyweight.maxValue);
        });
    });
    return oxygen < HYPOXIA_THRESHOLD;
}
