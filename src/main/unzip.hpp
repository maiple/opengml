#pragma once

#include <string>

// this is for unzipping .gmz projects.

namespace ogm
{
    // returns true if successful
    bool unzip(const std::string& source, const std::string& dst);
}
