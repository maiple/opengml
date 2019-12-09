#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/geometry/Vector.hpp"

#include "ogm/common/error.hpp"
#include <cmath>
#include <algorithm>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::exp(VO out, V v)
{
  out = std::exp(v.castCoerce<real_t>());
}

void ogm::interpreter::fn::ln(VO out, V v)
{
  out = std::log(v.castCoerce<real_t>());
}

void ogm::interpreter::fn::power(VO out, V base, V exponent)
{
  out = std::pow(base.castCoerce<real_t>(), exponent.castCoerce<real_t>());
}

void ogm::interpreter::fn::sqr(VO out, V v)
{
  real_t _v = v.castCoerce<real_t>();
  out =(_v * _v);
}

void ogm::interpreter::fn::sqrt(VO out, V v)
{
  out = std::sqrt(v.castCoerce<real_t>());
}

void ogm::interpreter::fn::log2(VO out, V v)
{
  out = std::log2(v.castCoerce<real_t>());
}

void ogm::interpreter::fn::log10(VO out, V v)
{
  out = std::log10(v.castCoerce<real_t>());
}

void ogm::interpreter::fn::logn(VO out, V v, V n)
{
  out = std::log2(v.castCoerce<real_t>()) / std::log2(n.castCoerce<real_t>());
}

void ogm::interpreter::fn::point_in_rectangle(VO out, V x, V y, V x1, V y1, V x2, V y2)
{
    ogm::geometry::Vector<real_t> p{ x.castCoerce<real_t>(), y.castCoerce<real_t>() };
    ogm::geometry::Vector<real_t> p1{ x1.castCoerce<real_t>(), y2.castCoerce<real_t>() };
    ogm::geometry::Vector<real_t> p2{ x2.castCoerce<real_t>(), y2.castCoerce<real_t>() };
    out = (p.x >= p1.x && p.y >= p1.y && p.x < p2.x && p.y < p2.y);
}