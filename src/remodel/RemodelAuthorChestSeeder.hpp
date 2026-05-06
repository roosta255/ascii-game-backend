#pragma once

#include "DungeonMutator.hpp"
#include "RemodelAuthorBase.hpp"
#include "int4.hpp"

struct RemodelAuthorChestSeeder : public RemodelAuthorBase {
    struct ChestSpec {
        int4 room;
        LockEnum lock = LOCK_NIL;
        RoleEnum critterRole = ROLE_EMPTY;
        std::vector<ItemEnum> items;
    };

    ChestSpec chest;
    explicit RemodelAuthorChestSeeder(ChestSpec c) : chest(c) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
