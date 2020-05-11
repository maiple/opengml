#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/ast/parse.h"
#include "ogm/bytecode/bytecode.hpp"

namespace ogm::project {

class ResourceScript : public Resource {
public:
    ResourceScript(const char* path, const char* name);
    ResourceScript(bool dummy, const char* source, const char* name);

    void load_file() override;
    void parse(const bytecode::ProjectAccumulator& acc) override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&);
    const char* get_type_name() override { return "script"; };

private:
    // data set during initialization
    std::string m_path;
    std::vector<std::string> m_names;
    uint64_t m_edit_time;

private:
    // data set during precompilation, required for compilation.
    std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t> m_root_ast;
    // ownership note: these decorated asts have (non-owning) pointers to subtrees m_root_ast.
    std::vector<bytecode::DecoratedAST> m_ast;
    std::vector<bytecode_index_t> m_bytecode_indices;
    std::string m_source;
};

}
