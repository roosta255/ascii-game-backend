#include "Array.hpp"
#include "ConductEnum.hpp"
#include "ConductFlyweight.hpp"

const Array<ConductFlyweight, CONDUCT_COUNT>& ConductFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<ConductFlyweight, CONDUCT_COUNT> flyweights;

        #define CONDUCT_DECL(name_) \
            flyweights.getPointer(CONDUCT_##name_).access([](ConductFlyweight& flyweight){ \
                flyweight.name = #name_; \
            });
        #include "Conduct.enum"
        #undef CONDUCT_DECL

        return flyweights;
    }();
    return flyweights;
}

bool ConductFlyweight::indexByString(const std::string& name, ConductEnum& output) {
    int i = 0;
    const auto& flyweights = getFlyweights();
    for (const ConductFlyweight& flyweight : flyweights) {
        if (name == flyweight.name) {
            output = (ConductEnum)i;
            return true;
        }
        i++;
    }
    return false;
}

const char* conduct_to_text(int index) {
    if (index < 0) {
        return "CONDUCT_INDEX_NEGATIVE";
    }
    if (index >= CONDUCT_COUNT) {
        return "CONDUCT_INDEX_OUT_OF_BOUNDS";
    }
    const char* result = "CONDUCT_UNKNOWN";
    ConductFlyweight::getFlyweights().accessConst(index, [&](const ConductFlyweight& flyweight){
        result = flyweight.name;
    });
    return result;
}
