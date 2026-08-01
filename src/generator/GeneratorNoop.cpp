#include "Codeset.hpp"
#include "GeneratorNoop.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool GeneratorNoop::generate(int seed, Match& dst, Codeset& codeset) const {
    return true;
}