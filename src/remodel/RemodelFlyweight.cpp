#include "Array.hpp"
#include "RemodelFlyweight.hpp"
#include "RemodelJailer.hpp"
#include "RemodelKeeper.hpp"
#include "RemodelShifter.hpp"
#include "RemodelTogglerBlue.hpp"
#include "RemodelTogglerOrange.hpp"
#include "RemodelTogglerSwitch.hpp"

const Array<RemodelFlyweight, REMODEL_COUNT>& RemodelFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<RemodelFlyweight, REMODEL_COUNT> flyweights;
        #define REMODEL_DECL( name_, class_, is_blocker_ ) flyweights.getPointer( REMODEL_##name_ ).access([](RemodelFlyweight& flyweight){ \
            flyweight.name = #name_; \
            static class_ remodel; \
            flyweight.remodel = remodel; \
            flyweight.isBlocker = is_blocker_; \
        });
        #include "Remodel.enum"
        #undef REMODEL_DECL

        return flyweights;
    }();
    return flyweights;
}

bool RemodelFlyweight::indexByString
(
    const std::string& name,
    RemodelEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, RemodelEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const RemodelFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = RemodelEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
