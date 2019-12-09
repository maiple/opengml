#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::array_length_1d(VO out, V a)
{
    if (a.array_height() > 0)
    {
        out = static_cast<real_t>(a.array_length(0));
    }
    else
    {
        out = static_cast<real_t>(0);
    }
}

void ogm::interpreter::fn::array_height_2d(VO out, V a)
{
    out = static_cast<real_t>(a.array_height());
}

void ogm::interpreter::fn::array_length_2d(VO out, V a, V n)
{
    out = static_cast<real_t>(a.array_length(n.castCoerce<size_t>()));
}

void ogm::interpreter::fn::array_copy(VO out, V a)
{
    if (a.get_type() != VT_ARRAY)
    {
        throw MiscError("Copying array, but source is not an array.");
    }
    out.copy(a);

    // getting a writeable copy of an array copies the array.
    out.get<VariableArrayHandle>().getWriteable();
}

void ogm::interpreter::fn::array_copy(VO out, V dst, V dsti, V src, V srci, V length)
{
    if (dst.get_type() == VT_ARRAY)
    {
        // getting a writeable handle for array
        Variable varr;
        varr.copy(dst);
        VariableArrayData& vdst = varr.get<VariableArrayHandle>().getWriteableNoCopy();
        const VariableArrayData& vsrc = src.get<VariableArrayHandle>().getReadable();
        size_t dst_index = dsti.castCoerce<size_t>();
        size_t src_index = srci.castCoerce<size_t>();
        size_t l = length.castCoerce<size_t>();
        if (vsrc.m_vector.size() < 1)
        {
            throw MiscError("src array is empty.");
        }
        if (vdst.m_vector.size() < 1)
        {
            vdst.m_vector.emplace_back();
        }
        if (vsrc.m_vector.at(0).size() < src_index + l)
        {
            throw MiscError("src array is too small.");
        }
        if (vdst.m_vector.size() < 1)
        {
            vdst.m_vector.emplace_back();
        }
        if (vdst.m_vector.at(0).size() < dst_index + l)
        {
            vdst.m_vector.resize(dst_index + l);
        }
        for (size_t i = 0; i < l; ++i)
        {
            vdst.m_vector.at(0).at(i + dst_index).cleanup();
            vdst.m_vector.at(0).at(i + dst_index).copy(vsrc.m_vector.at(0).at(i + src_index));
        }
        varr.cleanup();
    }
}

void ogm::interpreter::fn::array_create(VO out, V vn)
{
    array_create(out, vn, 0);
}

void ogm::interpreter::fn::array_create(VO out, V vn, V value)
{
    size_t n = vn.castCoerce<size_t>();
    if (n == 0)
    {
        throw UnknownIntendedBehaviourError("Zero-length array.");
    }
    else
    {
        out.array_ensure();
        VariableArrayData& data = out.get<VariableArrayHandle>().getWriteableNoCopy();
        ogm_assert(data.m_vector.size() == 0);
        data.m_vector.emplace_back();
        auto& _vec = data.m_vector.front();
        _vec.reserve(n);
        ogm_assert(_vec.size() == 0);
        for (size_t i = 0; i < n; ++i)
        {
            _vec.emplace_back();
            _vec.back().copy(vn);
        }
    }
}


void ogm::interpreter::fn::array_equals(VO out, V a, V b)
{
    if (a.get_type() != VT_ARRAY || b.get_type() != VT_ARRAY)
    {
        out = false;
        return;
    }
    const VariableArrayData& data_a = a.get<VariableArrayHandle>().getReadable();
    const VariableArrayData& data_b = b.get<VariableArrayHandle>().getReadable();
    const auto& vec_a = data_a.m_vector;
    const auto& vec_b = data_b.m_vector;
    if (vec_a.size() != vec_b.size()) goto different;
    for (size_t i = 0; i < vec_a.size(); ++i)
    {
        const auto& _vec_a = vec_a.at(i);
        const auto& _vec_b = vec_b.at(i);
        if (_vec_a.size() != _vec_b.size()) goto different;
        for (size_t j = 0; j < _vec_a.size(); ++i)
        {
            if (_vec_a.at(i) != _vec_b.at(i)) goto different;
        }
    }

    // arrays are the same.
    out = true;
    return;

different:
    // arrays are different.
    out = false;
    return;
}
