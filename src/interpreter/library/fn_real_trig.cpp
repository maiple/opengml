#include "library.h"
#include "ogm/interpreter/Variable.hpp"

#include <cassert>
#include <cmath>
#include <algorithm>

using namespace ogmi;
using namespace ogmi::fn;

void ogmi::fn::arccos(VO out, V v)
{
  out = static_cast<real_t>(acos(v.castCoerce<real_t>()));
}

void ogmi::fn::arcsin(VO out, V v)
{
  out = static_cast<real_t>(asin(v.castCoerce<real_t>()));
}

void ogmi::fn::arctan(VO out, V v)
{
  out = static_cast<real_t>(atan(v.castCoerce<real_t>()));
}

void ogmi::fn::arctan2(VO out, V x, V y)
{
  out = static_cast<real_t>(atan2(x.castCoerce<real_t>(), y.castCoerce<real_t>()));
}

void ogmi::fn::sin(VO out, V val)
{
  out = std::sin(val.castCoerce<real_t>());
}

void ogmi::fn::tan(VO out, V val)
{
  out = std::tan(val.castCoerce<real_t>());
}

void ogmi::fn::cos(VO out, V val)
{
  out = std::cos(val.castCoerce<real_t>());
}

void ogmi::fn::darccos(VO out, V v)
{
    Variable x = static_cast<real_t>(std::acos(v.castCoerce<real_t>()));
    radtodeg(out, x);
}

void ogmi::fn::darcsin(VO out, V v)
{
    Variable x = static_cast<real_t>(std::asin(v.castCoerce<real_t>()));
    radtodeg(out, x);
}

void ogmi::fn::darctan(VO out, V v)
{
    Variable x = static_cast<real_t>(std::atan(v.castCoerce<real_t>()));
    radtodeg(out, x);
}

void ogmi::fn::darctan2(VO out, V x, V y)
{
    Variable v = static_cast<real_t>(std::atan2(x.castCoerce<real_t>(), y.castCoerce<real_t>()));
    radtodeg(out, v);
}

void ogmi::fn::dsin(VO out, V val)
{
    degtorad(out, val);
    out = std::sin(out.castCoerce<real_t>());
}

void ogmi::fn::dtan(VO out, V val)
{
    degtorad(out, val);
    out = std::tan(out.castCoerce<real_t>());
}

void ogmi::fn::dcos(VO out, V val)
{
    degtorad(out, val);
    out = std::cos(out.castCoerce<real_t>());
}

void ogmi::fn::degtorad(VO out, V val)
{
    out = val.castCoerce<real_t>() * PI / static_cast<real_t>(180.0);
}

void ogmi::fn::radtodeg(VO out, V val)
{
  out = val.castCoerce<real_t>() * static_cast<real_t>(180.0) / PI;
}

void ogmi::fn::lengthdir_x(VO out, V len, V dir)
{
  out = std::cos(dir.castCoerce<real_t>()) * len.castCoerce<real_t>();
}

void ogmi::fn::lengthdir_y(VO out, V len, V dir)
{
  out = static_cast<real_t>(-1.0) * std::sin(dir.castCoerce<real_t>()) * len.castCoerce<real_t>();
}
