#include "Array.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "LayoutGrid.hpp"

const Array<LayoutFlyweight, LAYOUT_COUNT>& LayoutFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<LayoutFlyweight, LAYOUT_COUNT> flyweights;
        #define LAYOUT_DECL( name_text, x__, y__, z__, t__ ) flyweights.getPointer( LAYOUT_##name_text ).access([](LayoutFlyweight& flyweight){ \
            static LayoutGrid layout(x__, y__, z__, t__); \
            flyweight.name = #name_text; \
            flyweight.layout = layout; \
        });
        #include "Layout.enum"
        #undef LAYOUT_DECL

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
