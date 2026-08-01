#include "FieldFlyweight.hpp"
#include "OxygenSourceProvider.hpp"
#include "ROOM_ACCUMULATIONFieldResolver.hpp"

const Array<FieldFlyweight, FIELD_COUNT>& FieldFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<FieldFlyweight, FIELD_COUNT> flyweights;

        #define FIELD_DECL( name_, resolver_, provider_, mask_, min_, max_ ) \
            static resolver_##FieldResolver GLOBAL_##name_##_resolver; \
            static provider_ GLOBAL_##name_##_provider; \
            flyweights.getPointer( FIELD_##name_ ).access([&](FieldFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.resolver = GLOBAL_##name_##_resolver; \
                flyweight.sourceProvider = GLOBAL_##name_##_provider; \
                flyweight.invalidationMask = mask_; \
                flyweight.minValue = min_; flyweight.maxValue = max_; \
            });
        #include "Field.enum"
        #undef FIELD_DECL

        return flyweights;
    }();
    return flyweights;
}
