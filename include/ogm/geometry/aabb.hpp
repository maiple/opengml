#pragma once

#include "Vector.hpp"

#include "ogm/common/types.hpp"

namespace ogm
{
namespace geometry
{

// axis-aligned bounding box
// half-open.
template<typename coord_t=coord_t>
class AABB
{
    typedef Vector<coord_t> vector_t;
public:
    // half-open [m_start, m_end)
    vector_t m_start, m_end;

    inline bool contains(vector_t v) const
    {
        return v.x >= m_start.x && v.y >= m_start.y
            && v.x < m_end.x && v.y < m_end.y;
    }

    inline bool intersects(const AABB& other) const
    {
        // comparing two half-open ranges suggests use of < or > only.
        if (other.m_end.x > m_start.x)
            if (other.m_start.x < m_end.x)
                if (other.m_end.y > m_start.y)
                    if (other.m_start.y < m_end.y)
                        return true;
        return false;
    }

    inline bool intersects_line(const vector_t& start, const vector_t& end) const
    {
        // OPTIMIZE (a lot!)
        if (contains(start) || contains(end)) return true;

        // FIXME -- error-prone at small scales :(
        double distance = start.distance_to(end);
        for (double i = 0; i <= distance; i += 1.0)
        {
            const vector_t v = start + (end - start) * (i / distance);
            if (contains(v)) return true;
        }

        return false;
    }

    AABB<coord_t> operator+(const Vector<coord_t>& v) const
    {
        AABB<coord_t> copy = *this;
        copy.m_start += v;
        copy.m_end += v;
        return copy;
    }

    AABB<coord_t> operator*(const Vector<coord_t>& v) const
    {
        AABB<coord_t> copy = *this;
        copy.m_start.x *= v.x;
        copy.m_start.y *= v.y;
        copy.m_end.x *= v.x;
        copy.m_end.y *= v.y;
        return copy;
    }

    Vector<coord_t> get_center() const
    {
        return (m_start + m_end) / 2;
    }

    Vector<coord_t> get_dimensions() const
    {
        return m_end - m_start;
    }
};

}
}
