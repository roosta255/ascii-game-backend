#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "DungeonMutator.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RemodelTogglerBlue.hpp"

bool RemodelTogglerBlue::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter& counter, std::function<bool(const Match&)> acceptance) const {
    return counter.loopDoorways([&](const int& roomId, const Cardinal& dir){
        Match scratch = source;
        MatchController controller(scratch, params.codeset);
        DungeonMutator mutator(controller);
        mutator.setupTogglerBlue(roomId, dir);
        return acceptance(scratch);
    }, params.seed);
}

const Bitstick<REMODEL_COUNT>& RemodelTogglerBlue::getFixlist() const {
    static auto fixers = [&](){
        Bitstick<REMODEL_COUNT> bitstick;
        bitstick.setIndexOn(REMODEL_TOGGLER_SWITCH);
        return bitstick;
    }();
    return fixers;
}
