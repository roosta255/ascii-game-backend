#include "ActivatorElevator.hpp"
#include "build_room_map.hpp"
#include "Cardinal.hpp"
#include "Chest.hpp"
#include "ActivatorLightningRod.hpp"
#include "ActivatorTimeGate.hpp"
#include "Character.hpp"
#include "CharacterAllocSpec.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonMutator.hpp"
#include "iLayout.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleFlyweight.hpp"
#include "RoleEnum.hpp"

DungeonMutator::DungeonMutator(MatchController& controller)
    : dungeon(controller.match.dungeon), codeset(controller.codeset), controller(controller) {}

DungeonMutator::DungeonMutator(MatchController& controller, const iLayout& layoutIn)
    : dungeon(controller.match.dungeon), codeset(controller.codeset), controller(controller), layout(&layoutIn)
{
    size = build_room_map(dungeon.rooms, layoutIn, mapping);
    coordinateResolver = [this](const int4& c){ return this->getRoomId(c); };
}

void DungeonMutator::setCoordinateResolver(std::function<Maybe<int>(const int4&)> resolver) {
    coordinateResolver = std::move(resolver);
}

Maybe<int> DungeonMutator::getRoomId(const int4& coord) {
    auto found = mapping.find(coord);
    if (found == mapping.end()) {
        codeset.addError(CODE_DUNGEON_AUTHOR_FAILED_MAP_COORDINATE);
        return Maybe<int>::empty();
    }
    return found->second.map<int>([&](Room& room){ return room.roomId; });
}

Maybe<int> DungeonMutator::resolveRoom(const RoomRef& room) {
    return std::visit([&](auto&& v) -> Maybe<int> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) {
            return Maybe<int>(v);
        } else {
            if (!coordinateResolver) {
                codeset.addError(CODE_DUNGEON_MUTATOR_COORDINATE_RESOLVER_MISSING);
                return Maybe<int>::empty();
            }
            return coordinateResolver(v);
        }
    }, room);
}

Pointer<Room> DungeonMutator::getRoom(const int& roomId) {
    auto roomPtr = dungeon.rooms.getPointer(roomId);
    if (roomPtr.isEmpty()) {
        codeset.addError(CODE_DUNGEON_MUTATOR_GET_ROOM_REQUIRES_VALID_ROOM_ID);
    }
    return roomPtr;
}

bool DungeonMutator::setDoor(DoorEnum& source, DoorEnum setting) {
    if (source == setting) {
        return true;
    }
    const bool isDirtyUpdate = source != DOOR_WALL && source != DOOR_DOORWAY;
    const bool isElevatorUpdate = isElevatorOverride && ActivatorElevator::isElevator(source) && ActivatorElevator::isElevator(setting);
    if (isDirtyUpdate && !isElevatorUpdate) {
        codeset.addError(CODE_DUNGEON_MUTATOR_SET_DOOR_REQUIRES_DEFAULT_DOOR_TYPE);
        return false;
    }
    source = setting;
    return true;
}

bool DungeonMutator::setDoor(const RoomRef& room, Cardinal dir, DoorEnum type, const Maybe<int> setRoomId) {
    return resolveRoom(room).map<bool>([&](const int& roomId) {
        return getRoom(roomId).map<bool>([&](Room& r){
            Wall& wall = r.getWall(dir);
            if (!setDoor(wall.door, type)) {
                return false;
            }
            setRoomId.accessConst([&](const int& id){ wall.adjacent = id; });
            return true;
        }).orElse(false);
    }).orElse(false);
}

bool DungeonMutator::setRoom(const RoomRef& room, RoomEnum type) {
    return resolveRoom(room).map<bool>([&](const int& roomId) {
        return getRoom(roomId).map<bool>([&](Room& r){
            if (r.type != ROOM_RECT_4_x_5 && r.type != type) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_ROOM_REQUIRES_DEFAULT_ROOM_TYPE);
                return false;
            }
            r.type = type;
            return true;
        }).orElse(false);
    }).orElse(false);
}

bool DungeonMutator::setSharedDoor(
    const RoomRef& room1Ref,
    Cardinal dir,
    DoorEnum door1,
    DoorEnum door2,
    const Maybe<int> setRoomId
) {
    return resolveRoom(room1Ref).map<bool>([&](const int& room1Id) {
        return getRoom(room1Id).map<bool>([&](Room& room1){
            Wall& wall1 = room1.getWall(dir);
            if (!setDoor(wall1.door, door1)) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_1);
                return false;
            }
            return getRoom(setRoomId.orElse(wall1.adjacent)).map<bool>([&](Room& room2){
                Wall& wall2 = room2.getWall(dir.getFlip());
                if (!setDoor(wall2.door, door2)) {
                    codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_2);
                    return false;
                }
                setRoomId.accessConst([&](const int& room2Id){
                    wall1.adjacent = room2Id;
                    wall2.adjacent = room1Id;
                });
                return true;
            }).orElse(false);
        }).orElse(false);
    }).orElse(false);
}

bool DungeonMutator::setSharedShaftAbove(
    const RoomRef& room1Ref,
    Cardinal dir,
    DoorEnum door1,
    DoorEnum door2
) {
    return resolveRoom(room1Ref).map<bool>([&](const int& room1Id) {
        return getRoom(room1Id).map<bool>([&](Room& room1){
            Wall& wall1 = room1.getWall(dir);
            if (!setDoor(wall1.door, door1)) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_SHAFT_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_1);
                return false;
            }
            return getRoom(room1.above).map<bool>([&](Room& room2){
                Wall& wall2 = room2.getWall(dir);
                if (!setDoor(wall2.door, door2)) {
                    codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_2);
                    return false;
                }
                return true;
            }).orElse(false);
        }).orElse(false);
    }).orElse(false);
}

bool DungeonMutator::setupDoorway (const RoomRef& room, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(room, dir, DOOR_DOORWAY, DOOR_DOORWAY), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_DOORWAY);
}

bool DungeonMutator::setupExitDoor (const RoomRef& room, const Cardinal dir) {
    return !codeset.addFailure(!setDoor(room, dir, DOOR_EXIT), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_EXIT_DOOR);
}

bool DungeonMutator::setupChest(const RoomRef& room, const ChestSpec& spec) {
    return resolveRoom(room).map<bool>([&](const int& roomId) {
        ChestAllocSpec chestAlloc;
        chestAlloc.lock = spec.lock;

        if (spec.critterRole != ROLE_EMPTY)
            chestAlloc.inventory.push_back(CritterCharacterSpec{.character = CharacterAllocSpec{.role = spec.critterRole}});
        spec.containedRole.accessConst([&](const RoleEnum& role) {
            chestAlloc.inventory.push_back(ContainedCharacterSpec{.character = CharacterAllocSpec{.role = role}});
        });
        for (ItemEnum item : spec.items)
            chestAlloc.inventory.push_back(ItemSpec{.item = item});

        int containerId = -1;
        return !codeset.addFailure(!controller.allocate(
            CharacterAllocSpec{.role = ROLE_CHEST, .chest = chestAlloc},
            AttachmentContext::floor(roomId),
            containerId
        ), CODE_DUNGEON_MUTATOR_FAILED_TO_ALLOCATE_CHARACTER);
    }).orElse(false);
}

bool DungeonMutator::setup2x5Room(const RoomRef& room) {
    return setRoom(room, ROOM_RECT_2_x_5);
}

bool DungeonMutator::setup3x3Room(const RoomRef& room) {
    return setRoom(room, ROOM_RECT_3_x_3);
}

bool DungeonMutator::setup4x1Room(const RoomRef& room) {
    return setRoom(room, ROOM_RECT_4_x_1);
}

bool DungeonMutator::setupElevatorLevel(const RoomRef& elevatorRoom, const ElevatorProperties& elevatorProperties, const bool isElevatorPresent, const bool isExistingHigher, const bool isExistingLower, const bool isHigherPaid, const bool isLowerPaid) {
    bool isSuccess = true;
    for (const auto dir: Cardinal::getAllCardinals()) {
        const auto exteriorDoorType = ActivatorElevator::getExteriorType(isElevatorPresent, elevatorProperties.isPaid);
        if (codeset.addFailure(exteriorDoorType == DOOR_ELEVATOR_OPEN_KEYED_BUTTON_CALLING, CODE_ELEVATOR_OPEN_KEYED_BUTTON_CALLING_CANNOT_BE_EXTERIOR)) {
            return false;
        }
        elevatorProperties.connectedRoomIds.accessConst(dir.getIndex(), [&](const Maybe<int>& maybeRoomId){
            const auto interiorDoorType = ActivatorElevator::getInteriorType(dir, elevatorProperties.isPaid, maybeRoomId.isPresent(), isExistingHigher, isExistingLower, isHigherPaid, isLowerPaid);
            if (codeset.addFailure(interiorDoorType == DOOR_ELEVATOR_OPEN_KEYED_BUTTON_CALLING, CODE_ELEVATOR_OPEN_KEYED_BUTTON_CALLING_CANNOT_BE_INTERIOR)) {
                isSuccess = false;
                return;
            }
            if (isElevatorPresent) {
                isSuccess &= maybeRoomId.isPresent() ? setSharedDoor(elevatorRoom, dir, interiorDoorType, exteriorDoorType, maybeRoomId) : setDoor(elevatorRoom, dir, interiorDoorType, maybeRoomId);
            } else {
                // elevator not present
                isSuccess &= maybeRoomId.map<bool>([&](const int& roomId2){
                    return setDoor(roomId2, dir.getFlip(), exteriorDoorType);
                }).orElse(true);
            }
        });
    }
    return isSuccess;
}

bool DungeonMutator::setupElevatorColumn(const RoomRef& elevatorRoom, const Rack<ElevatorProperties>& elevatorPropertyList) {
    bool isSuccess = true;

    bool isPaidFound = false;
    int i = 0;
    for (const auto& elevatorProperties: elevatorPropertyList) {
        const bool isElevatorPresent = !isPaidFound && elevatorProperties.isPaid;
        isPaidFound = isPaidFound || elevatorProperties.isPaid;

        const bool isExistingHigher = i != elevatorPropertyList.size() - 1;
        const bool isExistingLower = i != 0;
        const ElevatorProperties unpaidDefault = { .connectedRoomIds = {}, .isPaid = false };
        const bool isHigherPaid = isExistingHigher && elevatorPropertyList.getOrDefault(i + 1, unpaidDefault).isPaid;
        const bool isLowerPaid = isExistingLower && elevatorPropertyList.getOrDefault(i - 1, unpaidDefault).isPaid;

        setupElevatorLevel(elevatorRoom, elevatorProperties, isElevatorPresent, isExistingHigher, isExistingLower, isHigherPaid, isLowerPaid);

        i++;
    }

    return isSuccess && setRoom(elevatorRoom, ROOM_ELEVATOR);
}

bool DungeonMutator::setupElevatorColumn(int elevatorRoomId, const std::vector<int>& columnRoomIds, int paidIndex) {
    const auto EMPTY = Maybe<int>::empty();
    std::vector<ElevatorProperties> props;
    props.reserve(columnRoomIds.size());
    for (int i = 0; i < (int)columnRoomIds.size(); ++i) {
        props.push_back({
            .connectedRoomIds = Array<Maybe<int>, 4>({EMPTY, EMPTY, Maybe<int>(columnRoomIds[i]), EMPTY}),
            .isPaid = i == paidIndex
        });
    }
    return setupElevatorColumn(elevatorRoomId, Rack<ElevatorProperties>(props));
}

bool DungeonMutator::setupJailer (const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(room, dir, isKeyed ? DOOR_JAILER_INGRESS_KEYED : DOOR_JAILER_INGRESS_KEYLESS, isKeyed ? DOOR_JAILER_EGRESS_KEYED : DOOR_JAILER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_JAILER);
}

bool DungeonMutator::setupKeeper (const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(room, dir, isKeyed ? DOOR_KEEPER_INGRESS_KEYED : DOOR_KEEPER_INGRESS_KEYLESS, isKeyed ? DOOR_KEEPER_EGRESS_KEYED : DOOR_KEEPER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_KEEPER);
}

bool DungeonMutator::setupLadderUp(const RoomRef& bottomRoom, const Cardinal dir) {
    return setSharedShaftAbove(bottomRoom, dir, DOOR_LADDER_1_BOTTOM, DOOR_LADDER_1_TOP);
}

bool DungeonMutator::setupLightningRodRoom(const RoomRef& room, const bool isCubed, const bool isAwakened) {
    return setRoom(room, ROOM_LIGHTNING_ROD)
        && setDoor(room, ActivatorLightningRod::LIGHTNING_ROD_WALL, !isCubed ? DOOR_LIGHTNING_ROD_EMPTY : isAwakened ? DOOR_LIGHTNING_ROD_AWAKENED : DOOR_LIGHTNING_ROD_DORMANT);
}

bool DungeonMutator::setupPoleUp(const RoomRef& bottomRoom, const Cardinal dir) {
    return setSharedShaftAbove(bottomRoom, dir, DOOR_POLE_1_BOTTOM, DOOR_POLE_1_TOP);
}

bool DungeonMutator::setupPowerGeneratorRoom(const RoomRef& room) {
    return setRoom(room, ROOM_POWER_GENERATOR);
}

bool DungeonMutator::setupCovenantDoor (const RoomRef& room, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(room, dir, DOOR_COVENANT_CLOSED, DOOR_COVENANT_CLOSED), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_COVENANT);
}

bool DungeonMutator::setupShifter (const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(room, dir, isKeyed ? DOOR_SHIFTER_INGRESS_KEYED : DOOR_SHIFTER_INGRESS_KEYLESS, isKeyed ? DOOR_SHIFTER_EGRESS_KEYED : DOOR_SHIFTER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_SHIFTER);
}

bool DungeonMutator::setupTimeGateRoomToFuture(const RoomRef& presentRoom, const bool isCubed, const bool isAwakened) {
    if (!setRoom(presentRoom, ROOM_TIME_GATE_TO_FUTURE) || !setDoor(presentRoom, ActivatorTimeGate::TIME_GATE_DIRECTION, !isCubed ? DOOR_TIME_GATE_EMPTY : isAwakened ? DOOR_TIME_GATE_AWAKENED : DOOR_TIME_GATE_DORMANT))
        return false;
    return resolveRoom(presentRoom).map<bool>([&](const int& roomId) {
        return getRoom(roomId).map<bool>([&](Room& room1){
            const auto futureRoomId = room1.posterior;
            return setRoom(futureRoomId, ROOM_TIME_GATE_TO_PAST) && setDoor(futureRoomId, ActivatorTimeGate::TIME_GATE_DIRECTION, DOOR_TIME_GATE_EMPTY);
        }).orElse(false);
    }).orElse(false);
}

bool DungeonMutator::setupTogglerBlue (const RoomRef& room, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(room, dir, DOOR_TOGGLER_BLUE_CLOSED, DOOR_TOGGLER_BLUE_CLOSED), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_BLUE);
}

bool DungeonMutator::setupTogglerOrange (const RoomRef& room, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(room, dir, DOOR_TOGGLER_ORANGE_OPEN, DOOR_TOGGLER_ORANGE_OPEN), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_ORANGE);
}

bool DungeonMutator::allocateCharacter(const RoomRef& room, RoleEnum role, int& outCharacterId, int& outFloorId) {
    return resolveRoom(room).map<bool>([&](const int& roomId) {
        bool success = !codeset.addFailure(!controller.allocate(
            CharacterAllocSpec{.role = role},
            AttachmentContext::floor(roomId),
            outCharacterId
        ), CODE_DUNGEON_MUTATOR_FAILED_TO_ALLOCATE_CHARACTER);
        if (success) {
            controller.match.getCharacter(outCharacterId, codeset.error).access([&](const Character& character) {
                outFloorId = character.location.data;
            });
        }
        return success;
    }).orElse(false);
}

bool DungeonMutator::setupTogglerSwitch (const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return allocateCharacter(room, ROLE_TOGGLER_BLUE, outCharacterId, outFloorId);
}

bool DungeonMutator::setupTogglerSwitchBlue (const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return allocateCharacter(room, ROLE_TOGGLER_BLUE, outCharacterId, outFloorId);
}

bool DungeonMutator::setupTogglerSwitchOrange (const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return allocateCharacter(room, ROLE_TOGGLER_ORANGE, outCharacterId, outFloorId);
}

bool DungeonMutator::setupSacramentForgiveness (const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return allocateCharacter(room, ROLE_SACRAMENT_FORGIVENESS, outCharacterId, outFloorId);
}

bool DungeonMutator::setupHorizontalWalls(std::vector<DoorEnum> row, int y, int z) {
    if (row.size() != (size_t)size[0]) {
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0]; x++, it++) {
            const int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                continue;
            }

            found->second.access([&](Room& roomA) {
                Wall& wallA = roomA.getWall(Cardinal::north());
                layout->getWallNeighbor(dungeon.rooms, roomA, Cardinal::north()).access([&](Room& roomB) {
                    Wall& wallB = roomB.getWall(Cardinal::south());
                    wallA.door = *it;
                    wallB.door = *it;
                });
            });
        }
    }

    return true;
}

bool DungeonMutator::setupVerticalWalls(std::vector<DoorEnum> row, int y, int z) {
    if (row.size() != (size_t)(size[0] - 1)) {
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0] - 1; ++x, ++it) {
            int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                continue;
            }

            found->second.access([&](Room& roomA) {
                Wall& wallEast = roomA.getWall(Cardinal::east());
                layout->getWallNeighbor(dungeon.rooms, roomA, Cardinal::east()).access([&](Room& roomB) {
                    Wall& wallWest = roomB.getWall(Cardinal::west());
                    wallEast.door = *it;
                    wallWest.door = *it;
                });
            });
        }
    }

    return true;
}

bool DungeonMutator::setBuilderStartingRoomId(int builderIndex, const RoomRef& room) {
    return resolveRoom(room).map<bool>([&](const int& roomId) {
        bool isSuccess = false;
        controller.match.builders.access(builderIndex, [&](Builder& builder) {
            builder.startingRoomId = roomId;
            isSuccess = true;
        });
        return isSuccess;
    }).orElse(false);
}
