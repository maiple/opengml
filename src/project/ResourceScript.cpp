#include "ogm/project/resource/ResourceScript.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include "cache.hpp"

#include <string>

namespace ogm { namespace project {

ResourceScript::ResourceScript(const char* path, const char* name)
    : m_path(path)
    , m_name(name)
    , m_source("")
{ }

ResourceScript::ResourceScript(bool dummy, const char* source, const char* name)
    : m_path("")
    , m_name(name)
    , m_source(source)
{ }

void ResourceScript::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    if (m_source == "")
    {
        std::string raw_script;

        std::string _path = native_path(m_path);

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
    if (acc.m_config->m_cache)
    {
        cache_path = m_path + ".ast.ogmc";
        cache_hit = cache_load(m_root_ast, cache_path);
    }
    
    if (!cache_hit)
    #endif
    {
        try
        {
            m_root_ast = ogm_ast_parse(
                m_source.c_str(),
                ogm_ast_parse_flag_no_decorations
            );
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
            cache_write(m_root_ast, cache_path);
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

void ResourceScript::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
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
        if (acc.m_config->m_cache)
        {
            cache_path = m_path + ".bc.ogmc";
            cache_hit = cache_load(b, acc, cache_path);
        }
        
        if (!cache_hit)
        #endif
        {
            try
            {
                bytecode::bytecode_generate(
                    b,
                    m_ast[i],
                    library, &acc
                );
            }
            catch(std::exception& e)
            {
                throw MiscError("Error while compiling " + m_names[i] + ": \"" + e.what() + "\"");
            }
            
            #ifdef CACHE_BYTECODE
            if (acc.m_config->m_cache)
            {
                cache_write(b, acc, cache_path);
            }
            #endif
        }
        acc.m_bytecode->add_bytecode(m_bytecode_indices[i], std::move(b));
    }
}

}}
