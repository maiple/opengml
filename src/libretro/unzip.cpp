#include "unzip.hpp"

#ifdef LIBZIP_ENABLED

#include "ogm/common/util.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace ogm
{

bool unzip(const std::string& source, const std::string& dst)
{
    using namespace std;
    std::string command = "7z x " + source + " -o" + dst;
    if (system(command.c_str()))
    {
        return false;
    }

    return true;
}

}

#else

namespace ogm
{

bool unzip(const std::string& source, const std::string& dst)
{
    return false;
}

}
#endif
