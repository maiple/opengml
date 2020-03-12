#include "cache.hpp"

namespace ogm::project
{

// loads/writes cached ast
bool cache_load(ogm_ast_t*& ast, const std::string& path, uint64_t min_timestamp)
{
    std::string npath = native_path(path);
    std::ifstream _if;
    _if.open(npath, std::ios::in | std::ios::binary);
    
    if (!_if.good()) return false;
    
    ast = ogm_ast_load(_if);
    
    _if.close();
    return true;
}

bool cache_write(const ogm_ast_t* ast, const std::string& path, uint64_t min_timestamp)
{
    std::string npath = native_path(path);
    std::ofstream _of;
    _of.open(npath, std::ios::out | std::ios::binary);
    
    if (!_of.good()) return false;
    
    ogm_ast_write(ast, _of);
    
    _of.close();
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