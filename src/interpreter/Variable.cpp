#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <cmath>

namespace ogmi
{

constexpr const char* const variable_type_string[] = {
  "undefined",
  "bool",
  "int",
  "uin64"
  "real",
  "string",
  "array",
  "pointer",
};

bool Variable::operator==(const Variable& other) const
{
    switch (m_tag)
    {
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
            // checks if arrays point to the same underlying data.
            return &m_array.getReadable() == &other.m_array.getReadable();
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
                    return m_int >= other.m_uint64;
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
                    m_uint64 -= other.m_int;
                    break;
                case VT_UINT64:
                    m_uint64 -= other.m_uint64;
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
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_int) / other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_int) / other.m_uint64;
                    break;
                case VT_REAL:
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
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_uint64) / other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_REAL;
                    m_real = static_cast<real_t>(m_uint64) / other.m_uint64;
                    break;
                case VT_REAL:
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
                    m_real /= other.m_int;
                    break;
                case VT_UINT64:
                    m_real /= other.m_uint64;
                    break;
                case VT_REAL:
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
                    m_tag = VT_INT;
                    m_int %= other.m_int;
                    break;
                case VT_UINT64:
                    m_tag = VT_UINT64;
                    m_uint64 = m_int % other.m_uint64;
                    break;
                case VT_REAL:
                    if (other.m_real == std::floor(other.m_real))
                    {
                        m_int %= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("int32 % real");
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
                        m_uint64 %= other.m_int;
                        break;
                    }
                case VT_UINT64:
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
                            m_tag = VT_INT;
                            m_int = m_uint64 % static_cast<int32_t>(other.m_real);
                        }
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("uint64 % real");
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
                        m_tag = VT_INT;
                        m_int = m_real;
                        m_int %= other.m_int;
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("real  % int");
                    }
                    break;
                case VT_UINT64:
                    if (m_real == floor(m_real))
                    {
                        m_tag = VT_UINT64;
                        m_uint64 = m_real;
                        m_uint64 %= other.m_uint64;
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("real % uint");
                    }
                    break;
                case VT_REAL:
                    if (m_real == floor(m_real) && other.m_real == floor(other.m_real))
                    {
                        m_real = VT_INT;
                        m_int = m_real;
                        m_int %= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("real % real");
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

void Variable::operator<<=(const Variable&)
{ throw NotImplementedError("leftshift"); }
void Variable::operator>>=(const Variable&)
{ throw NotImplementedError("rightshift"); }
void Variable::operator&= (const Variable&)
{ throw NotImplementedError("bw-&"); }
void Variable::operator|= (const Variable&)
{ throw NotImplementedError("bw-|"); }
void Variable::operator^= (const Variable&)
{ throw NotImplementedError("bw-^"); }

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
                        UnknownIntendedBehaviourError("int32 div real");
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
                        UnknownIntendedBehaviourError("uint64 div real");
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
                        m_real = VT_INT;
                        m_int = m_real;
                        m_int /= static_cast<int32_t>(other.m_real);
                    }
                    else
                    {
                        UnknownIntendedBehaviourError("real div real");
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
            m_int = ~m_int;
            break;
        case VT_UINT64:
            m_uint64 = ~m_uint64;
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

}
