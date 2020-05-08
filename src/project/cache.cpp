#include "cache.hpp"
#include "ogm/common/compile_time.h"

#include <time.h>

namespace ogm::project
{

uint64_t cache_version()
{
    #ifdef OGM_BUILD_GMTOFF
        return __TIME_UNIX__ - (OGM_BUILD_GMTOFF);
    #else
        #ifdef __GNUC__
            time_t t = time(NULL);
            struct tm lt = {0};
            localtime_r(&t, &lt);
            return __TIME_UNIX__ - lt.tm_gmtoff;
        #else
            // TODO.
            return __TIME_UNIX__;
        #endif
    #endif
}

// loads/writes cached ast
bool cache_load(ogm_ast_t*& ast, const std::string& path, uint64_t min_timestamp)
{
    if (path == "" || starts_with(path, ":") || strchr(path.c_str(), '^')) return false;
    std::string npath = native_path(path);
    std::ifstream _if;
    _if.open(npath, std::ios::in | std::ios::binary);

    if (!_if.good()) return false;

    uint64_t time = get_file_write_time(npath);
    if (time < ogm_ast_parse_version() || time < cache_version())
    {
        _if.close();
        return false;
    }

    ast = ogm_ast_load(_if);

    _if.close();
    return true;
}

bool cache_write(const ogm_ast_t* ast, const std::string& path, uint64_t min_timestamp)
{
    if (path == "" || starts_with(path, ":") || strchr(path.c_str(), '^')) return false;
    std::string npath = native_path(path);
    std::ofstream _of;
    _of.open(npath, std::ios::out | std::ios::binary);

    if (!_of.good()) return false;

    ogm_ast_write(ast, _of);
    _of.close();

    #ifndef NDEBUG
    ogm_ast_t* ast_l;
    if (cache_load(ast_l, path, min_timestamp))
    {
        ogm_assert(ogm_ast_tree_equal(ast, ast_l));
        ogm_ast_free(ast_l);
    }
    #endif
    return true;
}

// loads/writes cached bytecode
bool cache_load(bytecode::Bytecode& out_bytecode, bytecode::ProjectAccumulator& acc, const std::string& path, uint64_t min_timestamp)
{
    return false;
}

bool cache_write(bytecode::Bytecode& bytecode, bytecode::ProjectAccumulator& acc, const std::string& path, uint64_t min_timestamp)
{
    return false;
}

}
