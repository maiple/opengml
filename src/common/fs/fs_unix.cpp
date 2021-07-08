#include "ogm/common/util_sys.hpp"

#if defined(__unix__) || defined(__APPLE__)

#ifndef CPP_FILESYSTEM_ENABLED
  #error std::filesystem support required for unix build
#endif

#include <glob.h>
#include <unistd.h>

#include <iostream>

namespace ogm {

void sleep(int32_t ms)
{
    if (ms < 0)
    {
        ms = 0;
    }
    usleep(ms * 1000);
}

bool is_terminal()
{
    static int result = -1;

    if (result >= 0) return result;

    if (isatty(fileno(stdin)))
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static bool terminal_colours_enabled = false;

void enable_terminal_colours()
{
  if (!is_terminal()) return;
  terminal_colours_enabled = true;
}

void restore_terminal_colours()
{
    if (!is_terminal()) return;
    if (!terminal_colours_enabled) return;
    
    // reset colour if set.
    std::cout << ansi_colour("0");
}

// returns path to the directory containing the executable.
std::string get_binary_directory()
{
    const size_t bufsize = 1024;
    char buf[bufsize];
    int64_t len = readlink("/proc/self/exe", buf, bufsize);
    if (len >= 0)
    {
        buf[len] = 0;
        strcpy(buf, path_directory(buf).c_str());
        return static_cast<char*>(buf);
    }
    else
    {
        throw MiscError("get_binary_directory(): readlink failed, " + std::string(strerror(errno)));
    }
}

}

#endif