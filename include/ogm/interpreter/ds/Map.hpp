#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <forward_list>

namespace ogmi
{
    struct DSMap
    {
        std::map<Variable, Variable, DSComparator> m_data;
    };
}