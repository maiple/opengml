#include "ogm/common/util.hpp"
#include <vector>

// Not a standard include -- but supported on unix and windows.
#include <sys/stat.h>

// https://stackoverflow.com/a/12774387
bool path_exists(std::string name)
{
    struct stat buffer;
    return !(stat (name.c_str(), &buffer));
}

#ifdef __unix__

#include <glob.h>

// https://stackoverflow.com/a/24703135
std::vector<std::string> __glob(std::string pattern)
{
    glob_t glob_result;
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    std::vector<std::string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

#endif

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

//https://stackoverflow.com/a/20847429
std::vector<std::string> __glob(std::string search_path)
{
   vector<string> names;
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

std::string case_insensitive_path(std::string base, std::string head)
{
    assert(base.length() > 0);
    assert(base.back() == PATH_SEPARATOR);
    if (head.length() == 0)
    {
        return base;
    }
    assert(head.front() != PATH_SEPARATOR);

    if (path_exists(base + head))
    {
        return base + head;
    }

    std::vector<std::string> paths = __glob(base + std::string("*"));

    const std::string intermediate = path_split_first(head);


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

    if (head != "")
    {
        best_match.push_back(PATH_SEPARATOR);

        if (found)
        {
            return case_insensitive_path(base + best_match, head);
        }
    }

    return base + best_match + head;
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