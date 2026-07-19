#pragma once

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

/* Reads a simple YAML file (sections, "key: value" leaves, # comments) into a
 * flat map keyed by leaf name */
inline std::map<std::string, double> loadConfig(const char *path)
{
    std::map<std::string, double> config;
    std::ifstream file(path);
    if (!file)
    {
        printf("Cannot open config file %s\n", path);
        exit(1);
    }
    std::string line;
    while (std::getline(file, line))
    {
        size_t hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string value = line.substr(colon + 1);
        if (value.find_first_not_of(" \t\r") == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        try
        {
            config[key] = std::stod(value);
        }
        catch (...)
        {
            printf("Bad config line: %s\n", line.c_str());
            exit(1);
        }
    }
    return config;
}
