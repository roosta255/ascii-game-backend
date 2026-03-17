#include "Item.hpp"
#include "ItemFlyweight.hpp"

bool Item::accessFlyweight(std::function<void(const ItemFlyweight&)> consumer)const {
    return ItemFlyweight::getFlyweights().accessConst(this->type, consumer);
}

bool Item::operator==(const Item& rhs)const {
    return stacks == rhs.stacks && type == rhs.type;
}

std::ostream& operator<<(std::ostream& os, const Item& rhs) {
    return os << "Item{.type:" << item_to_text(rhs.type) << ", .stacks:" << rhs.stacks << " }";
}
