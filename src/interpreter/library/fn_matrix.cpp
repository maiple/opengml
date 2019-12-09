#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "ogm/common/error.hpp"
#include "ogm/geometry/Vector.hpp"

#include "ogm/common/error.hpp"
#include <cmath>
#include <algorithm>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

// it is assumed that arrays are indexed as (y,x) -> 4*y + x
// in other words, ROW-MAJOR order.
// (this assumption should, of course, be double-checked. :3)

void ogm::interpreter::fn::matrix_get(VO out, V v)
{
    std::array<real_t, 16> arr;

    switch(v.castCoerce<size_t>())
    {
    case 0:
        arr = frame.m_display->get_matrix_view();
        break;
    case 1:
        arr = frame.m_display->get_matrix_projection();
        break;
    case 2:
        arr = frame.m_display->get_matrix_model();
        break;
    }
    for (size_t i = 0; i < 16; ++i)
    {
        out.array_get(0, i) = arr[i];
    }
}

void ogm::interpreter::fn::matrix_set(VO out, V v, V matrix)
{
    std::array<real_t, 16> arr;
    for (size_t i = 0; i < 16; ++i)
    {
        arr[i] = matrix.array_at(0, i).castCoerce<real_t>();
    }
    switch(v.castCoerce<size_t>())
    {
    case 0:
        frame.m_display->set_matrix_view(arr);
        break;
    case 1:
        frame.m_display->set_matrix_projection(arr);
        break;
    case 2:
        frame.m_display->set_matrix_model(arr);
        break;
    }
}

void ogm::interpreter::fn::matrix_build(VO out, V x, V y, V z, V xa, V ya, V za, V xs, V ys, V zs)
{
    out.array_ensure();

    for (size_t i = 16; i > 0; --i)
    {
        out.array_get(0, i - 1) = static_cast<real_t>(0);
    }

    // scale
    out.array_get(0, 0) = xs.castCoerce<real_t>();
    out.array_get(0, 5) = ys.castCoerce<real_t>();
    out.array_get(0, 10) = zs.castCoerce<real_t>();
    out.array_get(0, 15) = 1.0;

    // rotate -- Y then X then Z
    int32_t axesi[3] = {1, 0, 2};
    const Variable* axesangles[3] = {&xa, &ya, &za};
    for (int32_t i = 0; i < 3; ++i)
    {
        int32_t axis = axesi[i];
        real_t theta = (axesangles[axis])->castCoerce<real_t>();
        if (theta == 0) continue;
        Variable rm;
        for (size_t i = 16; i > 0; --i)
        {
            rm.array_get(0, i - 1) = static_cast<real_t>(0);
        }
        rm.array_get(axis, axis) = 1;
        rm.array_get(0, 0 + (axis == 0) + 4*(0 + (axis == 0))) = std::cos(theta);
        rm.array_get(0, 0 + (axis == 0) + 4*(1 + (axis != 2))) = std::sin(theta);
        rm.array_get(0, 1 + (axis != 2) + 4*(0 + (axis == 0))) = -std::sin(theta);
        rm.array_get(0, 1 + (axis != 2) + 4*(1 + (axis != 2))) = std::cos(theta);
        rm.array_get(0, 15) = 1.0f;
        Variable outc;
        outc.copy(out);
        matrix_multiply(out, rm, outc);
        outc.cleanup();
        rm.cleanup();
    }

    // translate
    out.array_get(0, 12) = x.castCoerce<real_t>();
    out.array_get(0, 13) = y.castCoerce<real_t>();
    out.array_get(0, 14) = z.castCoerce<real_t>();
}

// left-right (standard) multiply m1 x m2
void ogm::interpreter::fn::matrix_multiply(VO out, V m1, V m2)
{
    Variable c;
    is_matrix(c, m1);
    if (!c.cond()) throw MiscError("input 0 is not a matrix.");
    is_matrix(c, m2);
    if (!c.cond()) throw MiscError("input 1 is not a matrix.");
    for (size_t i = 16; i > 0; --i)
    {
        out.array_get(0, i - 1) = static_cast<real_t>(0);
    }

    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            size_t index = 4 * j + i;
            real_t dot = 0;
            for (size_t v = 0; v < 4; ++v)
            {
                dot += m2.array_at(0, j*4 + v).castCoerce<real_t>()
                     * m1.array_at(0, i + v*4).castCoerce<real_t>();
            }
            out.array_get(0, index) = dot;
        }
    }
}
