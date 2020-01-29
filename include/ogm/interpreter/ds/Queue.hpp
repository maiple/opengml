#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <queue>

namespace ogm { namespace interpreter
{
    struct DSQueue
    {
        std::queue<Variable> m_data;
        
        #ifdef OGM_GARBAGE_COLLECTOR
        void ds_integrity_check()
        {
            // TODO: not sure how to iterate over queue.
        }
        #endif
    };
}}
