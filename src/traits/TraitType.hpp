#pragma once

enum TraitType
{
    TRAIT_TYPE_SOURCE, // externally granted: role, room, inventory
    TRAIT_TYPE_AFFLICTION, // persistent state:
    TRAIT_TYPE_DERIVED // computed only
};
