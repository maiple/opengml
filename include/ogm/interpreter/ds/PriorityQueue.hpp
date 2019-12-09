#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <vector>
#include <queue>

namespace ogm { namespace interpreter
{
    struct DSPriorityQueue
    {
        struct PairComparator
        {
            DSComparator c;

            bool operator()(const std::pair<Variable, Variable>& a, const std::pair<Variable, Variable>& b)
            {
                if (c(a.first, b.first))
                {
                    return true;
                }
                if (c(b.first, a.first))
                {
                    return false;
                }
                return c(a.second, b.second);
            }
        };

        std::priority_queue<
            std::pair<Variable, Variable>,
            std::vector<std::pair<Variable, Variable>>,
            PairComparator
        > m_data;
    };
}}
