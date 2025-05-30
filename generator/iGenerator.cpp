#include "iGenerator.hpp"
#include "Builder.hpp"
#include "Match.hpp"
#include "Titan.hpp"

bool iGenerator::generate (const Match& src, int seed, Match& dst) const {
    // generate new match
    if (!this->generate(seed, dst)) {
        return false;
    }

    // copy over pre-match data
    dst.match = src.match;
    dst.host = src.host;
    dst.version = src.version;
    dst.turn = src.turn;
    dst.started = src.started;
    dst.generator = src.generator;

    dst.titan.player.account = src.titan.player.account;

    int i = 0;
    for (const auto& builder: src.builders) {
        dst.builders.access(i, [&](auto& setter){
            setter.player.account = builder.player.account;
        });
        i++;
    }

    return true;
}