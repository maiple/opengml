#include "libpre.h"
    #include "fn_matrix.h"
    #include "fn_types.h"
#include "libpost.h"

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

namespace
{

real_t safe_division(real_t num, real_t den, real_t def=0)
{
    if (den == 0) return def;
    return num / den;
}
}

void ogm::interpreter::fn::matrix_get(VO out, V v)
{
    matrix_t arr;

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
    matrix_to_variable(arr, out);
}

void ogm::interpreter::fn::matrix_set(VO out, V v, V matrix)
{
    matrix_t arr;
    variable_to_matrix(matrix, arr);
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
        out.array_get(OGM_2DARRAY_DEFAULT_ROW i - 1) = static_cast<real_t>(0);
    }

    // scale
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 0) = xs.castCoerce<real_t>();
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 5) = ys.castCoerce<real_t>();
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 10) = zs.castCoerce<real_t>();
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 15) = 1.0;

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
            rm.array_get(OGM_2DARRAY_DEFAULT_ROW i - 1) = static_cast<real_t>(0);
        }
        rm.array_get(axis, axis) = 1;
        rm.array_get(OGM_2DARRAY_DEFAULT_ROW 0 + (axis == 0) + 4*(0 + (axis == 0))) = std::cos(theta);
        rm.array_get(OGM_2DARRAY_DEFAULT_ROW 0 + (axis == 0) + 4*(1 + (axis != 2))) = std::sin(theta);
        rm.array_get(OGM_2DARRAY_DEFAULT_ROW 1 + (axis != 2) + 4*(0 + (axis == 0))) = -std::sin(theta);
        rm.array_get(OGM_2DARRAY_DEFAULT_ROW 1 + (axis != 2) + 4*(1 + (axis != 2))) = std::cos(theta);
        rm.array_get(OGM_2DARRAY_DEFAULT_ROW 15) = 1.0f;
        Variable outc;
        outc.copy(out);
        matrix_multiply(out, rm, outc);
        outc.cleanup();
        rm.cleanup();
    }

    // translate
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 12) = x.castCoerce<real_t>();
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 13) = y.castCoerce<real_t>();
    out.array_get(OGM_2DARRAY_DEFAULT_ROW 14) = z.castCoerce<real_t>();
}

void ogm::interpreter::fn::matrix_build_identity(VO out)
{
    matrix_t arr;
    identity_matrix(arr);
    matrix_to_variable(arr, out);
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
        out.array_get(OGM_2DARRAY_DEFAULT_ROW i - 1) = static_cast<real_t>(0);
    }

    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            size_t index = 4 * j + i;
            real_t dot = 0;
            for (size_t v = 0; v < 4; ++v)
            {
                dot += m2.array_at(OGM_2DARRAY_DEFAULT_ROW j*4 + v).castCoerce<real_t>()
                     * m1.array_at(OGM_2DARRAY_DEFAULT_ROW i + v*4).castCoerce<real_t>();
            }
            out.array_get(OGM_2DARRAY_DEFAULT_ROW index) = dot;
        }
    }
}

void ogm::interpreter::fn::matrix_transform_vertex(VO out, V vmat, V vx, V vy, V vz)
{
    real_t vec[4] {
        vx.castCoerce<real_t>(),
        vy.castCoerce<real_t>(),
        vz.castCoerce<real_t>(),
        1
    };
    real_t ovec[4] { 0, 0, 0, 0 };
    matrix_t mat;
    variable_to_matrix(vmat, mat);
    
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            ovec[i] += mat[i * 4 + j] * vec[j];
        }
    }
    
    // there is no normalization step. (confirmed).
    #if 0
    if (ovec[3] == 0)
    {
        ovec[0] = ovec[1] = ovec[2] = ovec[3];
    }
    else if (ovec[3] != 1)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            ovec[i] /= ovec[3];
        }
    }
    #endif
    
    // to output;
    out.array_ensure();
    for (size_t i = 0; i < 3; ++i)
    {
        out.array_get(OGM_2DARRAY_DEFAULT_ROW i) = ovec[i];
    }
}

void ogm::interpreter::fn::matrix_build_projection_perspective(VO out, V vw, V vh, V zclose, V zfar)
{
    real_t zfield = zfar.castCoerce<real_t>() - zclose.castCoerce<real_t>();
    matrix_t mat {
        safe_division(2.0 * zclose.castCoerce<real_t>(), vw.castCoerce<real_t>(), 1), 0, 0, 0,
        0, safe_division(2.0 * zclose.castCoerce<real_t>(), vh.castCoerce<real_t>(), 1), 0, 0,
        0, 0, safe_division(zfar.castCoerce<real_t>(), zfield, 1), // z:z
        (zfield == 0) ? 0.0 : 1.0, // w:z
        0, 0, (zfield == 0) ? 0 : (zfar.castCoerce<real_t>() + zfar.castCoerce<real_t>() / zfield), 0 // z:w
    };
    
    matrix_to_variable(mat, out);
}

void ogm::interpreter::fn::matrix_build_projection_ortho(VO out, V vw, V vh, V zclose, V zfar)
{
    real_t zfield = zfar.castCoerce<real_t>() - zclose.castCoerce<real_t>();
    matrix_t mat {
        safe_division(2.0, vw.castCoerce<real_t>(), 1), 0, 0, 0,
        0, safe_division(2.0, vh.castCoerce<real_t>(), 1), 0, 0,
        0, 0, safe_division(1, zfield, 1), 0,
        0, 0, safe_division(-zclose.castCoerce<real_t>(), zfield), 1
    };
    
    matrix_to_variable(mat, out);
}