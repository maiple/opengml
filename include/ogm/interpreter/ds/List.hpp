#pragma once

#include "ogm/interpreter/Variable.hpp"

#include <forward_list>

namespace ogm { namespace interpreter
{
    struct DSList
    {
        std::forward_list<Variable> m_data;
        size_t m_size = 0;

        // data vis. nesting lists and maps for JSON encoding
        enum EntryFlag
        {
            NONE,
            LIST,
            MAP
        };
        std::map<size_t, EntryFlag> m_nested_ds;

        inline EntryFlag get_data_ds_type(size_t index)
        {
            if (index >= m_size)
            {
                return NONE;
            }

            auto iter = m_nested_ds.find(index);
            if (iter == m_nested_ds.end())
            {
                return NONE;
            }

            return iter->second;
        }
        
        #ifdef OGM_GARBAGE_COLLECTOR
        void ds_integrity_check()
        {
            for (Variable& v : m_data)
            {
                v.gc_integrity_check();
            }
        }
        #endif
    };
}}
