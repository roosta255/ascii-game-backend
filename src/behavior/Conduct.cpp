#include "Conduct.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorFlyweight.hpp"
#include "iProposer.hpp"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

bool Conduct::buildAndExecuteProposals(ActivationContext& ctx) {
    std::vector<std::pair<int, std::function<void(ActivationContext&)>>> proposals;

    for (int c = CONDUCT_NIL + 1; c < CONDUCT_COUNT; c++) {
        memory.accessConst(ConductEnum(c), [&](const ConductMemory& mem) {
            if (mem.state == BEHAVIOR_NIL) return;
            BehaviorFlyweight::getFlyweights().accessConst(mem.state, [&](const BehaviorFlyweight& fw) {
                fw.proposer.accessConst([&](const iProposer& p) {
                    p.buildProposal(ctx).access([&](auto& proposal) {
                        proposals.push_back(proposal);
                    });
                });
            });
        });
    }

    if (proposals.empty()) return false;

    auto best = std::max_element(proposals.begin(), proposals.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    best->second(ctx);
    return true;
}
