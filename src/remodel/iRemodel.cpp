#include "Codeset.hpp"
#include "generateRandomArray.hpp"
#include "iRemodel.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include "Remodel.hpp"
#include "RemodelFlyweight.hpp"

/*
bool iRemodel::findRemodel(const Bitstick<REMODEL_COUNT>& list) {
    for (int i = 0; i < REMODEL_COUNT && result.isEmpty(); i++) {
        if (fixers[i]) {
            RemodelFlyweight::getFlyweights().accessConst(i, [&](const RemodelFlyweight& flyweight){
            });
        }
    }
}
*/
Maybe<Match> iRemodel::buildMatch(Remodel& params, const Match& source) const {
    static const auto REMODEL_IDS = generateRandomArray<REMODEL_COUNT>(3777437);
    // if (depth == 0) {
    //     return Maybe<Match>::empty();
    // }
    auto& codeset = params.codeset;

    // solve it
    codeset.addTable(CODE_REMODEL_BUILD_MATCH_INITIAL_A_STAR_CALL);
    const auto counter = Pathfinder::build(params.playerId, params.characterId, params.bossRoomId, source, params.codeset).getCounter();

    // we know where to put it,
    Maybe<Match> result;
    this->mutateMatch(params, source, counter, [&](const Match& proposed){
        codeset.addTable(CODE_REMODEL_BUILD_MATCH_POST_BREAKER_A_STAR_CALL);
        const auto path2 = Pathfinder::build(params.playerId, params.characterId, params.bossRoomId, proposed, params.codeset);
        const auto counter2 = path2.getCounter();

        if (path2.isSolved) {
            codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_BREAKER_AND_SOLVED);
            // good to go, try and accept
            if (params.acceptance(proposed, counter2)) {
                // solved and acceptible, this is the solution
                codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_BREAKER_AND_ACCEPTED);
                result = proposed;
                return true;
            } else {
                codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_BREAKER_BUT_REJECTED);
                // solved, but unnacceptable
                return false;
            }
        } else {
            codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_BREAKER_BUT_UNSOLVED);
            // broken, will need fixing
            const auto fixlist = this->getFixlist();
            const auto fixers = params.allowlist.map<Bitstick<REMODEL_COUNT>>(
                [&](const Bitstick<REMODEL_COUNT>& allowlist){
                    return allowlist & fixlist;
                }).orElse(fixlist) - params.blocklist.orElse(Bitstick<REMODEL_COUNT>());
            for (int r = 0; r < REMODEL_COUNT && result.isEmpty(); r++) {
                const auto remodel_id = REMODEL_IDS.getWindow(r + params.seed);
                if (fixers[remodel_id]) {
                    RemodelFlyweight::getFlyweights().accessConst(remodel_id, [&](const RemodelFlyweight& flyweight){
                        flyweight.remodel.accessConst([&](const iRemodel& fixerRemodel){
                            // result = fixerRemodel.buildMatch(params, proposed);
                            fixerRemodel.mutateMatch(params, proposed, counter2, [&](const Match& amended){
                                codeset.addTable(CODE_REMODEL_BUILD_MATCH_POST_FIXER_A_STAR_CALL);
                                const auto path3 = Pathfinder::build(params.playerId, params.characterId, params.bossRoomId, amended, params.codeset);
                                const auto counter3 = path3.getCounter();
                                if (path3.isSolved) {
                                    // good to go, try and accept
                                    if (params.acceptance(amended, counter3)) {
                                        // solved and acceptible, this is the solution
                                        codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_FIXER_AND_ACCEPTED);
                                        result = amended;
                                        return true;
                                    } else {
                                        // solved, but unnacceptable, continue
                                        codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_FIXER_BUT_REJECTED);
                                        return false;
                                    }
                                } else {
                                    // the fixer didn't work, continue
                                    codeset.addTable(CODE_REMODEL_BUILD_MATCH_WITH_FIXER_BUT_UNSOLVED);
                                    return false;
                                }
                            });
                        });
                    });
                }
            }
        }
        return false;
    });

    return result;
}

const Bitstick<REMODEL_COUNT>& iRemodel::getEmpty() {
    static Bitstick<REMODEL_COUNT> result;
    return result;
}

const Bitstick<REMODEL_COUNT>& iRemodel::getKeyProviders() {
    static Bitstick<REMODEL_COUNT> result({REMODEL_KEEPER_KEYED, REMODEL_SHIFTER_KEYED});
    return result;
}
