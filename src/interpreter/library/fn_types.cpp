#include "libpre.h"
    #include "fn_types.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::is_string(VO out, V v)
{
    out = v.get_type() == VT_STRING;
}

void ogm::interpreter::fn::is_array(VO out, V v)
{
    out = v.is_array();
}

void ogm::interpreter::fn::is_real(VO out, V v)
{
    out = (v.get_type() == VT_REAL) ? 1.0 : 0.0;
}

void ogm::interpreter::fn::is_bool(VO out, V v)
{
    out = (v.get_type() == VT_BOOL) ? 1.0 : 0.0;
}


void ogm::interpreter::fn::is_ptr(VO out, V v)
{
    out = (v.get_type() == VT_PTR) ? 1.0 : 0.0;
}

void ogm::interpreter::fn::is_undefined(VO out, V v)
{
    out = (v.get_type() == VT_UNDEFINED) ? 1.0 : 0.0;
}

void ogm::interpreter::fn::is_int32(VO out, V v)
{
    out = (v.get_type() == VT_INT) ? 1.0 : 0.0;
}

void ogm::interpreter::fn::is_int64(VO out, V v)
{
    out = (v.get_type() == VT_UINT64) ? 1.0 : 0.0;
}

void ogm::interpreter::fn::ogm_typeof(VO out, V v)
{
    switch (v.get_type())
    {
    case VT_REAL:
        out = "number";
        return;
    case VT_STRING:
        out = "string";
        return;
    case VT_ARRAY:
    #ifdef OGM_GARBAGE_COLLECTOR
    case VT_ARRAY_ROOT:
    #endif
        out = "array";
        return;
    case VT_BOOL:
        out = "bool";
        return;
    case VT_INT:
        out = "int32";
        return;
    case VT_UINT64:
        out = "int64";
        return;
    case VT_PTR:
        out = "ptr";
        return;
    default:
        out = "unknown";
        return;
    }
}

namespace
{
    template<size_t L>
    void is_real_array_l(VO out, V v)
    {
        if (v.is_array())
        {
            #ifdef OGM_2DARRAY
            if (v.array_height() == 1)
            {
                if (v.array_length(1) == L)
                {
            #else
            if (v.array_height() == L)
            {
            #endif
                    for (size_t i = 0; i < L; ++i)
                    {
                        if (v.array_at(OGM_2DARRAY_DEFAULT_ROW i).get_type() != VT_REAL)
                        {
                            out = 0.0;
                            return;
                        }
                    }

                    out = 1.0;
                    return;
            #ifdef OGM_2DARRAY
                }
            #endif
            }
        }

        out = 0.0;
    }
}

void ogm::interpreter::fn::is_matrix(VO out, V v)
{
    is_real_array_l<16>(out, v);
}

void ogm::interpreter::fn::is_vec2(VO out, V v)
{
    is_real_array_l<2>(out, v);
}

void ogm::interpreter::fn::is_vec3(VO out, V v)
{
    is_real_array_l<3>(out, v);
}

void ogm::interpreter::fn::is_vec4(VO out, V v)
{
    is_real_array_l<4>(out, v);
}
