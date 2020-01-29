#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <vector>

namespace ogm { namespace interpreter
{
    struct DSStack
    {
        std::vector<Variable> m_data;
        
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
