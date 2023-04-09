#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <cmath>
#include <limits>

#ifndef NDEBUG
#include "ogm/interpreter/Variable_impl.inc"
#endif

// the include order is important.

#ifdef OGM_STRUCT_SUPPORT
#include "ogm/interpreter/Instance.hpp"
#endif

namespace ogm::interpreter
{
    static_assert(
        std::is_void<
            decltype(
                std::declval<Variable>().serialize<false>(
                    *std::declval<state_stream<false>::state_stream_t*>()
                )
            )
        >::value
    , "Variable::serialize is void");

    static_assert(has_serialize<Variable>::value, "Variables must be serializable");
}

namespace ogm { namespace interpreter
{

const char* const variable_type_string[] = {
  "undefined",
  "bool",
  "int",
  "uin64",
  "real",
  "string",
  "array",
  #ifdef OGM_GARBAGE_COLLECTOR
      "array",
  #endif
  #ifdef OGM_STRUCT_SUPPORT
      "object",
      #ifdef OGM_GARBAGE_COLLECTOR
          "object",
      #endif
  #endif
  #ifdef OGM_FUNCTION_SUPPORT
  "function",
  #endif
  "pointer",
};

bool Variable::operator==(const Variable& other) const
{
    switch (m_tag)
    {
        case VT_UNDEFINED:
            // undefined only equals another undefined.
            return m_tag == other.m_tag;
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_int == other.m_int;
                case VT_UINT64:
                    return m_int == other.m_uint64;
                case VT_REAL:
                    return m_int == other.m_real;
                default:
                    return false;
            }
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_uint64 == other.m_int;
                case VT_UINT64:
                    return m_uint64 == other.m_uint64;
                case VT_REAL:
                    return m_uint64 == other.m_real;
                default:
                    return false;
            }
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_real == other.m_int;
                case VT_UINT64:
                    return m_real == other.m_uint64;
                case VT_REAL:
                    return m_real == other.m_real;
                default:
                    return false;
            }
        case VT_STRING:
            if (other.m_tag == VT_STRING)
            {
                return *m_string == *other.m_string;
            }
            return false;
        case VT_ARRAY:
            if (!other.is_array()) return false;
            // checks if arrays point to the same underlying data.
            return &m_array.getReadable<false>() == (other.is_gc_root()
                ? &other.m_array.getReadable<true>()
                : &other.m_array.getReadable<false>()
            );
        #ifdef OGM_GARBAGE_COLLECTOR
            case VT_ARRAY_ROOT:
                if (!other.is_array()) return false;
                // checks if arrays point to the same underlying data.
                return &m_array.getReadable<true>() == (other.is_gc_root()
                    ? &other.m_array.getReadable<true>()
                    : &other.m_array.getReadable<false>()
                );
        #endif
        #ifdef OGM_STRUCT_SUPPORT
            case VT_STRUCT:
                if (!other.is_struct()) return false;
                // checks if structs point to same underlying data.
                return &m_struct.getReadable<false>() == (other.is_gc_root()
                    ? &other.m_struct.getReadable<true>()
                    : &other.m_struct.getReadable<false>()
                );
            break;
            #ifdef OGM_GARBAGE_COLLECTOR
                // check if structs point to same underlying data.
                case VT_STRUCT_ROOT:
                    if (!other.is_struct()) return false;
                    return &m_struct.getReadable<true>() == (other.is_gc_root()
                        ? &other.m_struct.getReadable<true>()
                        : &other.m_struct.getReadable<false>()
                    );
            #endif
        #endif
        #ifdef OGM_FUNCTION_SUPPORT
            case VT_FUNCTION:
                if (!other.is_function()) return false;
                return m_bytecode_index == other.m_bytecode_index;
        #endif
        case VT_PTR:
            if (other.m_tag == VT_PTR)
            {
                return m_ptr == other.m_ptr;
            }
            return false;
        default:
            return false;
    }
}

bool Variable::operator!=(const Variable& other) const
{
  return !(*this == other);
}

bool Variable::operator>=(const Variable& other) const
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_int >= other.m_int;
                case VT_UINT64:
                    if (other.m_uint64 >= std::numeric_limits<int32_t>::max())
                    {
                        return true;
                    }
                    return m_int >= static_cast<int32_t>(other.m_uint64);
                case VT_REAL:
                    return m_int >= other.m_real;
                default:
                    return false;
            }
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_uint64 >= other.m_int;
                case VT_UINT64:
                    return m_uint64 >= other.m_uint64;
                case VT_REAL:
                    return m_uint64 >= other.m_real;
                default:
                    return false;
            }
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_real >= other.m_int;
                case VT_UINT64:
                    return m_real >= other.m_uint64;
                case VT_REAL:
                    return m_real >= other.m_real;
                default:
                    return false;
            }
        default:
            throw MiscError("Cannot order non-numerical types");
    }
}

bool Variable::operator>(const Variable& other) const
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_int > other.m_int;
                case VT_UINT64:
                    return m_int > other.m_uint64;
                case VT_REAL:
                    return m_int > other.m_real;
                default:
                    return false;
            }
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_uint64 > other.m_int;
                case VT_UINT64:
                    return m_uint64 > other.m_uint64;
                case VT_REAL:
                    return m_uint64 > other.m_real;
                default:
                    return false;
            }
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_real > other.m_int;
                case VT_UINT64:
                    return m_real > other.m_uint64;
                case VT_REAL:
                    return m_real > other.m_real;
                default:
                    return false;
            }
        default:
            throw MiscError("Cannot order non-numerical types");
    }
}

bool Variable::operator<=(const Variable& other) const
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_int <= other.m_int;
                case VT_UINT64:
                    return m_int <= other.m_uint64;
                case VT_REAL:
                    return m_int <= other.m_real;
                default:
                    return false;
            }
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_uint64 <= other.m_int;
                case VT_UINT64:
                    return m_uint64 <= other.m_uint64;
                case VT_REAL:
                    return m_uint64 <= other.m_real;
                default:
                    return false;
            }
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_real <= other.m_int;
                case VT_UINT64:
                    return m_real <= other.m_uint64;
                case VT_REAL:
                    return m_real <= other.m_real;
                default:
                    return false;
            }
        default:
            throw MiscError("Cannot order non-numerical types");
    }
}

bool Variable::operator<(const Variable& other) const
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_int < other.m_int;
                case VT_UINT64:
                    return m_int < other.m_uint64;
                case VT_REAL:
                    return m_int < other.m_real;
                default:
                    return false;
            }
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_uint64 < other.m_int;
                case VT_UINT64:
                    return m_uint64 < other.m_uint64;
                case VT_REAL:
                    return m_uint64 < other.m_real;
                default:
                    return false;
            }
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    return m_real < other.m_int;
                case VT_UINT64:
                    return m_real < other.m_uint64;
                case VT_REAL:
                    return m_real < other.m_real;
                default:
                    return false;
            }
        default:
            throw MiscError("Cannot order non-numerical types");
    }
}

void Variable::operator+=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int += other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int + other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int += static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_int + other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot add types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_uint64 += other.m_int;
                    break;
                case VT_UINT64:
                    m_uint64 += other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real) && m_uint64 >= -other.m_real)
                    {
                        m_uint64 += (uint64_t)other.m_real;
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_uint64 + other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot add types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_INT;
                        m_int = static_cast<int32_t>(m_real) + other.m_int;
                    }
                    else
                    {
                        m_real += other.m_int;
                    }
                    break;
                case VT_UINT64:
                    if (m_real == floor(m_real))
                    {
                        if (m_real >= 0)
                        {
                            m_tag = VT_UINT64;
                            m_uint64 = static_cast<uint64_t>(m_real) + other.m_uint64;
                        }
                        else
                        {
                            m_tag = VT_INT;
                            m_int = static_cast<int32_t>(m_real) + static_cast<int32_t>(other.m_uint64);
                        }
                    }
                    else
                    {
                        m_real += other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    m_real += other.m_real;
                    break;
                default:
                    throw TypeError("Cannot add types");
            }
            break;
        case VT_STRING:
            if (other.m_tag == VT_STRING)
            {
                *m_string += *other.m_string;
            }
            break;
        default:
            throw TypeError("Cannot add types");
    }
}

void Variable::operator-=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int -= other.m_int;
                    break;
                case VT_UINT64:
                    m_int = m_int - other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int -= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_int - other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot subtract types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int > m_uint64)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 - other.m_int;
                    }
                    else
                    {
                        m_uint64 -= other.m_int;
                    }
                    break;
                case VT_UINT64:
                    if (other.m_uint64 > m_uint64)
                    {
                        m_tag = VT_INT;
                        m_uint64 = static_cast<int32_t>(m_uint64) - static_cast<int32_t>(other.m_uint64);
                    }
                    else
                    {
                        m_uint64 -= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real) && m_uint64 >= other.m_real)
                    {
                        m_uint64 -= (uint64_t) other.m_real;
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_uint64 - other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot subtract types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_INT;
                        m_int = static_cast<int32_t>(m_real) - other.m_int;
                    }
                    else
                    {
                        m_real -= other.m_int;
                    }
                    break;
                case VT_UINT64:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_INT;
                        m_int = static_cast<int32_t>(m_real) - other.m_uint64;
                    }
                    else
                    {
                        m_real -= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    m_real -= other.m_real;
                    break;
                default:
                    throw TypeError("Cannot subtract types");
            }
            break;
        default:
            throw TypeError("Cannot subtract types");
    }
}

void Variable::operator*=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int *= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int * other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int *= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_int * other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot multiply types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 * other.m_int;
                        break;
                    }
                    else
                    {
                        m_uint64 *= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 *= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        if (other.m_real >= 0)
                        {
                            m_uint64 *= (uint64_t) other.m_real;
                        }
                        else
                        {
                            m_tag = VT_INT;
                            m_int = m_uint64 * static_cast<int32_t>(other.m_real);
                        }
                    }
                    else
                    {
                        m_tag = VT_REAL;
                        m_real = m_uint64 * other.m_real;
                    }
                    break;
                default:
                    throw TypeError("Cannot multiply types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_INT;
                        m_int = m_real * other.m_int;
                    }
                    else
                    {
                        m_real *= other.m_int;
                    }
                    break;
                case VT_UINT64:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real * other.m_uint64;
                    }
                    else
                    {
                        m_real *= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    m_real *= other.m_real;
                    break;
                default:
                    throw TypeError("Cannot multiply types");
            }
            break;
        default:
            throw TypeError("Cannot multiply types");
    }
}

void Variable::divzero()
{
    m_tag = VT_REAL;
    m_real = std::numeric_limits<real_t>::infinity();
}

void Variable::operator/=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_int) / other.m_int;
                    break;
                case VT_UINT64:
                    if (other.m_uint64 == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_int) / other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = m_int / other.m_real;
                    break;
                default:
                    throw TypeError("Cannot divide types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_uint64) / other.m_int;
                    break;
                case VT_UINT64:
                    if (other.m_uint64 == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_uint64) / other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == 0)
                    {
                        divzero();
                        break;
                    }
                    m_tag = VT_REAL;
                    m_real = m_uint64 / other.m_real;
                    break;
                default:
                    throw TypeError("Cannot divide types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int == 0)
                    {
                        divzero();
                        break;
                    }
                    m_real /= other.m_int;
                    break;
                case VT_UINT64:
                    if (other.m_uint64 == 0)
                    {
                        divzero();
                        break;
                    }
                    m_real /= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == 0)
                    {
                        divzero();
                        break;
                    }
                    m_real /= other.m_real;
                    break;
                default:
                    throw TypeError("Cannot divide types");
            }
            break;
        default:
            throw TypeError("Cannot divide types");
    }
}

void Variable::operator%=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int == 0) throw DivideByZeroError();
                    m_tag = VT_INT;
                    m_int %= other.m_int;
                    break;
                case VT_UINT64:
                    if (other.m_uint64 == 0) throw DivideByZeroError();
                    m_tag = VT_UINT64;
                    m_uint64 = m_int % other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        if (other.m_real == 0) throw DivideByZeroError();
                        m_int %= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 % real");
                    }
                    break;
                default:
                    throw TypeError("Cannot % types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 % other.m_int;
                        break;
                    }
                    else
                    {
                        if (other.m_int == 0) throw DivideByZeroError();
                        m_uint64 %= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    if (other.m_uint64 == 0) throw DivideByZeroError();
                    m_uint64 %= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        if (other.m_real >= 0)
                        {
                            m_uint64 %= (uint64_t) other.m_real;
                        }
                        else
                        {
                            if (other.m_real == 0) throw DivideByZeroError();
                            m_tag = VT_INT;
                            m_int = m_uint64 % static_cast<int32_t>(other.m_real);
                        }
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 % real");
                    }
                    break;
                default:
                    throw TypeError("Cannot % types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (m_real == floor(m_real))
                    {
                        if (other.m_int == 0) throw DivideByZeroError();
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int %= other.m_int;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real  % int");
                    }
                    break;
                case VT_UINT64:
                    if (m_real == floor(m_real))
                    {
                        if (other.m_uint64 == 0) throw DivideByZeroError();
                        m_uint64 = m_real;
                        m_tag = VT_UINT64;
                        m_uint64 %= other.m_uint64;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real % uint");
                    }
                    break;
                case VT_REAL:
                    if (m_real == floor(m_real) && other.m_real == floor(other.m_real))
                    {
                        if (other.m_real == 0) throw DivideByZeroError();
                        m_int = m_real;
                        m_tag = VT_INT;
                        m_int %= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        if (other.m_real == 0)
                        {
                            throw DivideByZeroError();
                        }
                        else
                        {
                            m_real -= floor(m_real / other.m_real) * other.m_real;
                        }
                    }
                    break;
                default:
                    throw TypeError("Cannot % types");
            }
            break;
        default:
            throw TypeError("Cannot % types");
    }
}

void Variable::operator<<=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_UINT64;
                    m_uint64 = static_cast<uint64_t>(m_int) << other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int & other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = static_cast<uint64_t>(m_int) << static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 << real");
                    }
                    break;
                default:
                    throw TypeError("Cannot << types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        throw MiscError("negative leftshift");
                    }
                    else
                    {
                        m_uint64 <<= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 <<= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_uint64 <<= (uint64_t) other.m_real;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 & real");
                    }
                    break;
                default:
                    throw TypeError("Cannot & types");
                    break;
            }
            break;
        case VT_REAL:
            if (m_real != std::floor(m_real))
            {
                throw TypeError("Cannot & with real");
            }
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_int <<= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 &= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 <<= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real << real");
                    }
                    break;
                default:
                    throw TypeError("Cannot << types");
            }
            break;
        default:
            throw TypeError("Cannot << types");
    }
}

void Variable::operator>>=(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_UINT64;
                    m_uint64 = static_cast<uint64_t>(m_int) >> other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int & other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = static_cast<uint64_t>(m_int) >> static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 >> real");
                    }
                    break;
                default:
                    throw TypeError("Cannot >> types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        throw MiscError("negative leftshift");
                    }
                    else
                    {
                        m_uint64 >>= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 >>= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_uint64 >>= (uint64_t) other.m_real;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 & real");
                    }
                    break;
                default:
                    throw TypeError("Cannot & types");
                    break;
            }
            break;
        case VT_REAL:
            if (m_real != std::floor(m_real))
            {
                throw TypeError("Cannot & with real");
            }
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_int >>= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 &= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 >>= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real >> real");
                    }
                    break;
                default:
                    throw TypeError("Cannot >> types");
            }
            break;
        default:
            throw TypeError("Cannot >> types");
    }
}


void Variable::operator&= (const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int &= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int & other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int &= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 & real");
                    }
                    break;
                default:
                    throw TypeError("Cannot & types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 & other.m_int;
                        break;
                    }
                    else
                    {
                        m_uint64 &= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 &= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_uint64 &= (uint64_t) other.m_real;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 & real");
                    }
                    break;
                default:
                    throw TypeError("Cannot & types");
                    break;
            }
            break;
        case VT_REAL:
            if (m_real != std::floor(m_real))
            {
                throw TypeError("Cannot & with real");
            }
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    {
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int &= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 &= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int &= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real & real");
                    }
                    break;
                default:
                    throw TypeError("Cannot & types");
            }
            break;
        default:
            throw TypeError("Cannot & types");
    }
}
void Variable::operator|= (const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int |= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int | other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int |= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 | real");
                    }
                    break;
                default:
                    throw TypeError("Cannot | types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 | other.m_int;
                        break;
                    }
                    else
                    {
                        m_uint64 |= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 |= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_uint64 |= (uint64_t) other.m_real;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 | real");
                    }
                    break;
                default:
                    throw TypeError("Cannot | types");
                    break;
            }
            break;
        case VT_REAL:
            if (m_real != std::floor(m_real))
            {
                throw TypeError("Cannot | with real");
            }
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    {
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int |= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 |= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int |= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real | real");
                    }
                    break;
                default:
                    throw TypeError("Cannot | types");
            }
            break;
            case VT_UNDEFINED:
            {
                // | on undefined just takes the next type's value.
                // TODO: check this is true ^
                copy(other);
            }
            break;
        default:
            throw TypeError("Cannot | types");
    }
}

void Variable::operator^= (const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int ^= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int ^ other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int ^= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 ^ real");
                    }
                    break;
                default:
                    throw TypeError("Cannot ^ types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 ^ other.m_int;
                        break;
                    }
                    else
                    {
                        m_uint64 ^= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 ^= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_uint64 ^= (uint64_t) other.m_real;
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 ^ real");
                    }
                    break;
                default:
                    throw TypeError("Cannot ^ types");
                    break;
            }
            break;
        case VT_REAL:
            if (m_real != std::floor(m_real))
            {
                throw TypeError("Cannot ^ with real");
            }
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    {
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int ^= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 ^= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int ^= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real ^ real");
                    }
                    break;
                default:
                    throw TypeError("Cannot ^ types");
            }
            break;
        default:
            throw TypeError("Cannot ^ types");
    }
}

// integer division
void Variable::idiv(const Variable& other)
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_tag = VT_INT;
                    m_int /= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int / other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int /= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("int32 div real");
                    }
                    break;
                default:
                    throw TypeError("Cannot integer-divide types");
                    break;
            }
            break;
        case VT_UINT64:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    if (other.m_int < 0)
                    {
                        m_tag = VT_INT;
                        m_int = m_uint64 / other.m_int;
                        break;
                    }
                    else
                    {
                        m_uint64 /= other.m_int;
                        break;
                    }
                case VT_UINT64:
                    m_uint64 /= other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        if (other.m_real >= 0)
                        {
                            m_uint64 /= (uint64_t) other.m_real;
                        }
                        else
                        {
                            m_tag = VT_INT;
                            m_int = m_uint64 / static_cast<int32_t>(other.m_real);
                        }
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("uint64 div real");
                    }
                    break;
                default:
                    throw TypeError("Cannot integer-divide types");
                    break;
            }
            break;
        case VT_REAL:
            switch (other.m_tag)
            {
                case VT_BOOL:
                case VT_INT:
                    m_real = std::floor(m_real);
                    {
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int /= other.m_int;
                    }
                    break;;
                case VT_UINT64:
                    m_real = std::floor(m_real);
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 /= other.m_uint64;
                    }
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_real = std::floor(m_real);
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int /= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        throw UnknownIntendedBehaviourError("real div real");
                    }
                    break;
                default:
                    throw TypeError("Cannot div types");
            }
            break;
        default:
            throw TypeError("Cannot div types");
    }
}

void Variable::invert()
{
    switch (m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            m_tag = VT_UINT64;
            m_uint64 = m_int;
            // falthrough;
        case VT_UINT64:
            m_uint64 = ~m_uint64;
            break;
        case VT_REAL:
            {
                m_uint64 = m_real;
                m_tag = VT_UINT64;
                m_uint64 = ~m_uint64;
            }
            break;
        default:
            throw UnknownIntendedBehaviourError("~ on unexpected type");
    }
}


void Variable::TypeCastError::combine()
{
    m_message += variable_type_string[m_src];
    m_message += " to ";
    m_message += variable_type_string[m_dst];
}

void Variable::gc_integrity_check() const
{
    #ifdef OGM_GARBAGE_COLLECTOR
    if (is_array())
    {
        m_array.gc_integrity_check();
    }
    #endif
}

#ifdef OGM_GARBAGE_COLLECTOR
template <typename Data>
void VariableComponentHandle<Data>::gc_integrity_check() const
{
    m_data->gc_integrity_check();
}

void VariableComponentData::gc_integrity_check() const
{
    g_gc.integrity_check_touch(m_gc_node);
}
#endif

#ifdef OGM_STRUCT_SUPPORT
VariableStructData::VariableStructData()
{
    m_instance = new Instance();
    m_instance->m_is_struct = true;
    
    #ifdef OGM_GARBAGE_COLLECTOR
    // share the garbage collector node.
    m_instance->m_gc_node = m_gc_node;
    #else
    // need this so we can understand "self" in the
    // execution context of the struct.
    m_instance->m_struct_data = this;
    #endif
}

void VariableStructData::cleanup()
{
    if (m_instance)
    {
        m_instance->clear_variables();
    }
}

VariableStructData::~VariableStructData()
{
    if (m_instance)
    {
        delete m_instance;
    }
}
#endif

VariableArrayData::~VariableArrayData()
{
    #ifndef OGM_GARBAGE_COLLECTOR
    cleanup();
    #endif
}

void VariableArrayData::cleanup()
{
    for (auto& r : m_vector)
    {
        #ifdef OGM_2DARRAY
        for (Variable& v : r)
        {
            v.cleanup();
        }
        #else
        r.cleanup();
        #endif
    }
    m_vector.clear();
}

// --------------- explicit template instantiation ---------------------

template class VariableComponentHandle<VariableArrayData>;
template class VariableComponentHandle<VariableStructData>;
#ifdef OGM_GARBAGE_COLLECTOR
    template VariableArrayData& VariableArrayHandle::getWriteable<true>(GCNode*);
    template VariableArrayData& VariableArrayHandle::getWriteable<false>(GCNode*);
    #ifdef OGM_STRUCT_SUPPORT
        template VariableStructData& VariableStructHandle::getWriteable<true>(GCNode*);
        template VariableStructData& VariableStructHandle::getWriteable<false>(GCNode*);
    #endif
#else
    template VariableArrayData& VariableArrayHandle::getWriteable<true>();
    template VariableArrayData& VariableArrayHandle::getWriteable<false>();
    #ifdef OGM_STRUCT_SUPPORT
        template VariableStructData& VariableStructHandle::getWriteable<true>();
        template VariableStructData& VariableStructHandle::getWriteable<false>();
    #endif
#endif
template VariableArrayData& VariableArrayHandle::getWriteableNoCopy<true>();
template VariableArrayData& VariableArrayHandle::getWriteableNoCopy<false>();
template VariableStructData& VariableStructHandle::getWriteableNoCopy<true>();
template VariableStructData& VariableStructHandle::getWriteableNoCopy<false>();

template
void Variable::serialize<false>(
    typename state_stream<false>::state_stream_t& s
    #ifdef OGM_GARBAGE_COLLECTOR
        , GCNode* owner
    #endif
);

template
void Variable::serialize<true>(
    typename state_stream<true>::state_stream_t& s
    #ifdef OGM_GARBAGE_COLLECTOR
        , GCNode* owner
    #endif
);

#if defined(__GNUC__) && !defined(__llvm__)
// MSVC has trouble with this line for some reason.
template
ogm::interpreter::Variable::Variable<char const*>(std::vector<char const*>);
#endif

// FIXME: move these few matrix functions somewhere more appropriate

void identity_matrix(matrix_t& o_matrix)
{
    for (size_t i = 0; i < 16; ++i)
    {
        o_matrix[i] = i % 5 == 0;
    }
}

void variable_to_matrix(const Variable& v, matrix_t& o_matrix)
{
    if (v.is_array())
    {
        if (v.array_height() >= 16)
        {
            for (size_t i = 0; i < 16; ++i)
            {
                o_matrix[i] = v.array_at(OGM_2DARRAY_DEFAULT_ROW i).castCoerce<real_t>();
            }
            return;
        }
    }
    
    // default: identity
    identity_matrix(o_matrix);
}

void matrix_to_variable(const matrix_t& matrix, Variable& out)
{
    for (size_t i = 0; i < 16; ++i)
    {
        out.array_get(OGM_2DARRAY_DEFAULT_ROW i) = matrix[i];
    }
}

}}
