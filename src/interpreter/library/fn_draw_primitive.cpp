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

void ogm::interpreter::fn::draw_primitive_end(VO out)
{
    display->render_array(
        vcount,
        &vbuff.at(0),
        tex ? tex->m_tpage : nullptr,
        g_glenum
    );
}