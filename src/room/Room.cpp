#include "int2.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "Match.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>

Wall& Room::getWall (Cardinal dir) {
    // shouldn't be possible
    static Wall nil;
    Wall* result = &nil;
    walls.access(dir.getIndex(), [&](Wall& accessed){
        result = &accessed;
    });
    return *result;
}

const Wall& Room::getWall (Cardinal dir) const {
    // shouldn't be possible
    static Wall nil;
    const Wall* result = &nil;
    walls.accessConst(dir.getIndex(), [&](const Wall& accessed){
        result = &accessed;
    });
    return *result;
}

int Room::getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const {
    return this - &rooms.head();
}

Rack<Cell> Room::getUsedFloorCells() const {
    Rack<Cell> usedFloorCells(this->reserveFloorCells);
    RoomFlyweight::getFlyweights().accessConst(this->type, [&](const RoomFlyweight& flyweight) {
        usedFloorCells.resize(flyweight.width * flyweight.height);
    });
    return usedFloorCells;
}

Pointer<Cell> Room::getCell(int offset, CodeEnum& error) {
    Pointer<Cell> result = this->getUsedFloorCells().getPointer(offset);
    if (result.isEmpty()) {
        error = CODE_INACCESSIBLE_ROOM_FLOOR_CELL_ID;
    }
    return result;
}

void Room::forEachCharacter(Match& match, std::function<void(Character&)> consumer) {
    // Iterate over floor cells
    for (Cell& cell : this->getUsedFloorCells()) {
        CodeEnum error;
        match.getCharacter(cell.offset, error).access([&](Character& character){
            consumer(character);
        });
    }

    // Iterate over wall cells
    for (Wall& wall : walls) {
        CodeEnum error;
        match.getCharacter(wall.cell.offset, error).access([&](Character& character){
            consumer(character);
        });
    }
}

bool Room::containsCharacter(int offset) const {
    for (const auto& cell: this->getUsedFloorCells()) {
        if (cell.offset == offset) {
            return true;
        }
    }
    for (const auto& wall: walls) {
        if (wall.cell.offset == offset) {
            return true;
        }
    }
    return false;
}

bool Room::containsFloorCell(const Cell& cell, CodeEnum& error, int& index, int2& coords) const {
    bool isContains = false;
    if (this->reserveFloorCells.containsAddress(cell, index)) {
        RoomFlyweight::getFlyweights().accessConst(this->type, [&](const RoomFlyweight& flyweight) {
            const int x = index % flyweight.width;
            const int y = index / flyweight.width;
            if (y < flyweight.height) {
                coords = int2{ x,  y};
                isContains = true;
            } else {
                error = CODE_UNUSED_CELL_ID;
            }
        });
    }

    if (isContains) {
        return true;
    }

    error = CODE_INACCESSIBLE_ROOM_FLOOR_CELL_ID;
    return false;
}

// REMOVE: may no longer be needed
void Room::setType(const RoomEnum& input) {
    this->type = input;
}
