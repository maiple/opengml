#include "libpre.h"
    #include "fn_d3d.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define display frame.m_display

namespace
{
    const std::array<real_t, 16> k_identity =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

void ogm::interpreter::fn::d3d_set_fog(VO out, V enable, V colour, V start, V end)
{
    display->set_fog(enable.castCoerce<bool>(), start.castCoerce<real_t>(), end.castCoerce<real_t>(), colour.castCoerce<int32_t>());
}

void ogm::interpreter::fn::d3d_start(VO out)
{
    display->set_depth_test(true);
    display->set_culling(false); // TODO: true?
    display->set_zwrite(true);
}

void ogm::interpreter::fn::d3d_end(VO out)
{
    display->set_depth_test(false);
    display->set_culling(false);
    display->set_zwrite(false);
    display->set_matrix_projection(k_identity);
}

void ogm::interpreter::fn::d3d_set_hidden(VO out, V enable)
{
    display->set_depth_test(enable.cond());
}

void ogm::interpreter::fn::d3d_set_culling(VO out, V enable)
{
    display->set_culling(enable.cond());
}

void ogm::interpreter::fn::d3d_set_zwriteenable(VO out, V enable)
{
    display->set_zwrite(enable.cond());
}

void ogm::interpreter::fn::d3d_set_projection(VO out, V xfrom, V yfrom, V zfrom, V xto, V yto, V zto, V xup, V yup, V zup)
{
    display->set_camera(
        xfrom.castCoerce<real_t>(),
        yfrom.castCoerce<real_t>(),
        zfrom.castCoerce<real_t>(),
        xto.castCoerce<real_t>(),
        yto.castCoerce<real_t>(),
        zto.castCoerce<real_t>(),
        xup.castCoerce<real_t>(),
        yup.castCoerce<real_t>(),
        zup.castCoerce<real_t>(),
        45.0,
        display->get_window_dimensions().x / display->get_window_dimensions().y,
        0.5,
        100000.0
    );
}

void ogm::interpreter::fn::d3d_set_projection_ext(VO out, V xfrom, V yfrom, V zfrom, V xto, V yto, V zto, V xup, V yup, V zup, V fovangle, V aspect, V znear, V zfar)
{
    display->set_camera(
        xfrom.castCoerce<real_t>(),
        yfrom.castCoerce<real_t>(),
        zfrom.castCoerce<real_t>(),
        xto.castCoerce<real_t>(),
        yto.castCoerce<real_t>(),
        zto.castCoerce<real_t>(),
        xup.castCoerce<real_t>(),
        yup.castCoerce<real_t>(),
        zup.castCoerce<real_t>(),
        fovangle.castCoerce<real_t>() * TAU / 360.0,
        aspect.castCoerce<real_t>(),
        znear.castCoerce<real_t>(),
        zfar.castCoerce<real_t>()
    );
}

void ogm::interpreter::fn::d3d_set_projection_ortho(VO out, V x, V y, V width, V height, V angle)
{
    display->set_camera_ortho(
        x.castCoerce<real_t>(),
        y.castCoerce<real_t>(),
        width.castCoerce<real_t>(),
        height.castCoerce<real_t>(),
        angle.castCoerce<real_t>() * TAU / 360.0
    );
}

// NOTE: NOT CURRENTLY SERIALIZED
namespace
{
    TextureView* g_view = nullptr;
    std::vector<float> g_vertices;
    uint32_t g_vertex_type;
    bool active = false;
}

void ogm::interpreter::fn::d3d_primitive_begin(VO out, V type)
{
    if (active)
    {
        throw MiscError("cannot begin d3d_vertex while already in progress.");
    }
    else
    {
        active = true;
    }

    g_vertices.clear();
    g_view = nullptr;
    g_vertex_type = type.castCoerce<uint32_t>();
}

void ogm::interpreter::fn::d3d_primitive_begin_texture(VO out, V type, V tex)
{
    if (active)
    {
        throw MiscError("cannot begin d3d_vertex while already in progress.");
    }
    else
    {
        active = true;
    }

    g_vertices.clear();
    g_view = static_cast<TextureView*>(tex.castExact<void*>());
    g_vertex_type = type.castCoerce<uint32_t>();
}

namespace
{
    void push_colour(std::vector<float>& vertices, uint32_t colour, float alpha)
    {
        vertices.push_back((colour & (0xff0000) >> 16) / 255.0);
        vertices.push_back((colour & (0x00ff00) >> 8) / 255.0);
        vertices.push_back((colour & (0x0000ff) >> 0) / 255.0);
        vertices.push_back(alpha);
    }
}

void ogm::interpreter::fn::d3d_vertex(VO out, V x, V y, V z)
{
    if (!active)
    {
        throw MiscError("d3d_vertex requires d3d_primitive_begin.");
    }
    else
    {
        // vertex
        g_vertices.push_back(x.castCoerce<real_t>());
        g_vertices.push_back(y.castCoerce<real_t>());
        g_vertices.push_back(z.castCoerce<real_t>());

        // colour
        push_colour(g_vertices, 0xffffff, 1.0);

        // texture coordinates
        g_vertices.push_back(0.0f);
        g_vertices.push_back(0.0f);
    }
}

void ogm::interpreter::fn::d3d_vertex_colour(VO out, V x, V y, V z, V c, V alpha)
{
    if (!active)
    {
        throw MiscError("d3d_vertex requires d3d_primitive_begin.");
    }
    else
    {
        // vertex
        g_vertices.push_back(x.castCoerce<real_t>());
        g_vertices.push_back(y.castCoerce<real_t>());
        g_vertices.push_back(z.castCoerce<real_t>());

        // colour
        push_colour(g_vertices, 0xffffff, 1.0);

        // texture coordinates
        g_vertices.push_back(0.0f);
        g_vertices.push_back(0.0f);
    }
}

void ogm::interpreter::fn::d3d_vertex_texture(VO out, V x, V y, V z, V u, V v)
{
    if (!active)
    {
        throw MiscError("d3d_vertex requires d3d_primitive_begin.");
    }
    else
    {
        // vertex
        g_vertices.push_back(x.castCoerce<real_t>());
        g_vertices.push_back(y.castCoerce<real_t>());
        g_vertices.push_back(z.castCoerce<real_t>());

        // colour
        push_colour(g_vertices, 0xffffff, 1.0);

        // texture coordinates
        g_vertices.push_back(g_view->u_global(u.castCoerce<real_t>()));
        g_vertices.push_back(g_view->v_global(v.castCoerce<real_t>()));
    }
}

void ogm::interpreter::fn::d3d_vertex_texture_colour(VO out, V x, V y, V z, V u, V v, V c, V alpha)
{
    if (!active)
    {
        throw MiscError("d3d_vertex requires d3d_primitive_begin.");
    }
    else
    {
        std::array<real_t, 3> vertices = {
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            z.castCoerce<real_t>(),
        };

        // vertex
        g_vertices.push_back(vertices[0]);
        g_vertices.push_back(vertices[1]);
        g_vertices.push_back(vertices[2]);

        // colour
        push_colour(g_vertices, c.castCoerce<real_t>(), alpha.castCoerce<real_t>());

        // texture coordinates
        g_vertices.push_back(g_view->u_global(u.castCoerce<real_t>()));
        g_vertices.push_back(g_view->v_global(v.castCoerce<real_t>()));
    }
}

void ogm::interpreter::fn::d3d_primitive_end(VO out)
{
    if (!active)
    {
        throw MiscError("d3d_primitive_end requires d3d_primitive_begin.");
    }
    else
    {
        display->render_array(
            g_vertices.size(),
            &g_vertices.at(0),
            nullptr,
            g_vertex_type
        );
        g_vertices.clear();
        g_view = nullptr;
        active = false;
    }
}

void ogm::interpreter::fn::d3d_transform_set_identity(VO out)
{
    display->transform_identity();
}

void ogm::interpreter::fn::d3d_transform_set_translation(VO out, V x, V y, V z)
{
    display->transform_identity();
    std::array<float, 16> mat = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        static_cast<float>(x.castCoerce<real_t>()),
        static_cast<float>(y.castCoerce<real_t>()),
        static_cast<float>(z.castCoerce<real_t>()),
        1.0
    };
    display->transform_apply(mat);
}

void ogm::interpreter::fn::d3d_transform_set_scaling(VO out, V x, V y, V z)
{
    display->transform_identity();
    std::array<float, 16> mat = {
        static_cast<float>(x.castCoerce<real_t>()), 0.0, 0.0, 0.0,
        0.0, static_cast<float>(y.castCoerce<real_t>()), 0.0, 0.0,
        0.0, 0.0, static_cast<float>(z.castCoerce<real_t>()), 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    display->transform_apply(mat);
}

void ogm::interpreter::fn::d3d_transform_set_rotation_x(VO out, V theta)
{
    display->transform_identity();
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 1, 0, 0);
}

void ogm::interpreter::fn::d3d_transform_set_rotation_y(VO out, V theta)
{
    display->transform_identity();
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 0, 1, 0);
}

void ogm::interpreter::fn::d3d_transform_set_rotation_z(VO out, V theta)
{
    display->transform_identity();
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 0, 0, 1);
}

void ogm::interpreter::fn::d3d_transform_set_rotation_axis(VO out, V theta, V x, V y, V z)
{
    if (x != 0 || y != 0 || z != 0)
    {
        display->transform_apply_rotation(theta.castCoerce<real_t>(),
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            z.castCoerce<real_t>()
        );
    }
}

void ogm::interpreter::fn::d3d_transform_add_translation(VO out, V x, V y, V z)
{
    std::array<float, 16> mat = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        static_cast<float>(x.castCoerce<real_t>()),
        static_cast<float>(y.castCoerce<real_t>()),
        static_cast<float>(z.castCoerce<real_t>()),
        1.0
    };
    display->transform_apply(mat);
}

void ogm::interpreter::fn::d3d_transform_add_scaling(VO out, V x, V y, V z)
{
    std::array<float, 16> mat = {
        static_cast<float>(x.castCoerce<real_t>()), 0.0, 0.0, 0.0,
        0.0, static_cast<float>(y.castCoerce<real_t>()), 0.0, 0.0,
        0.0, 0.0, static_cast<float>(z.castCoerce<real_t>()), 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    display->transform_apply(mat);
}

void ogm::interpreter::fn::d3d_transform_add_rotation_x(VO out, V theta)
{
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 1, 0, 0);
}

void ogm::interpreter::fn::d3d_transform_add_rotation_y(VO out, V theta)
{
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 0, 1, 0);
}

void ogm::interpreter::fn::d3d_transform_add_rotation_z(VO out, V theta)
{
    display->transform_apply_rotation(theta.castCoerce<real_t>(), 0, 0, 1);
}

void ogm::interpreter::fn::d3d_transform_add_rotation_axis(VO out, V theta, V x, V y, V z)
{
    if (x != 0 || y != 0 || z != 0)
    {
        display->transform_apply_rotation(theta.castCoerce<real_t>(),
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            z.castCoerce<real_t>()
        );
    }
}

void ogm::interpreter::fn::d3d_transform_stack_clear(VO out)
{
    display->transform_stack_clear();
}

void ogm::interpreter::fn::d3d_transform_stack_empty(VO out)
{
    out = display->transform_stack_empty();
}

void ogm::interpreter::fn::d3d_transform_stack_push(VO out)
{
    out = display->transform_stack_push();
}

void ogm::interpreter::fn::d3d_transform_stack_pop(VO out)
{
    out = display->transform_stack_pop();
}

void ogm::interpreter::fn::d3d_transform_stack_top(VO out)
{
    out = display->transform_stack_top();
}

void ogm::interpreter::fn::d3d_transform_stack_discard(VO out)
{
    out = display->transform_stack_discard();
}

void ogm::interpreter::fn::d3d_transform_vertex(VO out, V x, V y, V z)
{
    std::array<real_t, 3> a;
    a[0] = x.castCoerce<real_t>();
    a[1] = y.castCoerce<real_t>();
    a[2] = z.castCoerce<real_t>();
    display->transform_vertex(a);
    out.array_ensure();
    out.array_get(0, 2) = a[2];
    out.array_get(0, 1) = a[1];
    out.array_get(0, 0) = a[0];
}

void ogm::interpreter::fn::d3d_transform_vertex_model_view(VO out, V x, V y, V z)
{
    std::array<real_t, 3> a;
    a[0] = x.castCoerce<real_t>();
    a[1] = y.castCoerce<real_t>();
    a[2] = z.castCoerce<real_t>();
    display->transform_vertex_mv(a);
    out.array_ensure();
    out.array_get(0, 2) = a[2];
    out.array_get(0, 1) = a[1];
    out.array_get(0, 0) = a[0];
}

void ogm::interpreter::fn::d3d_transform_vertex_model_view_projection(VO out, V x, V y, V z)
{
    std::array<real_t, 3> a;
    a[0] = x.castCoerce<real_t>();
    a[1] = y.castCoerce<real_t>();
    a[2] = z.castCoerce<real_t>();
    display->transform_vertex_mvp(a);
    out.array_ensure();
    out.array_get(0, 2) = a[2];
    out.array_get(0, 1) = a[1];
    out.array_get(0, 0) = a[0];
}

void ogm::interpreter::fn::d3d_draw_floor(VO out, V x1, V y1, V z1, V x2, V y2, V z2, V vtex, V hrepeat, V vrepeat)
{
    if (active) throw MiscError("Cannot draw d3d_* while primitive in progress.");

    display->set_matrix_pre_model();
    real_t x[2], y[2], z[2];
    x[0] = x1.castCoerce<real_t>();
    y[0] = y1.castCoerce<real_t>();
    z[0] = z1.castCoerce<real_t>();
    x[1] = x2.castCoerce<real_t>();
    y[1] = y2.castCoerce<real_t>();
    z[1] = z2.castCoerce<real_t>();

    TextureView* tv = nullptr;
    if (vtex.get_type() == VT_PTR)
    {
        tv = static_cast<TextureView*>(vtex.castExact<void*>());
    }

    int32_t hrep = 1, vrep = 1;
    if (tv)
    {
        hrep = hrepeat.castCoerce<int32_t>();
        vrep = vrepeat.castCoerce<int32_t>();
        if (hrep < 1) hrep = 1;
        if (vrep < 1) vrep = 1;
    }

    // add vertices
    for (int h = 0; h < hrep; ++h)
    {
        for (int v = 0; v < vrep; ++v)
        {
            // OPTIMIZE: use fewer vertices
            for (int a=0; a < 6; ++a)
            {
                int ah = 0, av = 0;
                if (a == 1 || a == 3 || a == 4) ah = 1;
                if (a == 2 || a == 4 || a == 5) av = 1;
                float hp = ((h + ah) / static_cast<float>(hrep));
                float vp = ((v + av) / static_cast<float>(vrep));
                float _x = x[0] * (1 - hp) + x[1] * hp;
                float _y = y[0] * (1 - vp) + y[1] * vp;
                float _z = z[0] * (1 - hp) + z[1] * hp;
                g_vertices.push_back(_x);
                g_vertices.push_back(_y);
                g_vertices.push_back(_z);

                // colour
                push_colour(g_vertices, 0xffffff, 1.0);

                // texture
                if (tv)
                {
                    g_vertices.push_back(tv->u_global(ah));
                    g_vertices.push_back(tv->v_global(av));
                }
                else
                {
                    g_vertices.push_back(ah);
                    g_vertices.push_back(av);
                }
            }
        }
    }

    display->render_array(
        g_vertices.size(),
        &g_vertices.at(0),
        tv ? tv->m_tpage : nullptr,
        4 // pr_trianglelist
    );
    g_vertices.clear();
    g_view = nullptr;
}

void ogm::interpreter::fn::d3d_draw_wall(VO out, V x1, V y1, V z1, V x2, V y2, V z2, V vtex, V hrepeat, V vrepeat)
{
    if (active) throw MiscError("Cannot draw d3d_* while primitive in progress.");

    display->set_matrix_pre_model();
    real_t x[2], y[2], z[2];
    x[0] = x1.castCoerce<real_t>();
    y[0] = y1.castCoerce<real_t>();
    z[0] = z1.castCoerce<real_t>();
    x[1] = x2.castCoerce<real_t>();
    y[1] = y2.castCoerce<real_t>();
    z[1] = z2.castCoerce<real_t>();

    TextureView* tv = nullptr;
    if (vtex.get_type() == VT_PTR)
    {
        tv = static_cast<TextureView*>(vtex.castExact<void*>());
    }

    int32_t hrep = 1, vrep = 1;
    if (tv)
    {
        hrep = hrepeat.castCoerce<int32_t>();
        vrep = vrepeat.castCoerce<int32_t>();
        if (hrep < 1) hrep = 1;
        if (vrep < 1) vrep = 1;
    }

    // add vertices
    for (int h = 0; h < hrep; ++h)
    {
        for (int v = 0; v < vrep; ++v)
        {
            // OPTIMIZE: use fewer vertices
            for (int a = 0; a < 6; ++a)
            {
                int ah = 0, av = 0;
                if (a == 1 || a == 3 || a == 4) ah = 1;
                if (a == 2 || a == 4 || a == 5) av = 1;
                float hp = ((h + ah) / static_cast<float>(hrep));
                float vp = ((v + av) / static_cast<float>(vrep));
                float _x = x[0] * (1 - hp) + x[1] * hp;
                float _y = y[0] * (1 - hp) + y[1] * hp;
                float _z = z[0] * (1 - vp) + z[1] * vp;
                g_vertices.push_back(_x);
                g_vertices.push_back(_y);
                g_vertices.push_back(_z);

                // colour
                push_colour(g_vertices, 0xffffff, 1.0);

                // texture
                if (tv)
                {
                    g_vertices.push_back(tv->u_global(ah));
                    g_vertices.push_back(tv->v_global(av));
                }
                else
                {
                    g_vertices.push_back(ah);
                    g_vertices.push_back(av);
                }
            }
        }
    }

    display->render_array(
        g_vertices.size(),
        &g_vertices.at(0),
        tv ? tv->m_tpage : nullptr,
        4 // pr_trianglelist
    );
    g_vertices.clear();
    g_view = nullptr;
}

void ogm::interpreter::fn::d3d_draw_block(VO out, V x1, V y1, V z1, V x2, V y2, V z2, V t, V h, V v)
{
    d3d_draw_floor(out, x1, y2, z1, x2, y1, z1, t, v, h);
    d3d_draw_wall(out, x1, y2, z1, x1, y1, z2, t, v, h);
    d3d_draw_wall(out, x2, y1, z1, x2, y2, z2, t, v, h);
    d3d_draw_wall(out, x1, y1, z1, x2, y1, z2, t, v, h);
    d3d_draw_wall(out, x2, y2, z1, x1, y2, z2, t, v, h);
    d3d_draw_floor(out, x1, y1, z2, x2, y2, z2, t, v, h);
}
