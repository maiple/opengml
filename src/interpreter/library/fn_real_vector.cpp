#include "libpre.h"
    #include "fn_math.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"

#include "ogm/common/error.hpp"
#include <algorithm>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::point_direction(VO out, V x1, V y1, V x2, V y2)
{
    real_t a = static_cast<real_t>(atan2(y1.castCoerce<real_t>() - y2.castCoerce<real_t>(), x2.castCoerce<real_t>() - x1.castCoerce<real_t>()));
    if (a < 0) a += TAU;
    Variable v = a;
    radtodeg(out, v);
}

void ogm::interpreter::fn::point_distance(VO out, V x1, V y1, V x2, V y2)
{
  auto dx = x1.castCoerce<real_t>() - x2.castCoerce<real_t>();
  auto dy = y1.castCoerce<real_t>() - y2.castCoerce<real_t>();
  out = std::sqrt(dx * dx + dy * dy);
}

void ogm::interpreter::fn::point_distance_3d(VO out, V x1, V y1, V z1, V x2, V y2, V z2)
{
  auto dx = x1.castCoerce<real_t>() - x2.castCoerce<real_t>();
  auto dy = y1.castCoerce<real_t>() - y2.castCoerce<real_t>();
  auto dz = z1.castCoerce<real_t>() - z2.castCoerce<real_t>();
  out = std::sqrt(dx*dx + dy*dy + dz*dz);
}

void ogm::interpreter::fn::dot_product(VO out, V x1, V y1, V x2, V y2)
{
  out = (x1.castCoerce<real_t>() * x2.castCoerce<real_t>()
       + y1.castCoerce<real_t>() * y2.castCoerce<real_t>());
}

void ogm::interpreter::fn::dot_product_3d(VO out, V x1, V y1, V z1, V x2, V y2, V z2)
{
  out = (x1.castCoerce<real_t>() * x2.castCoerce<real_t>()
       + y1.castCoerce<real_t>() * y2.castCoerce<real_t>()
       + z1.castCoerce<real_t>() * z2.castCoerce<real_t>());
}

void ogm::interpreter::fn::dot_product_normalised(VO out, V x1, V y1, V x2, V y2)
{
  // slightly inefficient -- takes sqrt then sqr
  dot_product(out, x1, y1, x2, y2);
  Variable pd;
  point_distance(pd, x1, y1, x2, y2);
  pd *= pd;
  out /= pd;
}

void ogm::interpreter::fn::dot_product_normalised_3d(VO out, V x1, V y1, V z1, V x2, V y2, V z2)
{
  // slightly inefficient -- takes sqrt then sqr
  dot_product_3d(out, x1, y1, z1, x2, y2, z2);
  Variable pd;
  point_distance_3d(pd, x1, y1, z1, x2, y2, z2);
  pd  *= pd;
  out /= pd;
}

void ogm::interpreter::fn::angle_difference(VO out, V a1, V a2)
{
  auto _a1 = a1.castCoerce<real_t>();
  auto _a2 = a2.castCoerce<real_t>();
  out = static_cast<real_t>(fmod((fmod((_a2 - _a1) , 360) + 360 + 180), 360) - 180);
}
