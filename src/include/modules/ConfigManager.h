#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ConfigManager {
private:
    nlohmann::json jsonroot;

public:
    ConfigManager() = default;

    bool load(const std::string &filename);

    bool contains(const std::string &key) const;

    std::vector<std::vector<int>> getVector(const std::string &key) const;
};
