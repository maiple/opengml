#pragma once

#include "ogm/common/types.hpp"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include <cmath>
#include <limits>

namespace ogm
{
namespace geometry
{

/** Stores an (x,y)-coordinate representing a vector or point in space.
 *  Class is mutable, though most methods support immutable-style
 *  usage. e.g. vector2F::normalize() returns a vector2F with a magnitude
 *  of 1, rather than editing this vector's magnitude. */
template<typename coord_t=real_t>
class Vector
{
public:
    coord_t x;
    coord_t y;

    Vector()
        : x(0)
        , y(0)
    { }

    template<typename T>
    Vector(const Vector<T>& other)
        : x(other.x)
        , y(other.y)
    { }

    Vector(coord_t x, coord_t y)
        : x(x)
        , y(y)
    { }

    /**Creates the vector that would point from (or_x,or_y) to (dst_x,dst_y)*/
    Vector(coord_t or_x, coord_t or_y, coord_t dst_x, coord_t dst_y)
        : x(dst_x - or_x)
        , y(dst_y - or_y)
    { }

    /**Determines rise/run */
    double slope() const
    {
        return (x==0)
            ? std::numeric_limits<float>::max()
            : y/x;
    }

    /**Determines vector angle*/
    double angle() const
    {
        double angle = std::atan2(y, x);
        if (angle < 0)
            angle += 2 * PI;
        return angle;
    }

    /** Squared length.*/
    double lengthsq() const
    {
        return x * x + y * y;
    }

    /**Determines vector magnitude using Pythagoras.*/
    double length() const
    {
        return std::sqrt(x * x + y * y);
    }

    // area of rectangle [0, x] * [0, y]
    coord_t area() const
    {
        return x * y;
    }

    /**True iff the vector is (0,0)*/
    bool is_zero() const
    {
        return (x == 0 && y == 0);
    }

    double distance_to(const Vector<coord_t>& other) const
    {
        return (*this - other).length();
    }

    double angle_distance(const Vector<coord_t>& other) const
    {
        double a = angle();
        double b = other.angle();
        double diff = a-b;
        return std::abs(diff);
    }

    double angle_difference(const Vector<coord_t>& other) const
    {
        double a = angle();
        double b = other.angle();
        double diff = a-b;
        if (diff > PI)
            diff = diff - TAU;
        if (diff < -PI)
            diff = diff + TAU;
        return diff;
    }

    double dot(const Vector<coord_t>& other) const
    {
        return x * other.x + y * other.y;
    }

    // projects onto another vector
    double projection_length(const Vector<coord_t>& other) const
    {
        return this->dot(other) / other.length();
    }

    Vector<coord_t> projection(const Vector<coord_t>& other) const
    {
        return other.normalize_copy() * projection_length(other);
    }

    Vector<coord_t> projection_perp(const Vector<coord_t>& other) const
    {
        return *this - this->projection(other);
    }

    /**returns a vector with the same angle but magnitude 1
     * (or zero, if the magnitude was zero to begin with.*/
    Vector<coord_t> normalize_safe_copy() const
    {
        return *this / this->length();
    }

    /**Sets the magnitude to unit value preserving the angle.
     * Has no effect on the zero-vector.
     * deprecated.*/
    Vector<coord_t>& normalize_apply()
    {
        return *this /= this->length();
    }

    /**multiplies the magnitude by the provided factor value preserving the
     * angle. Has no effect on the zero-vector.*/
    Vector<coord_t>& scale_apply(double factor)
    {
        x *= factor;
        y *= factor;
        return *this;
    }
    Vector<coord_t> scale_copy(double s) const
    {
        return {x * s, y * s};
    }

    Vector<coord_t>& scale_apply(const Vector<coord_t>& factor)
    {
        x *= factor.x;
        y *= factor.y;
        return *this;
    }
    Vector<coord_t> scale_copy(const Vector<coord_t>& s) const
    {
        return {x * s.x, y * s.y};
    }

    Vector<coord_t>& descale_apply(const Vector<coord_t>& factor)
    {
        x /= factor.x;
        y /= factor.y;
        return *this;
    }

    Vector<coord_t> descale_copy(const Vector<coord_t>& s) const
    {
        return {x / s.x, y / s.y};
    }

    Vector<coord_t> operator*(double s) const
    {
        return scale_copy(s);
    }

    Vector<coord_t> operator/(double s) const
    {
        return scale_copy(1.0/s);
    }

    // multiply like complex numbers
    Vector<coord_t> operator*(const Vector<coord_t>& other) const
    {
        return {
            x * other.x - y * other.y,
            x * other.y + y * other.x
        };
    }

    // multiply like complex numbers
    Vector<coord_t> operator/(const Vector<coord_t>& other) const
    {
        real_t div= x * other.x + y * other.y;
        return {
            (x * other.x + y * other.y) / div,
            (y * other.x - x * other.y) / div
        };
    }

    Vector<coord_t> operator+(const Vector<coord_t>& other) const
    {
        return {x + other.x, y + other.y};
    }

    Vector<coord_t> operator+=(const Vector<coord_t>& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector<coord_t> operator-(const Vector<coord_t>& other) const
    {
        return {x - other.x, y - other.y};
    }

    Vector<coord_t> operator-() const
    {
        return {-x, -y};
    }

    Vector<coord_t> operator-=(const Vector<coord_t>& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    bool operator==(const Vector<coord_t>& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector<coord_t>& other) const
    {
        return x != other.x && y != other.y;
    }

    /**Sets the angle to the given value, preserving magnitude. Has no effect
     * on the zero-vector.*/
    Vector<coord_t>& set_angle_apply(double a)
    {
        double mag = this->length();
        x = mag * cos(a);
        y = mag * sin(a);
        return *this;
    }

    /**Sets the magnitude to the given value, preserving the angle. Has no
     * effect on the zero-vector.*/
    Vector<coord_t>& set_length_apply(double m)
    {
        normalize_apply();
        scale_apply(m);
        return *this;
    }

    /**returns a vector offset ccw by the given angle in radians*/
    Vector<coord_t> add_angle_copy(double r) const
    {
         return polar(angle() + r, length());
    }

    /**Constructs a vector2f with the specified x and y value.*/
    static Vector<coord_t> cartesian(double x, double y)
    {
        return {x, y};
    }

    /**Constructs a vector2f at dst with the specified counter-clockwise angle in radians and magnitude.*/
    static Vector<coord_t> polar(double angle, double magnitude)
    {
        return {magnitude * std::cos(angle), magnitude * std::sin(angle)};
    }
};

}
}
