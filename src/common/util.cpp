#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include <vector>

#include <iostream>

// Not a standard include -- but supported on unix and windows.
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef CPP_FILESYSTEM_ENABLED
#include <filesystem>
#endif

#include <random>
#include <time.h>


// https://stackoverflow.com/a/12774387
bool path_exists(const std::string& name)
{
    struct stat buffer;
    return !(stat (name.c_str(), &buffer));
}


bool path_is_directory(const std::string& path)
{
    struct stat buffer;
    stat (path.c_str(), &buffer);
    return S_ISDIR(buffer.st_mode);
}

#ifdef __unix__

#include <glob.h>

// https://stackoverflow.com/a/24703135
std::vector<std::string> __glob(const std::string& pattern)
{
    glob_t glob_result;
    glob(pattern.c_str(),
        #ifdef EMSCRIPTEN
        0,
        #else
        GLOB_TILDE,
        #endif
        NULL,&glob_result);
    std::vector<std::string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

#endif

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

#include <Windows.h>

//https://stackoverflow.com/a/20847429
std::vector<std::string> __glob(const std::string& search_path)
{
   std::vector<std::string> names;
   WIN32_FIND_DATA fd;
   HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
   if(hFind != INVALID_HANDLE_VALUE) {
       do {
           // read all (real) files in current folder
           // , delete '!' read other 2 default folder . and ..
           if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
               names.push_back(fd.cFileName);
           }
       }while(::FindNextFile(hFind, &fd));
       ::FindClose(hFind);
   }
   return names;
}

#endif

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

    std::vector<std::string> paths = __glob(base + std::string("*"));

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

// https://stackoverflow.com/a/8317622
// TODO: is this hash function good enough?
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */
#define FIRSTH 37 /* also prime */
uint64_t hash_str(const char* s)
{
   uint64_t h = FIRSTH;
   while (char c = *s) {
     h = (h * A) ^ (c * B);
     ++s;
   }
   return h;
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

std::string pretty_typeid(const std::string& _name)
{
    #ifdef __GNUC__
    std::string name = _name;
    name = remove_suffix(name, "E");
    name = remove_prefix(name, "N3");
    for (size_t i = 0; i < name.length(); ++i)
    {
        if (name[i] >= '0' && name[i] <= '9')
        {
            name[i] = ':';
        }
    }
    #endif

    return name;
}

namespace
{
    template<bool recursive>
    void _list_paths_impl(const std::string& base, std::vector<std::string>& out)
    {
        std::vector<std::string> subs = __glob(base + "*");
        for (const std::string& sub : subs)
        {
            if (!ends_with(sub, PATH_SEPARATOR + "..") && !ends_with(sub, PATH_SEPARATOR + "."))
            {
                out.push_back(sub);
                if (recursive && path_is_directory(sub))
                {
                    std::string s;
                    if (ends_with(s, PATH_SEPARATOR + ""))
                    {
                        _list_paths_impl<recursive>(sub, out);
                    }
                    else
                    {
                        _list_paths_impl<recursive>(sub + PATH_SEPARATOR, out);
                    }
                }
            }
        }
    }
}

void list_paths_recursive(const std::string& base, std::vector<std::string>& out)
{
    _list_paths_impl<true>(base, out);
}

void list_paths(const std::string& base, std::vector<std::string>& out)
{
    _list_paths_impl<false>(base, out);
}

bool create_directory(const std::string& path)
{
    #ifdef CPP_FILESYSTEM_ENABLED
    return std::filesystem::create_directory(path);
    #else
    return false;
    #endif
}

std::string get_temp_root()
{
    #ifdef CPP_FILESYSTEM_ENABLED
    return std::filesystem::temp_directory_path();
    #else
    throw MiscError("c++ std::filesystem not supported.");
    #endif
}

// https://stackoverflow.com/a/7114482
typedef std::mt19937 MyRNG;  // the Mersenne Twister with a popular choice of parameters
uint32_t seed_val = std::time(nullptr);           // populate somehow
MyRNG rng{ seed_val };
std::uniform_int_distribution<uint32_t> uint_dist;

std::string create_temp_directory()
{
    #ifndef CPP_FILESYSTEM_ENABLED
    throw MiscError("c++ std::filesystem not supported.");
    #else
    
    std::string root = std::filesystem::temp_directory_path();

    while (true)
    {
        std::string subfolder = "ogm-tmp" + std::to_string(uint_dist(rng));
        std::string joined = path_join(root, subfolder);
        if (!path_exists(joined))
        {
            create_directory(joined);
            return joined + std::string(1, PATH_SEPARATOR);
        }
    }
    #endif
}
