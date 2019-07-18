#pragma once

#include "ogm/interpreter/Variable.hpp"

#include <vector>

namespace ogmi
{
    struct DSGrid
    {
        std::vector<std::vector<Variable>> m_data;
        size_t m_width;
        size_t m_height;
        
        DSGrid(size_t width, size_t height)
            : m_width(width)
            , m_height(height)
        { }
    };
}