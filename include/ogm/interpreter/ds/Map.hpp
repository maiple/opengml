#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <forward_list>

namespace ogm { namespace interpreter
{
    struct DSMap
    {
        struct MarkedVariable
        {
            enum EntryFlag
            {
                NONE,
                LIST,
                MAP
            } m_flag;
            Variable v;

            MarkedVariable(Variable&& from)
                : v(std::move(from))
                , m_flag(NONE)
            { }

            Variable* operator->()
            {
                return &v;
            }

            const Variable* operator*() const
            {
                return &v;
            }
        };

        std::map<Variable, MarkedVariable, DSComparator> m_data;
        
        #ifdef OGM_GARBAGE_COLLECTOR
        void ds_integrity_check()
        {
            for (auto& [v, mv] : m_data)
            {
                v.gc_integrity_check();
                mv.v.gc_integrity_check();
            }
        }
        #endif
    };
}}
