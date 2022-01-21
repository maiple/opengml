#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <queue>

namespace ogm { namespace interpreter
{
    struct DSQueue
    {
        std::deque<SafeVariable> m_data;
        
        #ifdef OGM_GARBAGE_COLLECTOR
        void ds_integrity_check()
        {
            for (auto& v : m_data)
            {
                v.gc_integrity_check();
            }
        }
        #endif
    };
}}
