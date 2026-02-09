#pragma once

struct CharacterDigest
{
    Maybe<int> healthRemaining = 0;
    Maybe<int> featsRemaining = 0;
    Maybe<int> actionsRemaining = 0;
    Maybe<int> movesRemaining = 0;
};
