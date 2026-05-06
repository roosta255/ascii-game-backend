#pragma once

#include "RemodelAuthorBase.hpp"

struct RemodelAuthorRunGeneratorLoop : public RemodelAuthorBase {
    int iterations;
    explicit RemodelAuthorRunGeneratorLoop(int iters) : iterations(iters) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
