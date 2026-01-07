#include "Item.hpp"
#include "ItemFlyweight.hpp"

bool Item::accessFlyweight(std::function<void(const ItemFlyweight&)> consumer)const {
    return ItemFlyweight::getFlyweights().accessConst(this->type, consumer);
}
