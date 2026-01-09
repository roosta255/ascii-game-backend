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

bool Inventory::giveItem(ItemEnum type, CodeEnum& result, const bool isDryRun)
{
    if (type == ITEM_NIL) {
        result = CODE_PUSHED_ITEM_WITH_NIL_TYPE;
        return false;
    }
    int capacity = 0;
    if (!ItemFlyweight::getFlyweights().accessConst(type, [&](const ItemFlyweight& flyweight){ capacity = flyweight.stacks; })) {
        result = CODE_PUSHED_ITEM_WITH_INVALID_TYPE;
        return false;
    }

    if (capacity == 0) {
        result = CODE_PUSHED_ITEM_WITH_0_CAPACITY_TYPE;
        return false;
    }

    for(auto& item : this->items)
    {
        if (type == item.type && item.stacks < capacity) {
            if (!isDryRun) {
                item.stacks += 1;
            }
            return true;
        }
    }

    for(auto& item : this->items)
    {
        if (item.type == ITEM_NIL) {
            if (!isDryRun) {
                item.type = type;
                item.stacks += 1;
            }
            return true;
        }
    }

    switch (type) {
        case ITEM_KEY:
            result = CODE_INVENTORY_FAILED_TO_ACCEPT_KEY;
            break;
        default:
            result = CODE_INVENTORY_FAILED_TO_ACCEPT_ITEM;
            break;
    }
    return false;
}

bool Inventory::takeItem(ItemEnum type, CodeEnum& result, const bool isDryRun)
{
    if (type == ITEM_NIL) {
        result = CODE_TAKEN_ITEM_WITH_NIL_TYPE;
        return false;
    }
    int capacity = 0;
    if (!ItemFlyweight::getFlyweights().accessConst(type, [&](const ItemFlyweight& flyweight){ capacity = flyweight.stacks; })) {
        result = CODE_TAKEN_ITEM_WITH_INVALID_TYPE;
        return false;
    }

    if (capacity == 0) {
        result = CODE_TAKEN_ITEM_WITH_0_CAPACITY_TYPE;
        return false;
    }

    for(auto& item : this->items)
    {
        if (type == item.type) {
            if (!isDryRun) {
                if (item.stacks > 1) {
                    item.stacks -= 1;
                } else {
                    item = Item();
                }
            }
            return true;
        }
    }

    switch (type) {
        case ITEM_KEY:
            result = CODE_INVENTORY_FAILED_TO_HAVE_KEY;
            break;
        default:
            result = CODE_INVENTORY_FAILED_TO_HAVE_ITEM;
            break;
    }
    return false;
}

bool Inventory::processDelta(ItemEnum type, const bool delta, CodeEnum& result, const bool isDryRun) {
    if (delta) {
        return giveItem(type, result, isDryRun);
    } else {
        return takeItem(type, result, isDryRun);
    }
}

bool Inventory::accessItem(int index, CodeEnum& result, std::function<void(Item&)> consumer) {
    if (this->items.access(index, consumer)) {
        return true;
    }
    result = CODE_INVENTORY_ITEM_INDEX_IS_INNACCESSIBLE;
    return false;
}
