// the bytecode generator's notion of what categories of variables there are.
// (global, local, instance, etc.)

#pragma once

#include <map>
#include <string>

#include "ogm/common/types.hpp"
#include "ogm/common/parallel.hpp"

#define LOCALS_COUNT 0x100

namespace ogm { namespace bytecode {

enum accessor_type_t
{
    accessor_none,
    accessor_map,
    accessor_grid,
    accessor_list
};

// describes where a variable id points to.
enum memspace_t
{
    // local variables
    memspace_local,

    // instance variables
    memspace_instance,

    // variables belonging to another instance
    // (shares same namespace as memspace_instance)
    memspace_other,

    // global variables
    memspace_global,

    // built-in instance variables
    memspace_builtin_instance,

    // built-in instance variables for other
    // (shares same namespace as memspace_builtin_instance)
    memspace_builtin_other,

    // built-in global variables
    // (these don't actually have proper memory space. The library defines how to handle them.)
    memspace_builtin_global,

    // not actually a memory space; no variables are stored here.
    memspace_constant,

    // not actually a memory space; used to represent accessor-lvalues.
    memspace_accessor
};

// holds the names in a memory space
class Namespace
{
public:
    Namespace()=default;
    Namespace(const Namespace& other)=default;
    Namespace(Namespace&& other)
        : m_next(std::move(other.m_next))
        , m_name_id(std::move(other.m_name_id))
        , m_id_name(std::move(other.m_id_name))
    { }

    Namespace& operator=(const Namespace& other)=default;

    variable_id_t get_id(std::string name)
    {
        WRITE_LOCK(m_mutex)
        if (m_name_id.find( name ) == m_name_id.end())
        {
            m_name_id[name] = m_next;
            m_id_name[m_next] = name;
            m_next++;
        }

        return m_name_id[name];
    }

    variable_id_t find_id(std::string name) const
    {
        READ_LOCK(m_mutex)
        return m_name_id.at(name);
    }

    const char* find_name(variable_id_t id) const
    {
        READ_LOCK(m_mutex)
        return m_id_name.at(id).c_str();
    }

    bool has_id(std::string name) const
    {
        READ_LOCK(m_mutex)
        return m_name_id.find( name ) != m_name_id.end();
    }

    bool has_name(variable_id_t id) const
    {
        READ_LOCK(m_mutex)
        return m_id_name.find( id ) != m_id_name.end();
    }

    variable_id_t add_id(std::string name)
    {
        // get adds the id.
        return get_id(name);
    }

    variable_id_t id_count() const
    {
        READ_LOCK(m_mutex)
        return m_next;
    }

private:
    variable_id_t m_next = 0;
    std::map<std::string, variable_id_t> m_name_id;
    std::map<variable_id_t, std::string> m_id_name;

#ifdef PARALLEL_COMPILE
    mutable std::mutex m_mutex;
#endif
};

}
}
