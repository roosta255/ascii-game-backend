#pragma once

#include <array>
#include <optional>
#include <sstream>

#include <catch2/catch_all.hpp>

#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"

// ── InventoryExpect ───────────────────────────────────────────────────────────
// Sparse specification: unspecified fields are wildcards that always pass.
// Build one with chained setters or direct aggregate initialization.
// Each entry asserts that the sum of items[i].stacks across every slot in
// Inventory::items[] whose type matches the index equals the expected value.

struct InventoryExpect {
    std::array<std::optional<int>, ITEM_COUNT> stackSum = {};

    InventoryExpect& expectStacks(ItemEnum type, int value) {
        stackSum[type] = value;
        return *this;
    }
};

// ── Sum helper ────────────────────────────────────────────────────────────────

inline int sumInventoryStacks(const Inventory& inventory, ItemEnum type) {
    int sum = 0;
    for (int i = 0; i < inventory.size; i++) {
        if (inventory.items[i].type == type) {
            sum += inventory.items[i].stacks;
        }
    }
    return sum;
}

// ── Full-state dump ───────────────────────────────────────────────────────────

inline std::string formatInventory(const Inventory& inventory) {
    std::ostringstream ss;
    ss << "Inventory{\n";
    for (int i = 0; i < inventory.size; i++) {
        const auto& item = inventory.items[i];
        if (item.type == ITEM_UNALLOCATED) continue;
        ss << "  [" << i << "] type=" << item_to_text(item.type) << " stacks=" << item.stacks << "\n";
    }
    ss << "}";
    return ss.str();
}

// Let Catch2 print the full slot dump whenever an Inventory appears in a REQUIRE_THAT expansion.
namespace Catch {
template<>
struct StringMaker<Inventory> {
    static std::string convert(const Inventory& inv) { return formatInventory(inv); }
};
} // namespace Catch

// ── Catch2 Matcher ────────────────────────────────────────────────────────────

class InventoryMatcher : public Catch::Matchers::MatcherBase<Inventory> {
    InventoryExpect _expect;
public:
    explicit InventoryMatcher(InventoryExpect expect) : _expect(std::move(expect)) {}

    bool match(const Inventory& actual) const override {
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (!_expect.stackSum[i]) continue;
            if (sumInventoryStacks(actual, ItemEnum(i)) != *_expect.stackSum[i])
                return false;
        }
        return true;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "satisfies InventoryExpect{\n";
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (!_expect.stackSum[i]) continue;
            ss << "  sum(stacks where type == " << item_to_text(i) << ") == " << *_expect.stackSum[i] << "\n";
        }
        ss << "}";
        return ss.str();
    }
};

inline InventoryMatcher MatchesInventoryExpect(InventoryExpect expect) {
    return InventoryMatcher(std::move(expect));
}
