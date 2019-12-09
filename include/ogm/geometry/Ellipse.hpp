#pragma once

#include "Vector.hpp"
#include "aabb.hpp"

#include "ogm/common/types.hpp"

namespace ogm
{
namespace geometry
{

template<typename coord_t=coord_t>
// ccw triangle class
class Ellipse
{
    typedef geometry::Vector<real_t> vector_t;

    vector_t m_center;

    // rectangle x:1/semimajoraxis y:1/semiminoraxis
    vector_t m_mat;

    real_t m_sin, m_cos;

public:
    Ellipse(vector_t center, vector_t mat, real_t angle)
        : m_center(center)
        , m_mat(1.0/mat.x, 1.0/mat.y)
        , m_sin(std::sin(-angle)) // angle in right-handed radians.
        , m_cos(std::cos(-angle)) // angle in right-handed radians.
    { }

    bool contains(vector_t point) const
    {
        // translate
        point -= m_center;

        // rotate
        vector_t tpoint {
            point.x * m_cos - point.y * m_sin,
            point.y * m_cos + point.x * m_sin
        };

        // scale
        tpoint.x *= m_mat.x;
        tpoint.y *= m_mat.y;

        return tpoint.lengthsq() < 1;
    }

    bool collides_line_segment(vector_t a, vector_t b) const
    {
        // OPTIMIZE
        for (real_t lambda = 0; lambda <= 1; lambda += 0.01)
        {
            vector_t p = a * (1.0 - lambda) + b * lambda;
            if (contains(p)) return true;
        }
        return false;
    }

    bool collides_aabb(AABB<real_t> aabb) const
    {
        if (aabb.contains(m_center))
        {
            return true;
        }

        if (collides_line_segment(aabb.top_left(), aabb.top_right()))
            return true;

        if (collides_line_segment(aabb.top_left(), aabb.bottom_left()))
            return true;

        if (collides_line_segment(aabb.bottom_left(), aabb.bottom_right()))
            return true;

        if (collides_line_segment(aabb.top_right(), aabb.bottom_right()))
            return true;

        return false;
    }
};

}
}
