#pragma once

#include "Vector.hpp"

#include "ogm/common/types.hpp"

namespace ogm
{
namespace geometry
{

template<typename coord_t=coord_t>
class Triangle
{
    typedef Vector<coord_t> vector_t;

public:
    vector_t m_a, m_b, m_c;
};

template<int N, typename coord_t=coord_t>
class SmallTriangleList
{
public:
    Triangle<coord_t> m_triangles[N];

    SmallTriangleList(size_t c, Triangle<coord_t>* v)
    {
        for (size_t i = 0; i < c; ++i)
        {
            m_triangles[i] = v[i];
        }
    }

    SmallTriangleList(Triangle<coord_t> v[N])
    {
        for (size_t i = 0; i < N; ++i)
        {
            m_triangles[i] = v[i];
        }
    }
};

}
}
