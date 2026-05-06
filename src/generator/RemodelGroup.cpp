#include "GeneratorRemodelGroupFlyweight.hpp"
#include "Match.hpp"
#include "Maybe.hpp"
#include "PathfindingCounter.hpp"
#include "RemodelGroup.hpp"

bool RemodelGroup::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const {
    bool allOk = false;
    GeneratorRemodelGroupFlyweight::getFlyweights().accessConst(group, [&](const GeneratorRemodelGroupFlyweight& fw) {
        Match current = source;
        for (const auto& v : fw.remodels) {
            bool ok = std::visit([&](const auto& r) -> bool {
                using T = std::decay_t<decltype(r)>;
                if constexpr (std::is_base_of_v<RemodelAuthorBase, T>) {
                    PathfindingCounter cnt{};
                    bool applied = false;
                    r.mutateMatch(params, current, cnt, [&](const Match& proposed) {
                        current = proposed;
                        applied = true;
                        return true;
                    });
                    return applied;
                } else {
                    Maybe<Match> result = r.buildMatch(params, current);
                    result.accessConst([&](const Match& m){ current = m; });
                    return !result.isEmpty();
                }
            }, v);
            if (!ok) return;
        }
        allOk = acceptance(current);
    });
    return allOk;
}
