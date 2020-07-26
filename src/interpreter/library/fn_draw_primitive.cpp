#include "libpre.h"
    #include "fn_draw.h"
    #include "fn_string.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define display frame.m_display

namespace
{
    uint8_t g_glenum;
    TextureView* tex = nullptr;
    std::vector<float> vbuff;
    size_t vcount = 0;
    size_t vcap = 0;
    
    void reset_vertex()
    {
        vcount = 0;
        vcap = 32;
        vbuff.resize(vcap);
    }
    
    float* next_vertex()
    {
        size_t next_vcount = vcount + display->get_vertex_size();
        if (next_vcount > vcap)
        {
            vcap *= 2;
            vbuff.resize(vcap);
        }
        float* f = &vbuff[vcount];
        vcount = next_vcount;
        return f;
    }
}

void ogm::interpreter::fn::draw_primitive_begin(VO out, V glenum)
{
    reset_vertex();
    g_glenum = glenum.castCoerce<int32_t>();
}

void ogm::interpreter::fn::draw_vertex(VO out, V x, V y)
{
    display->write_vertex(
        next_vertex(),
        x.castCoerce<real_t>(),
        y.castCoerce<real_t>(),
        0,
        display->get_colour4()
    );
}

void ogm::interpreter::fn::draw_vertex_colour(VO out, V x, V y, V col, V alpha)
{
    draw_vertex_texture_colour(out, x, y, 0, 0, col, alpha);
}

void ogm::interpreter::fn::draw_vertex_texture(VO out, V x, V y, V u, V v)
{
    real_t _u = u.castCoerce<real_t>();
    real_t _v = u.castCoerce<real_t>();

    if (tex)
    {
        _u = tex->u_global(_u);
        _v = tex->v_global(_v);
    }

    display->write_vertex(
        next_vertex(),
        x.castCoerce<real_t>(),
        y.castCoerce<real_t>(),
        0, // z
        display->get_colour4(),
        _u,
        _v
    );
}

void ogm::interpreter::fn::draw_vertex_texture_colour(VO out, V x, V y, V u, V v, V col, V alpha)
{
    real_t _u = u.castCoerce<real_t>();
    real_t _v = u.castCoerce<real_t>();

    uint32_t a = clamp<int>(alpha.castCoerce<real_t>() * 0xff, 0, 0xff);
    uint32_t c = col.castCoerce<uint64_t>();

    // remap u,v to texture coordinates.
    if (tex)
    {
        _u = tex->u_global(_u);
        _v = tex->v_global(_v);
    }

    display->write_vertex(
        next_vertex(),
        x.castCoerce<real_t>(),
        y.castCoerce<real_t>(),
        0, // z
        a | (c << 8),
        _u,
        _v
    );
}

void ogm::interpreter::fn::draw_primitive_end(VO out)
{
    display->render_array(
        vcount,
        &vbuff.at(0),
        tex ? tex->m_tpage : nullptr,
        g_glenum
    );
}