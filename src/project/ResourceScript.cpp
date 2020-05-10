#include "ogm/project/resource/ResourceScript.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include "cache.hpp"

#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

namespace ogm::project {

ResourceScript::ResourceScript(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
    , m_source("")
{ }

ResourceScript::ResourceScript(bool dummy, const char* source, const char* name)
    : Resource(name)
    , m_path("")
    , m_source(source)
{ }

void ResourceScript::load_file()
{
    if (mark_progress(FILE_LOADED)) return;

    if (m_source == "")
    {
        std::string raw_script;

        std::string _path = native_path(m_path);
        m_edit_time = get_file_write_time(_path);

        // read in script
        raw_script = read_file_contents(_path);

        m_source = raw_script;
    }
}

void ResourceScript::parse(const bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PARSED)) return;

    // parse

    #ifdef CACHE_AST
    // check for cached compiled ast...
    bool cache_hit = false;
    std::string cache_path;
    if (acc.m_config->m_cache && m_path != "")
    {
        cache_path = m_path + ".ast.ogmc";
        ogm_ast_t* ast;
        cache_hit = cache_load(ast, cache_path, m_edit_time);
        if (cache_hit)
        {
            m_root_ast = std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{ ast };
        }
    }

    if (!cache_hit)
    #endif
    {
        try
        {
            m_root_ast = std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{
                ogm_ast_parse(
                    m_source.c_str(),
                    ogm_ast_parse_flag_no_decorations
                )
            };
        }
        catch (const std::exception& e)
        {
            std::cout << "An error occurred while parsing the script '" << m_name << "':" << std::endl;
            std::cout << "what(): " << e.what() << std::endl;
            throw MiscError("Failed to parse script");
        }

        #ifdef CACHE_AST
        if (acc.m_config->m_cache)
        {
            cache_write(m_root_ast.get(), cache_path);
        }
        #endif
    }
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

        bytecode::DecoratedAST& decorated_ast = m_ast.emplace_back(
            &ast, name.c_str(), m_source.c_str()
        );
        bytecode::bytecode_preprocess(decorated_ast, *acc.m_reflection);

        // add placeholder bytecode which has retc and argc
        acc.m_bytecode->add_bytecode(
            nullptr, 0, bci,
            decorated_ast.m_retc,
            decorated_ast.m_argc
        );

        m_names.push_back(name);
        m_bytecode_indices.push_back(bci);
    }
}

void ResourceScript::compile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(COMPILED)) return;

    // compile
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        bytecode::Bytecode b;

        #ifdef CACHE_BYTECODE
        // check for cached compiled bytecode...
        bool cache_hit = false;
        std::string cache_path;
        if (acc.m_config->m_cache && m_path != "")
        {
            cache_path = m_path + ".bc.ogmc";
            cache_hit = cache_load(b, acc, cache_path, m_edit_time);
        }

        if (!cache_hit)
        #endif
        {
            try
            {
                bytecode::bytecode_generate(
                    m_ast[i],
                    acc,
                    nullptr,
                    m_bytecode_indices[i]
                );
            }
            catch(std::exception& e)
            {
                throw MiscError("Error while compiling " + m_names[i] + ": \"" + e.what() + "\"");
            }

            #ifdef CACHE_BYTECODE
            if (acc.m_config->m_cache)
            {
                cache_write(acc.m_bytecode.get_bytecode(m_bytecode_indices[i]), acc, cache_path);
            }
            #endif
        }
    }
}

}
