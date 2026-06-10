#pragma once

#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "iRemodel.hpp"
#include "int4.hpp"
#include "ItemEnum.hpp"
#include "RoleEnum.hpp"
#include <functional>
#include <vector>

class DungeonMutator;
struct Remodel;

// Base marker for all DungeonMutator-derived remodels.
// GeneratorFlyweight::buildMatch uses this to apply them without pathfinding checks,
// since they are structural setup operations rather than puzzle mutations.
class RemodelAuthorBase : public iRemodel {
protected:
    static bool withAuthor(
        Remodel& params,
        const Match& source,
        std::function<bool(DungeonMutator&)> body,
        std::function<bool(const Match&)> acceptance);
public:
    const Bitstick<REMODEL_COUNT>& getFixlist() const override { return iRemodel::getEmpty(); }
};
