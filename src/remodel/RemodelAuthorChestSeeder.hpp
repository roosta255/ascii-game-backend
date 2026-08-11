#pragma once

#include "DungeonMutator.hpp"
#include "Maybe.hpp"
#include "RemodelAuthorBase.hpp"
#include "int4.hpp"

struct RemodelAuthorChestSeeder : public RemodelAuthorBase {
    struct ChestSpec {
        int4 room;
        LockEnum lock = LOCK_NIL;
        RoleEnum critterRole = ROLE_EMPTY;
        Maybe<RoleEnum> containedRole = Maybe<RoleEnum>::empty();
        std::vector<ItemEnum> items;
        RoleEnum role = ROLE_CHEST;
    };

    ChestSpec chest;
    explicit RemodelAuthorChestSeeder(ChestSpec c) : chest(c) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
