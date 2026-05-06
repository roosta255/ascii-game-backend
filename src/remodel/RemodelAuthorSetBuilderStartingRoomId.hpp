#pragma once

#include "RemodelAuthorBase.hpp"

struct RemodelAuthorSetBuilderStartingRoomId : public RemodelAuthorBase {
    int builderIndex;
    int4 coords;
    RemodelAuthorSetBuilderStartingRoomId(int builderIndex, int4 coords) : builderIndex(builderIndex), coords(coords) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
