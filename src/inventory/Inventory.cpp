#include "Inventory.hpp"
#include "ItemFlyweight.hpp"
#include "min.hpp"

InventoryDigest Inventory::makeDigest() const
{
    InventoryDigest digest {
        .keys = 0,
        .isEmpty = true
    };
    for(const auto& item : this->items)
    {
        if (digest.isEmpty && item.type != ITEM_NIL) {
            digest.isEmpty = false;
        }
        if (item.type == ITEM_KEY) {
            digest.keys += item.stacks;
        }
    }
    return digest;
}

bool Inventory::accessItem(ItemEnum target, std::function<void(Item&)> consumer)
{
    for(auto& item : this->items)
    {
        if (target == item.type) {
            consumer(item);
            return true;
        }
    }
    return false;
}

bool Inventory::accessItem(ItemEnum target, std::function<void(const Item&)> consumer) const
{
    for(const auto& item : this->items)
    {
        if (target == item.type) {
            consumer(item);
            return true;
        }
    }
    return false;
}

bool Inventory::pushItem(Item& src)
{
    if (src.stacks == 0) {
        return true;
    }

    int capacity = 0;
    if (!src.accessFlyweight([&](const ItemFlyweight& flyweight){ capacity = flyweight.stacks; })) {
        return false;
    }

    for(auto& item : this->items)
    {
        if (src.type == item.type) {
            const int opening = capacity - item.stacks;
            const int available = src.stacks;
            const int moving = min(opening, available);
            item.stacks += moving;
            src.stacks -= moving;
        }
        if (src.stacks == 0) {
            return true;
        }
    }
    
    return false;
}
