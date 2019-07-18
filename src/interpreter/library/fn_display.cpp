#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::window_set_position(VO out, V x, V y)
{
    frame.m_display->set_window_position(x.castCoerce<real_t>(), x.castCoerce<real_t>());
}

void ogmi::fn::window_set_size(VO out, V x, V y)
{
    frame.m_display->set_window_size(x.castCoerce<real_t>(), x.castCoerce<real_t>());
}

void ogmi::fn::display_get_width(VO out)
{
    out = frame.m_display->get_display_dimensions().x;
}

void ogmi::fn::display_get_height(VO out)
{
    out = frame.m_display->get_display_dimensions().y;
}

void ogmi::fn::window_get_colour(VO out)
{
    out = frame.m_display->get_clear_colour();
}

void ogmi::fn::window_set_colour(VO out, V c)
{
    frame.m_display->set_clear_colour(c.castCoerce<uint32_t>());
}

void ogmi::fn::window_get_fullscreen(VO out)
{
    // not implemented yet.
    out = false;
}

void ogmi::fn::getv::view_enabled(VO out)
{
    out = frame.m_data.m_views_enabled;
}

void ogmi::fn::setv::view_enabled(V e)
{
    frame.m_data.m_views_enabled = e.cond();
}

void ogmi::fn::getv::view_current(VO out)
{
    out = frame.m_data.m_view_current;
}

void ogmi::fn::getv::view_visible(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_visible[view_index];
}

void ogmi::fn::setv::view_visible(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogmi::fn::getv::view_xview(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_position[view_index].x;
}

void ogmi::fn::getv::view_yview(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_position[view_index].y;
}

void ogmi::fn::setv::view_xview(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogmi::fn::setv::view_yview(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogmi::fn::getv::view_wview(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_dimension[view_index].x;
}

void ogmi::fn::getv::view_hview(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_dimension[view_index].y;
}

void ogmi::fn::setv::view_wview(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogmi::fn::setv::view_hview(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
}

void ogmi::fn::getv::view_visible(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_visible[view_index];
}

void ogmi::fn::setv::view_visible(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogmi::fn::getv::view_xview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_position[view_index].x;
}

void ogmi::fn::getv::view_yview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_position[view_index].y;
}

void ogmi::fn::setv::view_xview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogmi::fn::setv::view_yview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogmi::fn::getv::view_wview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_dimension[view_index].x;
}

void ogmi::fn::getv::view_hview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_dimension[view_index].y;
}

void ogmi::fn::setv::view_wview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogmi::fn::setv::view_hview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
}

void ogmi::fn::getv::view_angle(VO out, V i, V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_angle[view_index];
}

void ogmi::fn::setv::view_angle(VO out, V i, V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}

void ogmi::fn::getv::view_angle(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = frame.m_data.m_view_angle[view_index];
}

void ogmi::fn::setv::view_angle(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}
