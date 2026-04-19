#include "AnimationFlyweight.hpp"
#include "AnimationTypes.hpp"
#include "Array.hpp"
#include <unordered_map>

const Array<AnimationFlyweight, ANIMATION_COUNT>& AnimationFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<AnimationFlyweight, ANIMATION_COUNT> flyweights;

        #define ANIMATION_DECL(name_, types_, duration_, data_) \
            flyweights.getPointer( ANIMATION_##name_ ).access([&](AnimationFlyweight& flyweight){ \
                flyweight.index = ANIMATION_##name_; \
                flyweight.name = #name_; \
                flyweight.baseDuration = duration_; \
                flyweight.types = makeAnimationTypes types_; \
                flyweight.data = Array<AnimationSemantic, KEYFRAME_DATA_ARRAY_SIZE> data_; \
                for (const AnimationSemantic semantic : flyweight.data) { \
                    if (semantic != ANIMATION_SEMANTIC_NONE) \
                        flyweight.semantics.setIndexOn((size_t)semantic_to_type(semantic)); \
                } \
            });
        #include "Animation.enum"
        #undef ANIMATION_DECL

        return flyweights;
    }();
    return flyweights;
}

Maybe<AnimationEnum> AnimationFlyweight::queryAnimation(const AnimationEnum& animation, const AnimationTypes& semantics) {
    Maybe<AnimationEnum> result;
    getFlyweights().getPointer((int)animation).accessConst([&](const AnimationFlyweight& sourceFlyweight){
        const auto target = sourceFlyweight.types | semantics;
        for (const auto& flyweight: getFlyweights()) {
            const auto types = flyweight.types | flyweight.semantics;
            if (types == target) {
                result = (AnimationEnum)flyweight.index;
                return;
            }
        }
    });
    return result;
}

bool AnimationFlyweight::indexByString
(
    const std::string& name,
    AnimationEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, AnimationEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for (const AnimationFlyweight& flyweight : flyweights) {
            nameMap[flyweight.name] = AnimationEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
