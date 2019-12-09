#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <queue>

namespace ogm { namespace interpreter
{
    struct DSQueue
    {
        std::queue<Variable> m_data;
    };
}}
