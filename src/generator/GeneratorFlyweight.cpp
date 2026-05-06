#include "Array.hpp"
#include "Builder.hpp"
#include "Codeset.hpp"
#include "GeneratorAtom.hpp"
#include "GeneratorDoorwayDungeon1.hpp"
#include "DungeonAuthor.hpp"
#include "GeneratorElevator.hpp"
#include "GeneratorDoorwayDungeon3Traits.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorFlyweight.hpp"
#include "GeneratorTest.hpp"
#include "GeneratorTest4D.hpp"
#include "GeneratorTutorial.hpp"
#include "GeneratorPuzzle2.hpp"
#include "iLayout.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "PathfindingCounter.hpp"
#include "Remodel.hpp"
#include "RemodelAuthor.hpp"

const Array<GeneratorFlyweight, GENERATOR_COUNT>& GeneratorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<GeneratorFlyweight, GENERATOR_COUNT> flyweights;
        #define GENERATOR_DECL( name_, class_, isTest_, layout_, bossRoom_, ... ) \
            flyweights.getPointer( GENERATOR_##name_ ).access([](GeneratorFlyweight& fw){ \
                fw.name = #name_; \
                fw.isTest = isTest_; \
                static class_ t; \
                fw.generator = t; \
                fw.layout = layout_; \
                fw.bossRoomId = bossRoom_; \
                fw.remodels = std::vector<GeneratorRemodelVariant>({__VA_ARGS__}); \
            });
        #define GENERATOR_REMODEL_DECL(...)
        #include "Generator.enum"
        #undef GENERATOR_DECL
        #undef GENERATOR_REMODEL_DECL

        return flyweights;
    }();
    return flyweights;
}

bool GeneratorFlyweight::buildMatch(int seed, Match& dst, Codeset& codeset) const {
    bool success = false;

    LayoutFlyweight::getFlyweights().accessConst(this->layout, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            dst.dungeon.layout = this->layout;
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
        });
    });
    generator.accessConst([&](const iGenerator& gen){
        success = gen.generate(seed, dst, codeset);
    });
    if (!success || remodels.empty())
        return success;

    std::string playerId;
    int builderId = -1;
    dst.builders.access(0, [&](Builder& builder){
        playerId = builder.player.account.toString();
        dst.containsCharacter(builder.character, builderId);
    });
    if (builderId < 0)
        return false;

    Remodel params {
        .playerId   = playerId,
        .characterId = builderId,
        .bossRoomId = bossRoomId,
        .seed       = seed,
        .codeset    = codeset,
        .acceptance = [](const Match&, const PathfindingCounter&){ return true; }
    };

    for (const auto& v : remodels) {
        bool ok = std::visit([&](const auto& r) -> bool {
            using T = std::decay_t<decltype(r)>;
            if constexpr (std::is_base_of_v<RemodelAuthorBase, T>) {
                PathfindingCounter counter{};
                bool applied = false;
                r.mutateMatch(params, dst, counter, [&](const Match& proposed) {
                    dst = proposed;
                    applied = true;
                    return true;
                });
                return applied;
            } else {
                Maybe<Match> result = r.buildMatch(params, dst);
                result.accessConst([&](const Match& m){ dst = m; });
                return !result.isEmpty();
            }
        }, v);
        if (!ok) return false;
    }
    return true;
}

bool GeneratorFlyweight::indexByString (const std::string& name, int& output) {
    output = 0;
    const auto& flyweights = getFlyweights();
    for(const GeneratorFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            return true;
        }
        output++;
    }
    return false;
}
