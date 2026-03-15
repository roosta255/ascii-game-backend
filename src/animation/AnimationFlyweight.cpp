#include "AnimationFlyweight.hpp"
#include "AnimationTypes.hpp"
#include "Array.hpp"
#include <unordered_map>

const Array<AnimationFlyweight, ANIMATION_COUNT>& AnimationFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<AnimationFlyweight, ANIMATION_COUNT> flyweights;

        #define ANIMATION_DECL(name_, types_) \
            flyweights.getPointer( ANIMATION_##name_ ).access([&](AnimationFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.types = makeAnimationTypes types_; \
                flyweight.audio = ""; \
                flyweight.spritesheet = ""; \
            });
        #include "Animation.enum"
        #undef ANIMATION_DECL

        return flyweights;
    }();
    return flyweights;
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
