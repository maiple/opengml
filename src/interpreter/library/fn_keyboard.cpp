#include "libpre.h"
    #include "fn_keyboard.h"
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

void ogm::interpreter::fn::io_clear(VO out)
{
    display->clear_keys();
}

void ogm::interpreter::fn::keyboard_check(VO out, V key)
{
    out = display->get_key_down(key.castCoerce<int>());
}

void ogm::interpreter::fn::keyboard_check_pressed(VO out, V key)
{
    if (key.castCoerce<int>() == 1)
    // anykey
    {
        out = !!display->get_current_key();
    }
    else
    {
        out = display->get_key_pressed(key.castCoerce<int>());
    }
}

void ogm::interpreter::fn::keyboard_check_released(VO out, V key)
{
    out = display->get_key_released(key.castCoerce<int>());
}

void ogm::interpreter::fn::keyboard_check_direct(VO out, V key)
{
    // TODO: check even if application not in focue
    out = display->get_key_direct(key.castCoerce<int>());
}

namespace
{
    #define CONST(x, y) constexpr size_t x = y;
    #include "fn_keycodes.h"
}

void ogm::interpreter::fn::getv::mouse_button(VO out)
{
    for (size_t i = mb_left; i <= mb_middle; ++i)
    {
        if (display->get_key_down(i))
        {
            out = i;
            return;
        }
    }
    out = mb_none;
}

void ogm::interpreter::fn::mouse_check_button(VO out, V key)
{
    out = display->get_key_down(key.castCoerce<int>());
}

void ogm::interpreter::fn::mouse_check_button_pressed(VO out, V key)
{
    out = display->get_key_pressed(key.castCoerce<int>());
}

void ogm::interpreter::fn::mouse_check_button_released(VO out, V key)
{
    out = display->get_key_released(key.castCoerce<int>());
}

void ogm::interpreter::fn::mouse_wheel_up(VO out)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::mouse_wheel_down(VO out)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::getv::keyboard_key(VO out)
{
    out = display->get_current_key();
}

void ogm::interpreter::fn::setv::keyboard_key(V val)
{
    // TODO
}

void ogm::interpreter::fn::getv::keyboard_lastkey(VO out)
{
    // TODO
    out = display->get_current_key();
}

void ogm::interpreter::fn::setv::keyboard_lastkey(V val)
{
    // TODO
}

void ogm::interpreter::fn::getv::keyboard_lastchar(VO out)
{
    // TODO
}

void ogm::interpreter::fn::setv::keyboard_lastchar(V val)
{
    // TODO
}
