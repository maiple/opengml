#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <vector>

namespace ogm { namespace interpreter
{
    struct DSStack
    {
        std::vector<Variable> m_data;
    };
}}
