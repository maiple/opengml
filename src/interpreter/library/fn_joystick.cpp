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

void ogmi::fn::gamepad_is_supported(VO out)
{
    out = frame.m_display->get_joysticks_supported();
}

void ogmi::fn::gamepad_get_device_count(VO out)
{
    out = frame.m_display->get_joystick_max();
}

void ogmi::fn::gamepad_is_connected(VO out, V i)
{
    out = frame.m_display->get_joystick_connected(i.castCoerce<int32_t>());
}

void ogmi::fn::gamepad_get_description(VO out, V i)
{
    out = frame.m_display->get_joystick_name(i.castCoerce<int32_t>());
}

void ogmi::fn::gamepad_axis_count(VO out, V i)
{
    out = frame.m_display->get_joystick_axis_count(i.castCoerce<int32_t>());
}

void ogmi::fn::gamepad_axis_value(VO out, V i, V a)
{
    out = frame.m_display->get_joystick_axis_value(i.castCoerce<int32_t>(), a.castCoerce<int32_t>());
}

void ogmi::fn::joystick_exists(VO out, V i)
{
    out = frame.m_display->get_joystick_connected(i.castCoerce<int32_t>());
}

void ogmi::fn::joystick_has_pov(VO out, V i)
{
    out = false;
}

void ogmi::fn::joystick_pov(VO out, V i)
{
    out = 0;
}

void ogmi::fn::gamepad_button_check(VO out, V i, V b)
{
    // TODO
    out = false;
}

void ogmi::fn::gamepad_button_check_pressed(VO out, V i, V b)
{
    // TODO
    out = false;
}

void ogmi::fn::gamepad_button_check_released(VO out, V i, V b)
{
    // TODO
    out = false;
}

void ogmi::fn::gamepad_button_value(VO out, V i, V b)
{
    // TODO
    out = 0;
}
