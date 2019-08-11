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

    inline AABB()
    { }

    inline AABB(vector_t start, vector_t end)
        : m_start(start), m_end(end)
    { }

    inline AABB(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
        : m_start(x1, y1), m_end(x2, y2)
    { }

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

    // produces the aabb guaranteed to contain {this aabb
    // rotated ccw by angle}
    AABB<coord_t> rotated(double angle) const
    {
        vector_t p[4] = {
            top_left(),
            top_right(),
            bottom_left(),
            bottom_right()
        };
        for (size_t i = 0; i < 4; ++i)
        {
            // rotate ccw
            p[i] = p[i].add_angle_copy(-angle);
        }

        coord_t xmin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
            [](vector_t& v) -> coord_t
            {
                return v.x;
            }
        );

        coord_t xmax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
            [](vector_t& v) -> coord_t
            {
                return v.x;
            }
        );

        coord_t ymin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
            [](vector_t& v) -> coord_t
            {
                return v.y;
            }
        );

        coord_t ymax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
            [](vector_t& v) -> coord_t
            {
                return v.y;
            }
        );

        return {
            { xmin, ymin },
            { xmax, ymax }
        };
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
        copy.correct_sign();
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

    Vector<coord_t> top_left() const
    {
        return m_start;
    }

    Vector<coord_t> top_right() const
    {
        return {m_end.x, m_start.y};
    }

    Vector<coord_t> bottom_left() const
    {
        return {m_start.x, m_end.y};
    }

    Vector<coord_t> bottom_right() const
    {
        return m_end;
    }

    // swaps start and end x/y values where the start > end.
    void correct_sign()
    {
        if (m_start.x > m_end.x) std::swap(m_start.x, m_end.x);
        if (m_start.y > m_end.y) std::swap(m_start.y, m_end.y);
    }

};

}
}
