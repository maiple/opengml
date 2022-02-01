#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

#include "ogm/sys/util_sys.hpp"
#include "fs_share.hpp"
#include <iostream>
#include <Windows.h>


namespace ogm
{
    
void browser_open_url(const char* url, const char* target, const char* opts)
{
    std::string url_encoded = encode_url(url);
    ShellExecute(NULL, "open", url_encoded.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

}

#endif