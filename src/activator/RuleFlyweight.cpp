#include "RuleFlyweight.hpp"
#include "ActivatorWrapper.hpp"
#include "Array.hpp"
#include "Codeset.hpp"
#include "RequestContext.hpp"
#include "RuleEnum.hpp"
#include "WrapperConfig.hpp"
#include <algorithm>

const Array<RuleFlyweight, RULE_COUNT>& RuleFlyweight::getFlyweights() {
    static auto flyweights = []() {
        Array<RuleFlyweight, RULE_COUNT> arr;
        int i = 0;
        #define RULE_DECL(name_, config_) \
            arr.access(i++, [](RuleFlyweight& fw) { \
                fw.name = #name_; \
                fw.config = WrapperConfig config_; \
            });
        #include "Rule.enum"
        #undef RULE_DECL
        return arr;
    }();
    return flyweights;
}

static const Array<ActivatorWrapper, RULE_COUNT>& getWrappers() {
    static auto wrappers = []() {
        static Array<WrapperConfig, RULE_COUNT> configs;

        int i = 0;
        #define RULE_DECL( name_, config_ ) \
            configs.access(i++, [](WrapperConfig& config) { \
                config = WrapperConfig config_; \
            });
        #include "Rule.enum"
        #undef RULE_DECL

        // Sort configs by priority descending so higher-priority rules are tried first
        std::sort(configs.begin(), configs.end(), [](const WrapperConfig& a, const WrapperConfig& b) {
            return a.priority > b.priority;
        });

        static Array<ActivatorWrapper, RULE_COUNT> wrappers;
        for (int j = 0; j < RULE_COUNT; j++) {
            configs.access(j, [&](WrapperConfig& config) {
                wrappers.access(j, [&](ActivatorWrapper& wrapper) {
                    wrapper.init(config);
                });
            });
        }

        return wrappers;
    }();
    return wrappers;
}

bool RuleFlyweight::tryActivate(Activation& activation) {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        const auto& wrappers = getWrappers();
        for (const ActivatorWrapper& wrapper : wrappers) {
            bool wasFailure = req.codeset.isAnyFailure;
            if (wrapper.activate(activation)) {
                result = true;
                return;
            }
            // A new failure means the rule matched but couldn't succeed — stop here
            if (req.codeset.isAnyFailure && !wasFailure) return;
            // No failure change means the rule didn't match — continue to next
        }
    });
    return result;
}
