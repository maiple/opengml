#include "ogm/project/resource/ResourceScript.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include <string>

namespace ogm { namespace project {

ResourceScript::ResourceScript(const char* path, const char* name)
    : m_path(path)
    , m_name(name)
{
}

void ResourceScript::load_file()
{
    std::string raw_script;

    std::string _path = native_path(m_path);

    #ifdef VERBOSE_COMPILE_LOG
    std::cout << "parse " << _path << std::endl;
    #endif

    // read in script
    raw_script = read_file_contents(_path);

    m_source = raw_script;
}

void ResourceScript::parse()
{
    // parse
    // TODO: error handling
    m_root_ast = ogm_ast_parse(m_source.c_str());
}

void ResourceScript::precompile(bytecode::ProjectAccumulator& acc)
{
    // TODO: error handling

    std::string name = m_name;
    name = remove_suffix(name, ".gml");
    for (size_t i = 0; i < m_root_ast->m_sub_count; ++i)
    {
        ogm_ast_t& ast = m_root_ast->m_sub[i];
        std::string define = static_cast<char**>(m_root_ast->m_payload)[i];
        if (define != "")
        {
            // TODO: sanitize this name.
            name = remove_prefix(define, "#define");
            trim(name);
        }

        bytecode_index_t bci;
        if (name == "_ogm_entrypoint_")
        {
            bci = 0;
        }
        else
        {
            bci = acc.next_bytecode_index();
        }

        acc.m_assets->add_asset<asset::AssetScript>(name.c_str())->m_bytecode_index = bci;

        uint8_t retc, argc;
        bytecode::bytecode_preprocess(ast, retc, argc, *acc.m_reflection);

        // add placeholder bytecode which has retc and argc
        acc.m_bytecode->add_bytecode(nullptr, 0, bci, retc, argc);

        m_ast.push_back(&ast);
        m_names.push_back(name);
        m_bytecode_indices.push_back(bci);
        m_retc.push_back(retc);
        m_argc.push_back(argc);
    }
}

void ResourceScript::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
    // compile
    #ifdef VERBOSE_COMPILE_LOG
    std::cout << "compile " << m_path << std::endl;
    #endif
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        bytecode::Bytecode b;
        bytecode::bytecode_generate(
            b,
            {m_ast[i], m_retc[i], m_argc[i], (m_names[i] + "()").c_str(), m_source.c_str()},
            library, &acc);
        acc.m_bytecode->add_bytecode(m_bytecode_indices[i], std::move(b));
    }
}

}}
