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
class Triangle
{
    typedef Vector<coord_t> vector_t;

public:
    vector_t m_p[3];

    bool contains(const vector_t& v) const
    {
        for (size_t i = 0; i < 3; ++i)
        // check each line segment to see if the point is to its left.
        {
            const vector_t& a = m_p[i];
            const vector_t& b = m_p[(i + 1) % 3];
            auto d = determinant(a, b, v);
            if (d > 0)
            {
                return false;
            }
            if (d == 0)
            {
                // this part is tricky, have to make arbitrary decisions.
                // 0 counts as "inside" if a.x < b.x or a.y < b.y
                // TODO: consider dot product with [1, -1] instead?
                if (a.x < b.x || a.y < b.y)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool intersects(const Triangle& v) const
    {
        // check for total containment
        if (contains(v.m_p[0]))
        {
            return true;
        }

        // OPTIMIZE (a lot!)
        for (double p = 0; p < 1; p += 0.01)
        {
            for (size_t i = 0; i < 3; ++i)
            {
                vector_t x = (v.m_p[i] * (1.0 - p)) + (v.m_p[(i + 1) % 3] * p);
                if (contains(x))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool is_ccw() const
    {
        return determinant(m_p[0], m_p[1], m_p[2]) <= 0;
    }

    void make_ccw()
    {
        if (!is_ccw())
        {
            std::swap(m_p[0], m_p[1]);
            assert(is_ccw());
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

private:
    // negative if x is to the left of ab, positive if to the right.
    static double determinant(const vector_t& a, const vector_t& b, const vector_t& x)
    {
        return (b.y - a.y)*(x.x - b.y) - (x.y - b.y)*(b.x - a.x);
    }
};

template<int N, typename coord_t=coord_t>
class SmallTriangleList
{
    typedef Vector<coord_t> vector_t;
    typedef AABB<coord_t> aabb_t;
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

    // TODO: enable-if N==2
    explicit SmallTriangleList(const aabb_t& aabb)
    {
        assert(N == 2);
        m_triangles[0] = Triangle<coord_t>
        {
            aabb.top_left(),
            aabb.top_right(),
            aabb.bottom_left()
        };

        m_triangles[1] = Triangle<coord_t>
        {
            aabb.bottom_left(),
            aabb.top_right(),
            aabb.bottom_right()
        };

        m_triangles[0].make_ccw();
        m_triangles[1].make_ccw();
    }


    bool contains(const vector_t& v) const
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (m_triangles[i].contains(v))
            {
                return true;
            }
        }
        return false;
    }

    bool intersects(const Triangle<coord_t>& t) const
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (m_triangles[i].intersects(t))
            {
                return true;
            }
        }
        return false;
    }

    template<int M>
    bool intersects(const SmallTriangleList<M>& t) const
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (t.intersects(m_triangles[i]))
            {
                return true;
            }
        }
        return false;
    }
};

}
}
