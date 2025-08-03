#pragma once

#include "CodeEnum.hpp"
#include "iStore.hpp"
#include "Match.hpp"
#include <functional>
#include <string>

class MatchController {
public:
    MatchController(iStore& store);

    bool init(const Match& match, CodeEnum& error);

    bool save(const Match& match, CodeEnum& error);

    bool load(const std::string& matchId, CodeEnum& error, Match& output);

    bool list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer);

private:
    iStore& store_;
};
