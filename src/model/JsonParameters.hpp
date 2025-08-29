#pragma once

#include "JsonSetting.hpp"

class Match;



// struct to store flags for .toJson()
struct JsonParameters {
    constexpr JsonParameters(const Match& match, JsonSetting setting): match(match), setting(setting) {}

    int mask = ~0;
    JsonSetting setting;

    const Match& match;

    constexpr bool isDatastore() const {
        return setting == JSON_MATCH_DATASTORE;
    }

    constexpr bool isClient() const {
        return setting == JSON_MATCH_CLIENT;
    }

};