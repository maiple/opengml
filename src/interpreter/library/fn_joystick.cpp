#include "libpre.h"
    #include "fn_joystick.h"
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

void ogm::interpreter::fn::gamepad_is_supported(VO out)
{
    out = frame.m_display->get_joysticks_supported();
}

void ogm::interpreter::fn::gamepad_get_device_count(VO out)
{
    out = static_cast<real_t>(frame.m_display->get_joystick_max());
}

void ogm::interpreter::fn::gamepad_is_connected(VO out, V i)
{
    out = frame.m_display->get_joystick_connected(i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::gamepad_get_description(VO out, V i)
{
    out = frame.m_display->get_joystick_name(i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::gamepad_axis_count(VO out, V i)
{
    out = static_cast<real_t>(frame.m_display->get_joystick_axis_count(i.castCoerce<int32_t>()));
}

void ogm::interpreter::fn::gamepad_axis_value(VO out, V i, V a)
{
    // OGM constants for axes begin at 15.
    out = static_cast<real_t>(frame.m_display->get_joystick_axis_value(i.castCoerce<int32_t>(), a.castCoerce<int32_t>() - 15));
}

void ogm::interpreter::fn::gamepad_button_count(VO out, V i)
{
    out = frame.m_display->get_joystick_button_count(i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::gamepad_button_value(VO out, V i, V j)
{
    // OGM constants for axes begin at 15.
    real_t r = static_cast<real_t>(frame.m_display->get_joystick_axis_value(i.castCoerce<int32_t>(), j.castCoerce<int32_t>() - 15));
    r *= -1;
    r += 1;
    r *= 0.5;
    r = std::clamp(r, 0.0, 1.0);
    out = r;
}

void ogm::interpreter::fn::gamepad_button_check(VO out, V j, V i)
{
    out = frame.m_display->get_joystick_button_down(j.castCoerce<int32_t>(), i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::gamepad_button_check_pressed(VO out, V j, V i)
{
    out = frame.m_display->get_joystick_button_pressed(j.castCoerce<int32_t>(), i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::gamepad_button_check_released(VO out, V j, V i)
{
    out = frame.m_display->get_joystick_button_released(j.castCoerce<int32_t>(), i.castCoerce<int32_t>());
}

void ogm::interpreter::fn::joystick_exists(VO out, V i)
{
    out = frame.m_display->get_joystick_connected(i.castCoerce<int32_t>() + 1);
}

void ogm::interpreter::fn::joystick_has_pov(VO out, V i)
{
    out = false;
}

void ogm::interpreter::fn::joystick_pov(VO out, V i)
{
    out = 0;
}
