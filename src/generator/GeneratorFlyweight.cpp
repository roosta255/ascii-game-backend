#include "Array.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorFlyweight.hpp"
#include "GeneratorTest.hpp"
#include "GeneratorTest4D.hpp"
#include "GeneratorTutorial.hpp"
#include "GeneratorPuzzle2.hpp"

const Array<GeneratorFlyweight, GENERATOR_COUNT>& GeneratorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<GeneratorFlyweight, GENERATOR_COUNT> flyweights;
        #define GENERATOR_DECL( name_, class_ ) flyweights.getPointer( GENERATOR_##name_ ).access([](GeneratorFlyweight& flyweight){ flyweight.name = #name_; static class_ t; flyweight.generator = t; });
        #include "Generator.enum"
        #undef GENERATOR_DECL

        return flyweights;
    }();
    return flyweights;
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
