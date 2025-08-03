#pragma once

#include "CodeEnum.hpp"
#include "Match.hpp"
#include "iStore.hpp"
#include <functional>
#include <string>

class FileStore : public iStore {
public:
    FileStore(const std::string& dirPath);

    bool load(const std::string& filename, CodeEnum& error, nlohmann::json& root) override;
    bool save(const bool isInitial, CodeEnum& error, const nlohmann::json& body) override;

    bool list(const int limit, const int offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer) override;

    bool version(const std::string& filename, CodeEnum& error, int& version) override;

private:
    std::string getFilePath(const std::string& filename);
    bool setupDirectory(CodeEnum& error);

    std::string dir;
};
