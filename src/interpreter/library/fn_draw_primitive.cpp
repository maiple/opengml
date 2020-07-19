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
    float* primitive_vertex_buffer = nullptr;
    size_t primitive_vertex_buffer_length = 0;
    size_t primitive_vertex_buffer_capacity = 0;
    
    void reset_vertex()
    {
        primitive_vertex_buffer_length = 0;
    }
    
    float* next_vertex()
    {
        size_t new_length = primitive_vertex_buffer_length + display->get_vertex_size();
        if (new_length > primitive_vertex_buffer_capacity)
        {
            // realloc
            size_t new_capacity = std::max<size_t>(0x40, (primitive_vertex_buffer_capacity * 2));
            float* new_buffer = alloc<float>(new_capacity);
            
            // copy old data 
            memcpy(new_buffer, primitive_vertex_buffer, primitive_vertex_buffer_length * sizeof(float));
            
            // clean up and swap.
            free(primitive_vertex_buffer);
            primitive_vertex_buffer = new_buffer;
        }
        
        float* vertex = primitive_vertex_buffer + primitive_vertex_buffer_length;
        primitive_vertex_buffer_length = new_length;
        return vertex;
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
        primitive_vertex_buffer_length,
        primitive_vertex_buffer,
        tex ? tex->m_tpage : nullptr,
        g_glenum
    );
}