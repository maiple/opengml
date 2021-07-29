#include "ogm/sys/util_sys.hpp"
#include "fs_share.hpp"

#if defined(__unix__) || defined(__APPLE__)

#ifdef __APPLE__
    // for _NSGetExecutablePath
    #include <mach-o/dyld.h>
#endif

// for readlink and usleep
#include <unistd.h>

#ifndef CPP_FILESYSTEM_ENABLED
  #error std::filesystem support required for unix build
#endif

#include <glob.h>
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
    #ifdef __APPLE__
    uint32_t
    #else
    const size_t
    #endif
        bufsize = 2048;
    char buf[bufsize];
    #ifdef __unix__
        int64_t len = readlink("/proc/self/exe", buf, bufsize);
    #elif defined(__APPLE__)
        // Apple
        int64_t len;
        if (_NSGetExecutablePath(buf, &bufsize) != 0)
        {
            len = -1;
        }
        else
        {
            len = bufsize;
        }
    #else
        #error ""
    #endif
    if (len >= 0)
    {
        buf[len] = 0;
        strcpy(buf, path_directory(buf).c_str());
        return static_cast<char*>(buf);
    }
    else
    {
        #ifdef __unix__
        throw MiscError("get_binary_directory(): readlink failed, " + std::string(strerror(errno)));
        #else
        throw MiscError("get_binary_directory() failed: insufficient size");
        #endif
    }
}

}

#endif