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

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::variable_global_exists(VO out, V v)
{
    if (frame.m_reflection)
    {
        std::string name = v.castCoerce<std::string>();
        const auto& namesp = frame.m_reflection->m_namespace_global;
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
        const auto& namesp = frame.m_reflection->m_namespace_global;
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
        auto& namesp = frame.m_reflection->m_namespace_global;
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
        instance_id_t instance_id = id.castCoerce<direct_instance_id_t>();
        const Instance* instance = frame.get_instance_single(instance_id, staticExecutor.m_self, staticExecutor.m_other);
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
        instance_id_t instance_id = id.castCoerce<direct_instance_id_t>();
        const Instance* instance = frame.get_instance_single(instance_id, staticExecutor.m_self, staticExecutor.m_other);
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
        instance_id_t instance_id = id.castCoerce<direct_instance_id_t>();
        const Instance* instance = frame.get_instance_single(instance_id, staticExecutor.m_self, staticExecutor.m_other);
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

        Variable arr(names);
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
        instance_id_t instance_id = id.castCoerce<direct_instance_id_t>();
        Instance* instance = frame.get_instance_single(instance_id, staticExecutor.m_self, staticExecutor.m_other);
        if (!instance)
        {
            throw MiscError("Instance does not exist.");
        }
        const char* name = v.castCoerce<std::string>().c_str();
        auto& namesp = frame.m_reflection->m_namespace_instance;
        variable_id_t variable_id = namesp.get_id(name);
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
