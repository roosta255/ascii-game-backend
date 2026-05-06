#include "DungeonAuthor.hpp"
#include "GeneratorElevator.hpp"
#include "GeneratorRemodelGroupFlyweight.hpp"

const Array<GeneratorRemodelGroupFlyweight, REMODEL_GROUP_COUNT>& GeneratorRemodelGroupFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<GeneratorRemodelGroupFlyweight, REMODEL_GROUP_COUNT> flyweights;
        #define GENERATOR_DECL(...)
        #define GENERATOR_REMODEL_DECL(name_, ...) \
            flyweights.getPointer(REMODEL_GROUP_##name_).access([](GeneratorRemodelGroupFlyweight& fw){ \
                fw.remodels = std::vector<GeneratorRemodelVariant>({__VA_ARGS__}); \
            });
        #include "Generator.enum"
        #undef GENERATOR_DECL
        #undef GENERATOR_REMODEL_DECL
        return flyweights;
    }();
    return flyweights;
}
