#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/ast/parse.h"
#include "ogm/bytecode/bytecode.hpp"

namespace ogm { namespace project {

class ResourceScript : public Resource {
public:
    ResourceScript(const char* path, const char* name);
    ResourceScript(bool dummy, const char* source, const char* name);

    void load_file() override;
    void parse() override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);
    const char* get_name() { return m_name.c_str(); }
    ~ResourceScript()
    {
        if (m_root_ast)
        {
            ogm_ast_free(m_root_ast);
        }
    }

private:
    // data set during initialization
    std::string m_path;
    std::string m_name;
    std::vector<std::string> m_names;

private:
    // data set during precompilation, required for compilation.
    ogm_ast_t* m_root_ast = nullptr;
    std::vector<ogm_ast_t*> m_ast;
    std::vector<bytecode_index_t> m_bytecode_indices;
    std::vector<uint8_t> m_retc;
    std::vector<uint8_t> m_argc;
    std::string m_source;
};

}}
