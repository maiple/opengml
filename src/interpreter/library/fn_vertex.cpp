#include "libpre.h"
    #include "fn_vertex.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define display frame.m_display

namespace
{
    uint32_t g_active_vao = 0;
}

void ogm::interpreter::fn::vertex_format_begin(VO out)
{
    if (g_active_vao)
    {
        throw MiscError("Cannot begin new vertex format; already in progress.");
    }

    g_active_vao = display->make_vertex_format() + 1;
}

void ogm::interpreter::fn::vertex_format_add_colour(VO out)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            VertexFormatAttribute::RGBA,
            VertexFormatAttribute::COLOUR
        }
    );
}

void ogm::interpreter::fn::vertex_format_add_position(VO out)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            VertexFormatAttribute::F2,
            VertexFormatAttribute::POSITION
        }
    );
}

void ogm::interpreter::fn::vertex_format_add_position_3d(VO out)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            VertexFormatAttribute::F3,
            VertexFormatAttribute::POSITION
        }
    );
}

void ogm::interpreter::fn::vertex_format_add_textcoord(VO out)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            VertexFormatAttribute::F2,
            VertexFormatAttribute::TEXCOORD
        }
    );
}

void ogm::interpreter::fn::vertex_format_add_normal(VO out)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            VertexFormatAttribute::F3,
            VertexFormatAttribute::NORMAL
        }
    );
}

void ogm::interpreter::fn::vertex_format_add_custom(VO out, V type, V destination)
{
    if (!g_active_vao)
    {
        throw MiscError("No active vertex format.");
    }

    VertexFormatAttribute::DataType t;
    VertexFormatAttribute::Destination d;

    switch (type.castCoerce<int32_t>())
    {
    case 0:
        t = VertexFormatAttribute::F1;
        break;
    case 1:
        t = VertexFormatAttribute::F2;
        break;
    case 2:
        t = VertexFormatAttribute::F3;
        break;
    case 3:
        t = VertexFormatAttribute::F4;
        break;
    case 4:
        t = VertexFormatAttribute::U4;
        break;
    case 5:
        t = VertexFormatAttribute::RGBA;
        break;
    default:
        throw MiscError("Unrecognized data type");
    }

    switch (destination.castCoerce<int32_t>())
    {
    case 0:
        d = VertexFormatAttribute::POSITION;
        break;
    case 1:
        d = VertexFormatAttribute::COLOUR;
        break;
    case 2:
        d = VertexFormatAttribute::NORMAL;
        break;
    case 3:
        d = VertexFormatAttribute::TEXCOORD;
        break;
        throw MiscError("Unrecognized destination");
    }

    display->vertex_format_append_attribute(
        g_active_vao - 1,
        VertexFormatAttribute
        {
            t,
            d
        }
    );
}

void ogm::interpreter::fn::vertex_format_end(VO out)
{
    display->vertex_format_finish(g_active_vao - 1);
    out = static_cast<real_t>(g_active_vao);
    g_active_vao = 0;
}

void ogm::interpreter::fn::vertex_format_delete(VO out, V id)
{
    display->free_vertex_format(id.castCoerce<int32_t>());
}

void ogm::interpreter::fn::vertex_create_buffer(VO out)
{
    out = static_cast<real_t>(display->make_vertex_buffer());
}

void ogm::interpreter::fn::vertex_create_buffer_ext(VO out, V size)
{
    out = static_cast<real_t>(display->make_vertex_buffer(size.castCoerce<size_t>()));
}

void ogm::interpreter::fn::vertex_get_buffer_size(VO out, V id)
{
    out = static_cast<real_t>(display->vertex_buffer_get_size(id.castCoerce<size_t>()));
}

void ogm::interpreter::fn::vertex_get_number(VO out, V id)
{
    out = static_cast<real_t>(display->vertex_buffer_get_count(id.castCoerce<size_t>()));
}

void ogm::interpreter::fn::vertex_delete_buffer(VO out, V id)
{
    display->free_vertex_buffer(id.castCoerce<size_t>());
}

void ogm::interpreter::fn::vertex_begin(VO out, V id, V format)
{
    display->associate_vertex_buffer_format(id.castCoerce<size_t>(), format.castCoerce<size_t>());
}

void ogm::interpreter::fn::vertex_colour(VO out, V id, V colour, V valpha)
{
    uint64_t col = colour.castCoerce<uint64_t>() & 0xffffff;
    float colours[4];
    colours[0] = col & 0xff0000;
    colours[1] = col & 0xff00;
    colours[2] = col & 0xff;
    colours[3] = valpha.castCoerce<real_t>() * 0xff;

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)colours),
        4 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_normal(VO out, V id, V vx, V vy, V vz)
{
    float data[3];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();
    data[2] = vz.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        3 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_position(VO out, V id, V vx, V vy)
{
    float data[2];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        2 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_position_3d(VO out, V id, V vx, V vy, V vz)
{
    float data[3];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();
    data[2] = vz.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        3 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_texcoord(VO out, V id, V u, V v)
{
    float data[2];
    data[0] = u.castCoerce<real_t>();
    data[1] = v.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        2 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_float4(VO out, V id, V vx, V vy, V vz, V vw)
{
    float data[4];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();
    data[2] = vz.castCoerce<real_t>();
    data[3] = vw.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        4 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_float3(VO out, V id, V vx, V vy, V vz)
{
    float data[3];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();
    data[2] = vz.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        3 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_float2(VO out, V id, V vx, V vy)
{
    float data[2];
    data[0] = vx.castCoerce<real_t>();
    data[1] = vy.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        2 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_float1(VO out, V id, V vx)
{
    float data[1];
    data[0] = vx.castCoerce<real_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        static_cast<uint8_t*>((void*)data),
        1 * sizeof(float)
    );
}

void ogm::interpreter::fn::vertex_ubyte4(VO out, V id, V vx, V vy, V vz, V vw)
{
    uint8_t data[4];
    data[0] = vx.castCoerce<size_t>();
    data[1] = vy.castCoerce<size_t>();
    data[2] = vz.castCoerce<size_t>();
    data[3] = vw.castCoerce<size_t>();

    display->add_vertex_buffer_data(
        id.castCoerce<size_t>(),
        data,
        4 * sizeof(uint8_t)
    );
}

void ogm::interpreter::fn::vertex_end(VO out, V id)
{
    // it doesn't seem like this is supposed to actually do anything.
}

void ogm::interpreter::fn::vertex_freeze(VO out, V id)
{
    display->freeze_vertex_buffer(id.castCoerce<size_t>());
}

void ogm::interpreter::fn::vertex_submit(VO out, V id, V type, V texture)
{
    if (!texture.is_pointer())
    {
        display->render_buffer(
            id.castCoerce<size_t>(),
            nullptr,
            type.castCoerce<uint64_t>()
        );
    }
    else
    {
        display->render_buffer(
            id.castCoerce<size_t>(),
            static_cast<TextureView*>(texture.castExact<void*>())->m_tpage,
            type.castCoerce<uint64_t>()
        );
    }
}
