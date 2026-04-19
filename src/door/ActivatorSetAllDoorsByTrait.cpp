#include "ActivatorSetAllDoorsByTrait.hpp"
#include "Cardinal.hpp"
#include "DoorFlyweight.hpp"
#include "Match.hpp"
#include "Room.hpp"

bool ActivatorSetAllDoorsByTrait::activate(ActivationContext& activation) const {
    activation.request.access([&](RequestContext& req) {
        int roomId = 0;
        for (Room& room : req.match.dungeon.rooms) {
            for (Cardinal dir : Cardinal::getAllCardinals()) {
                Wall& wall = room.getWall(dir);
                DoorFlyweight::getFlyweights().accessConst((int)wall.door, [&](const DoorFlyweight& fw) {
                    if ((match - fw.doorAttributes).isAny()) return;
                    const TraitBits newTraits = (fw.doorAttributes | set) - clear;
                    if (newTraits == fw.doorAttributes) return;
                    DoorEnum newDoor;
                    if (!DoorFlyweight::findByTraits(newTraits, newDoor)) return;
                    wall.setDoor(newDoor, req.time, req.isSkippingAnimations, roomId, animation);
                });
            }
            roomId++;
        }
    });
    return true;
}
