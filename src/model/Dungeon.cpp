#include "Dungeon.hpp"
#include "iLayout.hpp"
#include "int2.hpp"
#include "int4_to_json.hpp"
#include "JsonParameters.hpp"
#include "LayoutFlyweight.hpp"
#include "DoorEnum.hpp"
#include "RoleEnum.hpp"
#include "CodeEnum.hpp"
#include <json/json.h>

bool Dungeon::accessLayoutFlyweight(CodeEnum& error, std::function<void(const LayoutFlyweight&)> consumer) const
{
    const bool isAccessed = LayoutFlyweight::getFlyweights().accessConst(layout, [&](const LayoutFlyweight& flyweight){
        consumer(flyweight);
    });
    if (!isAccessed) {
        error = CODE_INACCESSIBLE_LAYOUT;
    }
    return isAccessed;
}

bool Dungeon::accessLayout(CodeEnum& error, std::function<void(const iLayout&)> consumer) const
{
    bool isAccessed = false;
    accessLayoutFlyweight(error, [&](const LayoutFlyweight& flyweight){
        const bool isLayoutAccessed = flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            isAccessed = true;
            consumer(layoutIntf);
        });
        if (!isLayoutAccessed) {
            error = CODE_INACCESSIBLE_LAYOUT;
        }
    });
    return isAccessed;
}

bool Dungeon::findCharacter(
    Room& room,
    int searched,
    std::function<void(Cell&, const Cardinal, Room&, Cell&)> wallConsumer,
    std::function<void(int, int, Cell&)> floorConsumer
)
{
    CodeEnum error;
    bool isFound = false;

    // Search floor cells
    for (Cell& cell: room.getUsedFloorCells()) {
        int index;
        int2 coords;
        if (cell.offset == searched) {
            if (room.containsFloorCell(cell, error, index, coords))
            {
                isFound = true;
                floorConsumer(coords[0], coords[1], cell);
            } else {
                error = CODE_DUNGEON_FIND_CHARACTER_LOOPS_OVER_UNCONTAINED_FLOOR_CELL;
            }
        }
    };

    for (int y = 0; y < Room::DUNGEON_ROOM_HEIGHT; ++y) {
        for (int x = 0; x < Room::DUNGEON_ROOM_WIDTH; ++x) {
            int i = y * Room::DUNGEON_ROOM_WIDTH + x;
        }
    }

    // Search wall cells
    for (Cardinal dir: Cardinal::getAllCardinals()) {
        Wall& wall = room.getWall(dir);

        if (wall.cell.offset == searched) {
            isFound = true;
            this->rooms.access(room.getWall(dir).adjacent, [&](Room& neighbor) {
                Cell& otherCell = neighbor.getWall(dir.getFlip()).cell;
                if (otherCell.offset == searched) {
                    wallConsumer(wall.cell, dir, neighbor, otherCell);
                }
            });
        }
    }

    return isFound;
}

void Dungeon::toggleDoors() {
    isBlueOpen = !isBlueOpen;
    
    // Iterate through all rooms and their walls
    for (Room& room : rooms) {
        for (Cardinal dir : Cardinal::getAllCardinals()) {
            Wall& wall = room.getWall(dir);
            
            // Update door types based on isBlueOpen
            if (wall.door == DOOR_TOGGLER_BLUE_OPEN || wall.door == DOOR_TOGGLER_BLUE_CLOSED) {
                wall.door = isBlueOpen ? DOOR_TOGGLER_BLUE_OPEN : DOOR_TOGGLER_BLUE_CLOSED;
            }
            else if (wall.door == DOOR_TOGGLER_ORANGE_OPEN || wall.door == DOOR_TOGGLER_ORANGE_CLOSED) {
                wall.door = isBlueOpen ? DOOR_TOGGLER_ORANGE_CLOSED : DOOR_TOGGLER_ORANGE_OPEN;
            }
        }
    }
}

bool Dungeon::containsRoom(const Room& room, int& roomId) const {
    // Get base address of rooms array
    auto base = reinterpret_cast<const char*>(&rooms.head());
    
    // Get address of the source room
    auto ptr = reinterpret_cast<const char*>(&room);
    
    // Verify that the room lies within the rooms array
    auto end = base + (sizeof(rooms));
    if (ptr >= base && ptr < end) {
        // Calculate room index
        roomId = static_cast<int>((ptr - base) / sizeof(Room));
        return true;
    }
    return false;
}

Pointer<Room> Dungeon::getRoom(int roomId, CodeEnum& error) {
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) {
        error = CODE_ROOM_ID_OUT_OF_BOUNDS;
        return Pointer<Room>::empty();
    }
    return rooms.getPointer(roomId);
}

Pointer<const Room> Dungeon::getRoom(int roomId, CodeEnum& error) const {
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) {
        error = CODE_ROOM_ID_OUT_OF_BOUNDS;
        return Pointer<const Room>::empty();
    }
    return rooms.getPointer(roomId);
}

bool Dungeon::accessWall(
    Room& source,
    Cardinal dir,
    std::function<void(Cell&, Cell&, Room&)> neighborCallback,
    std::function<void(Cell&)> noNeighborCallback
) {
    CodeEnum error;
    // Get the source wall cell
    Cell& sourceCell = source.getWall(dir).cell;

    // Try to get the neighboring room
    bool success = false;
    const bool isAccessed = this->rooms.access(source.getWall(dir).adjacent, [&](Room& neighbor) {
        // If we have a neighbor, get its corresponding wall cell
        Cell& neighborCell = neighbor.getWall(dir.getFlip()).cell;
        neighborCallback(sourceCell, neighborCell, neighbor);
        success = true;
    });

    if (!isAccessed) {
        // If no neighbor, just handle the source cell
        noNeighborCallback(sourceCell);
        success = true;
    }

    return success;
}

bool Dungeon::accessWall(
    Room& source,
    Cardinal dir,
    std::function<void(Cell&)> callback
) {
    return this->accessWall(source, dir, [&](Cell& sourceCell, Cell&, Room&){callback(sourceCell);}, callback);
}

bool Dungeon::accessWall(
    const Room& source,
    Cardinal dir,
    std::function<void(const Cell&, const Cell&, const Room&)> neighborCallback,
    std::function<void(const Cell&)> noNeighborCallback
) const
{
    CodeEnum error;
    // Get the source wall cell
    const Cell& sourceCell = source.getWall(dir).cell;

    // Try to get the neighboring room
    bool success = false;
    const bool isAccessed = this->rooms.accessConst(source.getWall(dir).adjacent, [&](const Room& neighbor) {
        const Cell& neighborCell = neighbor.getWall(dir.getFlip()).cell;
        neighborCallback(sourceCell, neighborCell, neighbor);
        success = true;
    });

    if (!isAccessed) {
        // If no neighbor, just handle the source cell
        noNeighborCallback(sourceCell);
        success = true;
    }

    return success;
}
