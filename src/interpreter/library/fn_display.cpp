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

void ogm::interpreter::fn::display_reset(VO out, V aa, V vsync)
{
    frame.m_display->set_vsync(vsync.cond());
    
    // TODO: anti-aliasing
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
