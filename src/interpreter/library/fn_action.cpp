#include "libpre.h"
    #include "all.h"
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
#include <chrono>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::action_execute_script(VO out, unsigned char argc, const Variable* argv)
{
    script_execute(out, argc, argv);
}

void ogm::interpreter::fn::action_another_room(VO out, V room)
{
    room_goto(out, room);
}

void ogm::interpreter::fn::action_colour(VO out, V c)
{
    draw_set_colour(out, c);
}

void ogm::interpreter::fn::action_create_object(VO out, V obj, V x, V y)
{
    instance_create(out, x, y, obj);
}

void ogm::interpreter::fn::action_create_object_motion(VO out, V obj, V x, V y, V speed, V dir)
{
    instance_create(out, x, y, obj);
    Instance* instance = frame.get_instance(out.castCoerce<instance_id_t>());
    // TODO: set speed before create event / cc?
    instance->set_value(28, speed);
    instance->set_value(29, dir);
}

void ogm::interpreter::fn::action_create_object_random(VO out, V obj0, V obj1, V obj2, V obj3, V x, V y)
{
    irandom(out, 4);
    switch (out.castCoerce<size_t>())
    {
    case 0:
        instance_create(out, x, y, obj0);
        break;
    case 1:
        instance_create(out, x, y, obj1);
        break;
    case 2:
        instance_create(out, x, y, obj2);
        break;
    case 3:
        instance_create(out, x, y, obj3);
        break;
    }
}

void ogm::interpreter::fn::action_if(VO out, V c)
{
    out = c.cond();
}

void ogm::interpreter::fn::action_inherited(VO out)
{
    event_inherited(out);
}

void ogm::interpreter::fn::action_kill_object(VO out)
{
    instance_destroy(out);
}
