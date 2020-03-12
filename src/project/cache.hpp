#pragma once

#include "ogm/bytecode/bytecode.hpp"

namespace ogm::project
{

// loads/writes cached ast
bool cache_load(ogm_ast_t*& ast, const std::string& path, uint64_t min_timestamp=0);
bool cache_write(const ogm_ast_t* ast, const std::string& path, uint64_t min_timestamp=0);

// loads/writes cached bytecode
bool cache_load(bytecode::Bytecode& out_bytecode, bytecode::ProjectAccumulator& acc, const std::string& path, uint64_t min_timestamp=0);
bool cache_write(bytecode::Bytecode& bytecode, bytecode::ProjectAccumulator& acc, const std::string& path, uint64_t min_timestamp=0);

}