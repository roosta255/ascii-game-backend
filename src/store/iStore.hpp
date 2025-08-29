#pragma once

#include "CodeEnum.hpp"
#include "Match.hpp"
#include <functional>
#include <nlohmann/json.hpp>
#include <string>

class iStore {
public:
    virtual ~iStore() = default;
    virtual bool list(const int limit, const int offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer) = 0;
    virtual bool version(const std::string& filename, CodeEnum& error, int& version) = 0;

    virtual bool load(const std::string& filename, CodeEnum& error, nlohmann::json& root) = 0;
    virtual bool save(const bool isInitial, CodeEnum& error, const nlohmann::json& body) = 0;
};
