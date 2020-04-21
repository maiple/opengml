#include "libpre.h"
    #include "fn_display.h"
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

void ogm::interpreter::fn::window_set_position(VO out, V x, V y)
{
    frame.m_display->set_window_position(x.castCoerce<real_t>(), y.castCoerce<real_t>());
}

void ogm::interpreter::fn::window_set_size(VO out, V x, V y)
{
    frame.m_display->set_window_size(x.castCoerce<real_t>(), y.castCoerce<real_t>());
}

void ogm::interpreter::fn::display_get_width(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_display_dimensions().x);
}

void ogm::interpreter::fn::display_get_height(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_display_dimensions().y);
}

void ogm::interpreter::fn::window_get_width(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_window_dimensions().x);
}

void ogm::interpreter::fn::window_get_height(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_window_dimensions().y);
}

void ogm::interpreter::fn::window_get_colour(VO out)
{
    out = frame.m_display->get_clear_colour();
}

void ogm::interpreter::fn::window_set_colour(VO out, V c)
{
    frame.m_display->set_clear_colour(c.castCoerce<uint32_t>());
}

void ogm::interpreter::fn::window_get_fullscreen(VO out)
{
    // not implemented yet.
    out = false;
}

void ogm::interpreter::fn::getv::view_enabled(VO out)
{
    out = frame.m_data.m_views_enabled;
}

void ogm::interpreter::fn::setv::view_enabled(V e)
{
    frame.m_data.m_views_enabled = e.cond();
}

void ogm::interpreter::fn::getv::view_current(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_view_current);
}

#ifdef OGM_2DARRAY
#define OGM_2DARRAY_i V i,
#else
#define OGM_2DARRAY_i
#endif

void ogm::interpreter::fn::getv::view_visible(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_visible[view_index];
}

void ogm::interpreter::fn::setv::view_visible(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogm::interpreter::fn::getv::view_xview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
}

void ogm::interpreter::fn::getv::view_yview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
}

void ogm::interpreter::fn::setv::view_xview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_yview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_wview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
}

void ogm::interpreter::fn::getv::view_hview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
}

void ogm::interpreter::fn::setv::view_wview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_hview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_visible(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_visible[view_index]);
}

void ogm::interpreter::fn::setv::view_visible(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogm::interpreter::fn::getv::view_xview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
}

void ogm::interpreter::fn::getv::view_yview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
}

void ogm::interpreter::fn::setv::view_xview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_yview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_wview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
}

void ogm::interpreter::fn::getv::view_hview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
}

void ogm::interpreter::fn::setv::view_wview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_hview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_angle(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
}

void ogm::interpreter::fn::setv::view_angle(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}

void ogm::interpreter::fn::getv::view_angle(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
}

void ogm::interpreter::fn::setv::view_angle(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}

void ogm::interpreter::fn::getv::mouse_x(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_mouse_coord_invm().x);
}

void ogm::interpreter::fn::getv::mouse_y(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_mouse_coord_invm().y);
}

void ogm::interpreter::fn::window_mouse_get_x(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_mouse_coord().x);
}

void ogm::interpreter::fn::window_mouse_get_y(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_mouse_coord().y);
}
