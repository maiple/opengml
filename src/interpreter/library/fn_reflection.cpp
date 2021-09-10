#include "libpre.h"
    #include "fn_reflection.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>
#include <functional>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame


void ogm::interpreter::fn::string_execute(VO out, V v)
{
    static std::map<size_t, bytecode_index_t> indices;
    
    // can't perform string execution without reflection.
    if (!frame.m_reflection) throw MiscError("Reflection is not enabled.");
    
    // check if code is already cached.
    bytecode_index_t index;;
    std::string code = v.castCoerce<std::string>();
    size_t hash = std::hash<std::string>{}(code);
    auto iter = indices.find(hash);
        
    if (iter == indices.end())
    // not found in cache -- compile now.
    {
        // parse code
        ogm_ast_t* ast;
        try
        {
            ast = ogm_ast_parse(code.c_str());
        }
        catch(const std::exception& e)
        {
            std::stringstream s;
            s << "While parsing the provided string, an error occurred:\n";
            s << e.what() << std::endl;
            throw MiscError{ s.str() };
        }
        
        if (!ast)
        {
            throw MiscError{ "Unable to parse ast.\n" };
        }
        
        // compile code
        try
        {
            GenerateConfig cfg;
            bytecode::ProjectAccumulator acc = ogm::interpreter::staticExecutor.create_project_accumulator();
            index = bytecode_generate(
                {ast, "anonymous string_execute section", code.c_str(), 0, 0},
                acc,
                &cfg
            );
        }
        catch(const std::exception& e)
        {
            std::stringstream s;
            s << "While compiling the provided string, an error occurred.\n";
            s << e.what() << std::endl;
            ogm_ast_free(ast);
            throw MiscError{ s.str() };
        }
        ogm_ast_free(ast);
        
        // store in cache
        indices[hash] = index;
    }
    else
    {
        // already cached -- use this bytecode.
        index = iter->second;
    }
    
    // execute code
    execute_bytecode(index);
    
    // don't clean up bytecode, because it's cached.
}

void ogm::interpreter::fn::variable_global_exists(VO out, V v)
{
    if (frame.m_reflection)
    {
        std::string name = v.castCoerce<std::string>();
        const auto& namesp = frame.m_reflection->m_namespace_instance;
        if (namesp.has_id(name))
        {
            variable_id_t variable_id = namesp.find_id(name);
            if (frame.has_global_variable(variable_id))
            {
                out = true;
                return;
            }
        }

        out = false;
        return;
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_global_get(VO out, V v)
{
    if (frame.m_reflection)
    {
        std::string name = v.castCoerce<std::string>().c_str();
        const auto& namesp = frame.m_reflection->m_namespace_instance;
        if (namesp.has_id(name))
        {
            variable_id_t variable_id = namesp.find_id(name);
            if (frame.has_global_variable(variable_id))
            {
                out.copy(frame.get_global_variable(variable_id));
                return;
            }

            out.copy(k_undefined_variable);
            return;
        }
        else
        {
            out.copy(k_undefined_variable);
            return;
        }
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_global_set(VO out, V v, V value)
{
    if (frame.m_reflection)
    {
        std::string name = v.castCoerce<std::string>().c_str();
        auto& namesp = frame.m_reflection->m_namespace_instance;
        variable_id_t variable_id = namesp.get_id(name);
        Variable _value;
        _value.copy(value);
        frame.store_global_variable(variable_id, std::move(_value));
        return;
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_instance_exists(VO out, V id, V v)
{
    if (frame.m_reflection)
    {
        const Instance* instance = frame.get_instance_single(id, staticExecutor.m_self, staticExecutor.m_other);
        if (!instance)
        {
            throw MiscError("Instance does not exist.");
        }
        std::string name = v.castCoerce<std::string>();
        const auto& namesp = frame.m_reflection->m_namespace_instance;
        if (namesp.has_id(name))
        {
            variable_id_t variable_id = namesp.find_id(name);
            if (instance->hasVariable(variable_id))
            {
                out = true;
                return;
            }
        }

        out = false;
        return;
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_instance_get(VO out, V id, V v)
{
    if (frame.m_reflection)
    {
        const Instance* instance = frame.get_instance_single(id, staticExecutor.m_self, staticExecutor.m_other);
        if (!instance)
        {
            throw MiscError("Instance does not exist.");
        }
        std::string name = v.castCoerce<std::string>().c_str();
        const auto& namesp = frame.m_reflection->m_namespace_instance;
        if (namesp.has_id(name))
        {
            variable_id_t variable_id = namesp.find_id(name);
            if (instance->hasVariable(variable_id))
            {
                out.copy(instance->findVariable(variable_id));
                return;
            }

            out.copy(k_undefined_variable);
            return;
        }
        else
        {
            out.copy(k_undefined_variable);
            return;
        }
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_instance_get_names(VO out, V id)
{
    if (frame.m_reflection)
    {
        const Instance* instance = frame.get_instance_single(id, staticExecutor.m_self, staticExecutor.m_other);
        if (!instance)
        {
            throw MiscError("Instance does not exist.");
        }

        const auto& namesp = frame.m_reflection->m_namespace_instance;
        // OPTIMIZE: iterate through the IDs in the instance's variables directly

        std::vector<const char*> names;
        for (variable_id_t i = 0; i < namesp.id_count(); ++i)
        {
            if (instance->hasVariable(i))
            {
                names.push_back(namesp.find_name(i));
            }
        }

        std::sort(names.begin(), names.end(), [](const char* a, const char* b) -> bool
            {
                return strcmp(a, b) < 0;
            }
        );
        
        #if defined(__GNUC__) && !defined(__llvm__)
            // MSVC has trouble with this constructor.
            Variable arr(names);
        #else
            Variable arr;
            for (size_t i = 0; i < names.size(); ++i)
            {
                arr.array_get(
                    #ifdef OGM_2DARRAY
                    0,
                    #endif
                    i
                ) = std::string(names.at(i));
            }
        #endif
        out = std::move(arr);
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}

void ogm::interpreter::fn::variable_instance_set(VO out, V id, V v, V value)
{
    if (frame.m_reflection)
    {
        Instance* instance = frame.get_instance_single(id, staticExecutor.m_self, staticExecutor.m_other);
        if (!instance)
        {
            throw MiscError("Instance does not exist.");
        }
        auto& namesp = frame.m_reflection->m_namespace_instance;
        variable_id_t variable_id = namesp.get_id(
            v.castCoerce<std::string>().c_str()
        );
        Variable _value;
        _value.copy(value);
        instance->storeVariable(variable_id, std::move(_value));
        return;
    }
    else
    {
        throw MiscError("Reflection is not enabled.");
    }
}
