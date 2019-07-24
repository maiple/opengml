#pragma once

#include "ogm/interpreter/Variable.hpp"

#include <forward_list>

namespace ogm { namespace interpreter
{
    struct DSList
    {
        std::forward_list<Variable> m_data;
        size_t m_size = 0;
    };
}}