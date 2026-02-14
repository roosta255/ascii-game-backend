#pragma once

#include "iRemodel.hpp"
#include "Match.hpp"
#include "Maybe.hpp"

struct Remodel;

class RemodelTogglerBlue : public iRemodel {
public:
    bool mutateMatch(Remodel& params, const Match& source, const PathfindingCounter& counter, std::function<bool(const Match&)> acceptance) const override;
    const Bitstick<REMODEL_COUNT>& getFixlist() const override;
};
