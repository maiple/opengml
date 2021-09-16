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
}