#include "Array.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "LayoutGrid.hpp"

const Array<LayoutFlyweight, LAYOUT_COUNT>& LayoutFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<LayoutFlyweight, LAYOUT_COUNT> flyweights;
        #define LAYOUT_DECL( name_text ) flyweights.getPointer( LAYOUT_##name_text ).access([](LayoutFlyweight& flyweight){ flyweight.name = #name_text; });
        #include "Layout.enum"
        #undef LAYOUT_DECL

        flyweights.getPointer(LAYOUT_2D_8x8).access([](LayoutFlyweight& flyweight){
            static LayoutGrid t;
            t.setDimensions(8, 8, 1, 1);
            flyweight.layout = t;
        });
        flyweights.getPointer(LAYOUT_3D_4x4x4).access([](LayoutFlyweight& flyweight){
            static LayoutGrid t;
            t.setDimensions(4, 4, 4, 1);
            flyweight.layout = t;
        });
        flyweights.getPointer(LAYOUT_4D_3x3x2).access([](LayoutFlyweight& flyweight){
            static LayoutGrid t;
            t.setDimensions(3, 3, 2, 2);
            flyweight.layout = t;
        });
        flyweights.getPointer(LAYOUT_7_LEVEL_TOWER).access([](LayoutFlyweight& flyweight){
            static LayoutGrid t;
            t.setDimensions(3, 3, 7, 1);
            flyweight.layout = t;
        });

        return flyweights;
    }();
    return flyweights;
}

bool LayoutFlyweight::indexByString (const std::string& name, int& output) {
    output = 0;
    const auto& flyweights = getFlyweights(); 
    for(const LayoutFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            return true;
        }
        output++;
    }
    return false;
}
