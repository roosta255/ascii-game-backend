#pragma once

#include "GeneratorRemodelGroupEnum.hpp"
#include "RemodelAuthor.hpp"

// Executes all GeneratorRemodelVariants registered under a GENERATOR_REMODEL_DECL name.
struct RemodelGroup : public RemodelAuthorBase {
    GeneratorRemodelGroupEnum group;
    explicit RemodelGroup(GeneratorRemodelGroupEnum g) : group(g) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
