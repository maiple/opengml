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

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::move_towards_point(VO out, V vx, V vy, V vspeed)
{
    real_t xdst = vx.castCoerce<real_t>();
    real_t ydst = vy.castCoerce<real_t>();
    real_t xsrc = staticExecutor.m_self->m_data.m_position.x;
    real_t ysrc = staticExecutor.m_self->m_data.m_position.y;
    
    real_t xdiff = xdst - xsrc;
    real_t ydiff = ydst - ysrc;
    
    real_t len = std::sqrt(xdiff * xdiff + ydiff * ydiff);
    if (len == 0)
    {
        staticExecutor.m_self->m_data.m_hspeed = 0;
        staticExecutor.m_self->m_data.m_vspeed = 0;
    }
    else
    {
        real_t spd = vspeed.castCoerce<real_t>();
        staticExecutor.m_self->m_data.m_hspeed = xdiff / len  * spd;
        staticExecutor.m_self->m_data.m_hspeed = ydiff / len  * spd;
    }
}

void ogm::interpreter::fn::motion_set(VO out, V vdir, V vspeed)
{
    real_t dir = vdir.castCoerce<real_t>();
    real_t spd = vspeed.castCoerce<real_t>();
    
    real_t xp = std::cos(dir * TAU / 360.0);
    real_t yp = -std::sin(dir * TAU / 360.0);
    
    staticExecutor.m_self->m_data.m_hspeed = spd * xp;
    staticExecutor.m_self->m_data.m_vspeed = spd * yp;
}

void ogm::interpreter::fn::motion_add(VO out, V vdir, V vspeed)
{
    real_t dir = vdir.castCoerce<real_t>();
    real_t spd = vspeed.castCoerce<real_t>();
    
    real_t xp = std::cos(dir * TAU / 360.0);
    real_t yp = -std::sin(dir * TAU / 360.0);
    
    staticExecutor.m_self->m_data.m_hspeed += spd * xp;
    staticExecutor.m_self->m_data.m_vspeed += spd * yp;
}