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

bool RuleFlyweight::tryActivate(ActivationContext& ctx) {
    bool result = false;
    ctx.request.access([&](RequestContext& req) {
        const auto& wrappers = getWrappers();
        bool anyMatched = false;
        for (const ActivatorWrapper& wrapper : wrappers) {
            bool wasFailure = ctx.codeset.isAnyFailure;
            if (wrapper.activate(ctx)) {
                result = true;
                anyMatched = true;
                break;
            }
            if (ctx.codeset.isAnyFailure && !wasFailure) {
                // Rule matched but failed — wrapper already logged specific codes
                anyMatched = true;
                break;
            }
            // else: rule didn't match — try the next one
        }
        if (!anyMatched) {
            ctx.codeset.addFailure(true, CODE_RULE_EXECUTION_FAILED);
        }
    });
    return result;
}
