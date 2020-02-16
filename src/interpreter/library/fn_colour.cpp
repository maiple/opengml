#include "libpre.h"
    #include "fn_colour.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::make_colour_rgb(VO out, V vr, V vg, V vb)
{
    uint32_t r = vr.castCoerce<uint32_t>() & 0xff;
    uint32_t g = vg.castCoerce<uint32_t>() & 0xff;
    uint32_t b = vb.castCoerce<uint32_t>() & 0xff;
    
    // BGR colour (not RGB!)
    out = (b << 16) | (g << 8) | r; 
}

#define clamp(a, b, c) (a < b ? b : (a > c ? c : a))

// https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
void ogm::interpreter::fn::make_colour_hsv(VO out, V vh, V vs, V vv)
{
    // TODO: confirm correctness
    double h = vh.castCoerce<real_t>();
    double s = vs.castCoerce<real_t>();
    double v = vv.castCoerce<real_t>();
    h = clamp(h, 0, 255);
    s = clamp(s, 0, 255);
    v = clamp(v, 0, 255);
    
    double hp = 6.0 * (h / 255.0);
    double chroma = v * s;
    
    // x is the second-largest rgb component.
    double x = (1 - std::abs(fmod(hp, 2) - 1)) * chroma;    
    
    float r, g, b;
    switch (static_cast<size_t>(hp))
    {
    case 0: // red-yellow
        r = chroma;
        g = x;
        b = 0;
        break;
    case 1: // yellow-green
        r = x;
        g = chroma;
        b = 0;
        break;
    case 2: // green-teal
        r = 0;
        g = chroma;
        b = x;
        break;
    case 3: // teal-blue
        r = 0;
        g = x;
        b = chroma;
        break;
    case 4: // blue-purple
        r = x;
        g = 0;
        b = chroma;
        break;
    case 5: // purple-red
        r = chroma;
        g = 0;
        b = x;
        break;
    default:
        r = v;
        b = v;
        g = v;
        break;
    }
    
    // make up remaining value.
    float m = v - chroma;
    r += m;
    g += m;
    b += m;
    
    uint32_t rz = r;
    uint32_t gz = g;
    uint32_t bz = b;
    
    // BGR colour (not RGB!)
    out = (bz << 16) | (gz << 8) | rz; 
}

void ogm::interpreter::fn::merge_colour(VO out, V a, V b, V vx)
{
    uint32_t ra = (a.castCoerce<uint32_t>() & 0xff);
    uint32_t ga = (a.castCoerce<uint32_t>() & 0xff00) >> 8;
    uint32_t ba = (a.castCoerce<uint32_t>() & 0xff0000) >> 16;
    
    uint32_t rb = (b.castCoerce<uint32_t>() & 0xff);
    uint32_t gb = (b.castCoerce<uint32_t>() & 0xff00) >> 8;
    uint32_t bb = (b.castCoerce<uint32_t>() & 0xff0000) >> 16;
    
    float x = vx.castCoerce<real_t>();
    
    uint32_t rz = ra * (1 - x) + rb * x;
    if (rz > 255) rz = 255;
    uint32_t gz = ga * (1 - x) + gb * x;
    if (gz > 255) gz = 255;
    uint32_t bz = ba * (1 - x) + bb * x;
    if (bz > 255) bz = 255;
    
    // BGR colour (not RGB!)
    out = (bz << 16) | (gz << 8) | rz; 
}

void ogm::interpreter::fn::colour_get_red(VO out, V c)
{
    out = (c.castCoerce<uint32_t>() & 0xff);
}

void ogm::interpreter::fn::colour_get_blue(VO out, V c)
{
    out = (c.castCoerce<uint32_t>() & 0xff00) >> 8;
}

void ogm::interpreter::fn::colour_get_green(VO out, V c)
{
    out = (c.castCoerce<uint32_t>() & 0xff0000) >> 16;
}

namespace
{
    // OPTIMIZE: force inline.
    inline void rgb_to_hsv(V c, VO hue, VO val, VO sat)
    {
        uint32_t r = (c.castCoerce<uint32_t>() & 0xff);
        uint32_t g = (c.castCoerce<uint32_t>() & 0xff00) >> 8;
        uint32_t b = (c.castCoerce<uint32_t>() & 0xff0000) >> 16;
        
        uint32_t _max = r;
        if (g > r) _max = g;
        if (b > _max) _max = b;
        uint32_t _min = r;
        if (g < r) _min = g;
        if (b < _min) _min = b;
        
        val = _max;
        
        double _range = static_cast<double>(_max - _min);
        
        if (_max == _min)
        {
            hue = 0;
        }
        else if (_max == r)
        {
            hue = (255.0 / 6.0) * (g - b) / _range;
        }
        else if (_max == g)
        {
            hue = (255.0 / 6.0) * (2.0 + (b - r) / _range);
        }
        else //if (_max == b)
        {
            hue = (255.0 / 6.0) * (4.0 + (r - g) / _range);
        }
        
        if (hue < 0)
        {
            hue += 255.0;
        }
        else if (hue >= 255.0)
        {
            hue -= 255.0;
        }
        
        if (_max == 0)
        {
            sat = 0;
        }
        else
        {
            sat = _range / _max;
        }
    }
}

// HSV are all calculated independently, so the compiler should be able to
// optimize out these dummy calculations completely.

void ogm::interpreter::fn::colour_get_hue(VO out, V c)
{
    Variable dummy[2];
    rgb_to_hsv(c, out, dummy[0], dummy[1]);
}

void ogm::interpreter::fn::colour_get_saturation(VO out, V c)
{
    Variable dummy[2];
    rgb_to_hsv(c, dummy[0], out, dummy[1]);
}

void ogm::interpreter::fn::colour_get_value(VO out, V c)
{
    Variable dummy[2];
    rgb_to_hsv(c, dummy[0], dummy[1], out);
}