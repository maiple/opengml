#pragma once

#include "Vector.hpp"

#include "ogm/common/types.hpp"

namespace ogm
{
namespace geometry
{

template<typename coord_t=coord_t>
// ccw triangle class
class Triangle
{
    typedef Vector<coord_t> vector_t;

public:
    vector_t m_p[3];

    bool is_ccw() const
    {
        // TODO
        return false;
    }

    void make_ccw()
    {
        if (!is_ccw())
        {
            std::swap(m_p[0], m_p[1]);
        }
    }

    Triangle operator+(vector_t v) const
    {
        return {
            m_p[0] + v,
            m_p[1] + v,
            m_p[2] + v
        };
    }

    Triangle rotated(double angle) const
    {
        return {
            m_p[0].add_angle_copy(angle),
            m_p[1].add_angle_copy(angle),
            m_p[2].add_angle_copy(angle)
        };
    }
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
