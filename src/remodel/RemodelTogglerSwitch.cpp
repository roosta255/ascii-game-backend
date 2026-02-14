#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "DungeonMutator.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RemodelTogglerSwitch.hpp"

bool RemodelTogglerSwitch::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter& counter, std::function<bool(const Match&)> acceptance) const {
    return counter.loopRooms([&](const int& roomId){
        Match scratch = source;
        MatchController controller(scratch, params.codeset);
        DungeonMutator mutator(controller);
        int outCharacterId, outFloorId;
        mutator.setupTogglerSwitch(roomId, outCharacterId, outFloorId);
        return acceptance(scratch);
    }, params.seed);
}

const Bitstick<REMODEL_COUNT>& RemodelTogglerSwitch::getFixlist() const {
    static auto fixers = [&](){
        Bitstick<REMODEL_COUNT> bitstick;   
        return bitstick;
    }();
    return fixers;
}
