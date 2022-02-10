#if defined(__unix__) || defined(__APPLE__)

#include "ogm/sys/util_sys.hpp"
#include <stdlib.h>

namespace ogm
{
    
void browser_open_url(const char* url, const char* target, const char* opts)
{
    std::string sys = std::string("xdg-open \"") + encode_url(url) + "\"";
    system(sys.c_str());
}

std::string browser_get_url() { return ""; }
std::string browser_get_domain() { return ""; }

}
#endif