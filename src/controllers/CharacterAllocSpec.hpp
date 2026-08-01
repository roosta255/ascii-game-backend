#pragma once

#include "ChannelEnum.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Maybe.hpp"
#include "RoleEnum.hpp"
#include <variant>
#include <vector>

struct ConductSpec {};

// Forward-declared so ChestAllocSpec can hold vector<InventoryEntrySpec> before
// InventoryEntrySpec is fully defined (C++17 supports incomplete element types in vector).
struct InventoryEntrySpec;

struct ChestAllocSpec {
    LockEnum lock = LOCK_NIL;
    std::vector<InventoryEntrySpec> inventory;
};

struct CharacterAllocSpec {
    RoleEnum role = ROLE_EMPTY;
    Maybe<ConductSpec> conduct = Maybe<ConductSpec>::empty();
    Maybe<ChestAllocSpec> chest = Maybe<ChestAllocSpec>::empty();
};

// ── inventory entry alternatives ─────────────────────────────────────────────

struct ItemSpec {
    ItemEnum item = ITEM_NIL;
    int stacks = 1;
};

struct ContainedCharacterSpec {
    CharacterAllocSpec character;
};

struct CritterCharacterSpec {
    CharacterAllocSpec character;
};

// InventoryEntrySpec inherits from the variant so it can be forward-declared
// as a struct above, which lets vector<InventoryEntrySpec> appear in ChestAllocSpec.
struct InventoryEntrySpec : std::variant<ItemSpec, ContainedCharacterSpec, CritterCharacterSpec> {
    using base_t = std::variant<ItemSpec, ContainedCharacterSpec, CritterCharacterSpec>;
    using base_t::base_t;
    using base_t::operator=;
};

// ── attachment context ────────────────────────────────────────────────────────

struct AttachmentContext {
    enum class Type { NIL, FLOOR };
    Type type = Type::NIL;
    int roomId = -1;
    ChannelEnum channel = CHANNEL_CORPOREAL;

    static AttachmentContext none() { return {}; }
    static AttachmentContext floor(int roomId, ChannelEnum channel = CHANNEL_CORPOREAL) {
        AttachmentContext ctx;
        ctx.type = Type::FLOOR;
        ctx.roomId = roomId;
        ctx.channel = channel;
        return ctx;
    }
};
