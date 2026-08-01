#include "FieldController.hpp"
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
