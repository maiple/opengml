// common implementations that aren't platform-specific

#include "ogm/sys/util_sys.hpp"
#include "fs_share.hpp"

// Not a standard include -- but supported on unix and windows.
#include <sys/stat.h>
#include <iostream>

namespace ogm
{
using namespace ogm::fs;

namespace fs
{
    int terminal_colours_are_supported = -1;
}

// https://stackoverflow.com/a/12774387
bool path_exists(const std::string& name)
{
    struct stat buffer;
    return !(stat (name.c_str(), &buffer));
}

bool path_is_directory(const std::string& path)
{
    #ifdef _WIN32
        // https://stackoverflow.com/q/6993723
        struct stat buf;
        stat(path.c_str(), &buf);
        return ((buf.st_mode & _S_IFDIR) > 0);
    #else
        struct stat buf;
        stat (path.c_str(), &buf);
        return S_ISDIR(buf.st_mode);
    #endif
}

std::string case_insensitive_path(const std::string& base, const std::string& head, bool* o_casechange)
{
    ogm_assert(base.length() > 0);
    ogm_assert(base.back() == PATH_SEPARATOR);
    if (head.length() == 0)
    {
        return base;
    }
    if (head.front() == PATH_SEPARATOR)
    {
        return case_insensitive_path(base + PATH_SEPARATOR, head.substr(1), o_casechange);
    }
    ogm_assert(head.front() != PATH_SEPARATOR);

    if (path_exists(base + head))
    {
        return base + head;
    }

    if (o_casechange) *o_casechange = true;

    std::vector<std::string> paths;
    list_paths(base, paths);

    std::string h = head;
    const std::string intermediate = path_split_first(h);

    bool found = false;
    std::string best_match = intermediate;
    if (!path_exists(base + std::string(1, PATH_SEPARATOR) + intermediate))
    {
        for (const std::string& _path : paths)
        {
            const std::string path = path_leaf(trim_terminating_path_separator(_path));
            if (path == intermediate)
            {
                found = true;
                best_match = intermediate;
                break;
            }
            else if (string_lowercase(path) == string_lowercase(intermediate))
            {
                found = true;
                best_match = path;

                // keep searching for a better match.
                continue;
            }
        }
    }
    else
    {
        found = true;
    }

    if (h != "")
    {
        best_match.push_back(PATH_SEPARATOR);

        if (found)
        {
            return case_insensitive_path(base + best_match, h);
        }
    }

    return base + best_match + h;
}

bool can_read_file(const std::string& s)
{
    FILE * fp;
    fp = fopen(s.c_str(), "rb");
    if (!fp)
    {
        return false;
    }
    fclose(fp);
    return true;
}

bool terminal_supports_colours()
{
    if (terminal_colours_are_supported == -1)
    {
        // TODO: terminfo(5) to check if ANSI colour codes are supported.
        terminal_colours_are_supported = is_terminal();
    }
    return terminal_colours_are_supported;
}

uint64_t get_file_write_time(const std::string& path_to_file)
{
    struct stat result;
    if(stat(path_to_file.c_str(), &result)==0)
    {
        return result.st_mtime;
    }
    return 0;
}

}

