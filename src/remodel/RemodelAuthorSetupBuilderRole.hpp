#pragma once

#include "RemodelAuthorBase.hpp"

struct RemodelAuthorSetupBuilderRole : public RemodelAuthorBase {
    int builderIndex;
    explicit RemodelAuthorSetupBuilderRole(int builderIndex) : builderIndex(builderIndex) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
