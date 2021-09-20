#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include <fmt/core.h>
#include <vector>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <random>
#include <time.h>

#if __has_include (<cxxabi.h>)
#include <cxxabi.h>
#define OGM_CXX_ABI_AVAILABLE
#endif

// https://stackoverflow.com/a/8317622
// TODO: is this hash function good enough?
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */
#define FIRSTH 37 /* also prime */

namespace ogm {
uint64_t hash64(const char* s)
{
   uint64_t h = FIRSTH;
   while (char c = *s) {
     h = (h * A) ^ (c * B);
     ++s;
   }
   return h;
}

std::string pretty_typeid(const std::string& _name)
{
    #ifdef OGM_CXX_ABI_AVAILABLE
    int status = -4;
    char* const demangled_name = abi::__cxa_demangle(_name.c_str(), NULL, NULL, &status);
    if (status == 0) {
        ogm_defer(free(demangled_name));
        return demangled_name;
    }
    #endif
    
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
    #else
    std::string name = _name;
    #endif

    return name;
}

std::string join(const std::vector<std::string>& vec, const std::string& separator)
{
    std::stringstream ss;
    bool first = true;
    for (const std::string& string : vec)
    {
        if (!first) ss << separator;
        first = false;
        ss << string;
    }

    return ss.str();
}

std::string hex_string(int64_t i, bool caps)
{
    return fmt::format(caps ? "{:X}" : "{:x}", i);
}

}
