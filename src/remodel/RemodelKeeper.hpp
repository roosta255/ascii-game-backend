#pragma once

#include "iRemodel.hpp"
#include "Match.hpp"
#include "Maybe.hpp"

struct Remodel;

class RemodelKeeper : public iRemodel {
protected:
// members
    bool isKeyed = false;
public:
// overrides
    bool mutateMatch(Remodel& params, const Match& source, const PathfindingCounter& counter, std::function<bool(const Match&)> acceptance) const override;
    const Bitstick<REMODEL_COUNT>& getFixlist() const override;
};

class RemodelKeeperKeyed : public RemodelKeeper {
public:
// constructors
    RemodelKeeperKeyed();
// overrides
    const Bitstick<REMODEL_COUNT>& getFixlist() const override;
};
