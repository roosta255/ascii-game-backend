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
    LayoutGrid() = default;
    constexpr LayoutGrid(const int4& data): width(data[0]), height(data[1]), depth(data[2]), times(data[3]) {}

    int width = 1;
    int height = 1;
    int depth = 1;
    int times = 1;
    Room& getEntrance(Array<Room, DUNGEON_ROOM_COUNT>& rooms) const override;
    bool getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir, int& offset) const override;
    bool getTimeDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const override;
    bool getDepthDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const override;

    bool getDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const int4& delta, int& offset) const;

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

static_assert(LayoutGrid(int4{3,3,2,2}).getIndex(int4{1,1,0,1}) == 22, "");
static_assert(LayoutGrid(int4{3,3,2,2}).getCoordinates(22) == int4{1,1,0,1}, "");
static_assert(LayoutGrid(int4{3,3,2,2}).contains(int4{1,1,0,1}), "");
