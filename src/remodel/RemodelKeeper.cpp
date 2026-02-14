#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "DungeonMutator.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RemodelKeeper.hpp"

bool RemodelKeeper::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter& counter, std::function<bool(const Match&)> acceptance) const {
    return counter.loopDoorways([&](const int& roomId, const Cardinal& dir){
        Match scratch = source;
        MatchController controller(scratch, params.codeset);
        DungeonMutator mutator(controller);
        mutator.setupKeeper(roomId, dir, isKeyed);
        return acceptance(scratch);
    }, params.seed);
}

const Bitstick<REMODEL_COUNT>& RemodelKeeper::getFixlist() const {
    return iRemodel::getKeyProviders();
}

// RemodelKeeperKeyed
RemodelKeeperKeyed::RemodelKeeperKeyed() {
    isKeyed = true;
}

// overrides
const Bitstick<REMODEL_COUNT>& RemodelKeeperKeyed::getFixlist() const {
    return iRemodel::getEmpty();
}
