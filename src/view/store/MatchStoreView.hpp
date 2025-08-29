#pragma once

#include "adl_serializer.hpp"
#include "BuilderStoreView.hpp"
#include "DungeonStoreView.hpp"
#include "GeneratorFlyweight.hpp"
#include "Match.hpp"
#include "TitanStoreView.hpp"
#include "TurnerStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct MatchStoreView
{
    std::string filename;
    std::string host;
    std::string username;
    std::string generator;
    uint32_t version = 0;

    Array<BuilderStoreView, Match::MATCH_BUILDER_COUNT> builders;
    DungeonStoreView dungeon;
    TitanStoreView titan;
    TurnerStoreView turner;

    inline MatchStoreView() = default;
    inline MatchStoreView(const Match& model)
    : filename(model.filename), version(model.version)
    , username(model.username), host(model.host)
    , titan(model.titan)
    , builders(model.builders.convert<BuilderStoreView>())
    , dungeon(model.dungeon)
    , turner(model.turner)
    {
        GeneratorFlyweight::getFlyweights().accessConst(model.generator, [&](const GeneratorFlyweight& flyweight) {
            generator = flyweight.name;
        });
    }

    inline explicit operator Match() const
    {
        Match model{
            .filename = this->filename,
            .host = this->host,
            .version = this->version,
            .builders = this->builders.convert<Builder>(),
            .dungeon = this->dungeon,
            .turner = this->turner
        };
        model.username = this->username;
        model.titan = this->titan;
        GeneratorFlyweight::indexByString(this->generator, model.generator);
        return model;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MatchStoreView, filename, host, username, generator, version, titan, builders, dungeon, turner)
