#include "modules/ConfigManager.h"
#include <fstream>

bool ConfigManager::load(const std::string &filename)
{
    std::ifstream in(filename);
    if (!in.is_open()) return false;

    in >> jsonroot;
    return true;
}

bool ConfigManager::contains(const std::string &key) const
{
    return jsonroot.contains(key);
}

std::vector<std::vector<int>> ConfigManager::getVector(const std::string &key) const
{
    std::vector<std::vector<int>> out;

    if (!jsonroot.contains(key)) return out;

    for (auto &v : jsonroot.at(key))
    {
        out.push_back({v[0], v[1]});
    }

    return out;
}
