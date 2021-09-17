#pragma once

#include "ogm/common/util.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ogm::project
{
inline void checkModelName(const json& j, std::string expected)
{
    std::string field = j.at("modelName").get<std::string>();
    if (!ends_with(field, expected)) throw ProjectError(
        1003,
        "Expected modelName compatible with \"OGM{}\"; model name is \"{}\".",
        expected, field
    );
}

template<typename T>
inline typename std::conditional<std::is_convertible<T, std::string>::value, std::string, T>::type
getDefault(const json& j, const char* key, T def)
{
    auto iter = j.find(key);
    if (iter != j.end())
    {
        // TODO: reuse iter
        return j.at(key).get<typename std::conditional<std::is_convertible<T, std::string>::value, std::string, T>::type>();
    }
    else
    {
        return def;
    }
}

inline bool hasKey(const json& j, const char* key)
{
    return j.find(key) != j.end();
}
}