#pragma once

#include "adl_serializer.hpp"
#include "BuilderApiView.hpp"
#include "DungeonApiView.hpp"
#include "GeneratorFlyweight.hpp"
#include "Match.hpp"
#include "MatchApiParameters.hpp"
#include "Timestamp.hpp"
#include "TitanApiView.hpp"
#include "TurnerApiView.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct MatchApiView
{
    std::string filename;
    std::string host;
    std::string username;
    std::string generator;
    uint32_t version = 0;
    bool isStarted = false, isCompleted = false, isEmpty = false, isFull = false;
    std::vector<std::string> turners;
    long serverTime;

    Array<BuilderApiView, Match::MATCH_BUILDER_COUNT> builders;
    DungeonApiView dungeon;
    TitanApiView titan;
    TurnerApiView turner;

    inline MatchApiView() = default;
    inline MatchApiView(const MatchApiParameters& params)
    : filename(params.match.filename.toString()), version(params.match.version)
    , username(params.match.username.toString()), host(params.match.host.toString())
    , titan(params.match.titan, params)
    , builders(params.match.builders.transform([&](const Builder& builder){return BuilderApiView(builder, params);}))
    , dungeon(params.match.dungeon, params)
    , turner(params.match.turner, params)
    , serverTime((long)Timestamp())
    {
        // lobby flags
        CodeEnum error;
        this->isStarted = params.match.isStarted(error);
        this->isCompleted = params.match.isCompleted(error);
        this->isFull = params.match.isFull(error);
        this->isEmpty = params.match.isEmpty(error);

        // turners
        if (params.match.turner.isTitansTurn()) {
            turners.push_back(params.match.titan.player.account.toString());
        } else {
            for (const auto& builder: params.match.builders) {
                turners.push_back(builder.player.account.toString());
            }
        }

        // generator
        GeneratorFlyweight::getFlyweights().accessConst(params.match.generator, [&](const GeneratorFlyweight& flyweight) {
            generator = flyweight.name;
        });
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MatchApiView, filename, host, username, generator, version, titan, builders, dungeon, turner, turners, isStarted, isCompleted, isFull, isEmpty, serverTime)
