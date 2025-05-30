#pragma once

#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "iLayout.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class LayoutGrid : public iLayout {
public:
    int width = 1;
    int height = 1;
    int depth = 1;
    int times = 1;
    Room& getEntrance(Array<Room, DUNGEON_ROOM_COUNT>& rooms) const override;
    Pointer<Room> getWallNeighbor(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal) const override;

    void setDimensions(int,int,int,int);

    constexpr int getIndex(int4 coords) const {
        return ((coords[3] * depth + coords[2]) * height + coords[1]) * width + coords[0];
    }

    constexpr int4 getCoordinates(int index) const {
        return int4{
            index % width,
            (index / width) % height,
            (index / (width * height)) % depth,
            (index / (width * height * depth))
        };
    }

    constexpr bool contains(int4 coords) const {
        return 0 <= coords[0] && coords[0] < width
            && 0 <= coords[1] && coords[1] < height
            && 0 <= coords[2] && coords[2] < depth
            && 0 <= coords[3] && coords[3] < times;
    }
};
