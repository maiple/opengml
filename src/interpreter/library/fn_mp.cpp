#include "libpre.h"
    #include "fn_mp.h"
    #include "fn_math.h"
    #include "fn_collision.h"
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
        staticExecutor.m_self->m_data.m_speed.x = 0;
        staticExecutor.m_self->m_data.m_speed.y = 0;
    }
    else
    {
        real_t spd = vspeed.castCoerce<real_t>();
        staticExecutor.m_self->m_data.m_speed.x = xdiff / len  * spd;
        staticExecutor.m_self->m_data.m_speed.y = ydiff / len  * spd;
    }
}

void ogm::interpreter::fn::motion_set(VO out, V vdir, V vspeed)
{
    real_t dir = vdir.castCoerce<real_t>();
    real_t spd = vspeed.castCoerce<real_t>();

    real_t xp = std::cos(dir * TAU / 360.0);
    real_t yp = -std::sin(dir * TAU / 360.0);

    staticExecutor.m_self->m_data.m_speed.x = spd * xp;
    staticExecutor.m_self->m_data.m_speed.y = spd * yp;
}

void ogm::interpreter::fn::motion_add(VO out, V vdir, V vspeed)
{
    real_t dir = vdir.castCoerce<real_t>();
    real_t spd = vspeed.castCoerce<real_t>();

    real_t xp = std::cos(dir * TAU / 360.0);
    real_t yp = -std::sin(dir * TAU / 360.0);

    staticExecutor.m_self->m_data.m_speed.x += spd * xp;
    staticExecutor.m_self->m_data.m_speed.y += spd * yp;
}

void ogm::interpreter::fn::mp_linear_step(VO out, V gx, V gy, V vstepsize, V vall)
{
    bool all = vall.cond();
    coord_t stepsize = vstepsize.castCoerce<coord_t>();

    frame.process_collision_updates();
    real_t xdst = gx.castCoerce<real_t>();
    real_t ydst = gy.castCoerce<real_t>();
    real_t xsrc = staticExecutor.m_self->m_data.m_position.x;
    real_t ysrc = staticExecutor.m_self->m_data.m_position.y;

    real_t xdiff = xdst - xsrc;
    real_t ydiff = ydst - ysrc;

    real_t len = std::sqrt(xdiff * xdiff + ydiff * ydiff);
    if (len == 0)
    {
        out = true;
    }
    else
    {
        coord_t xnext = xsrc + xdiff / len * stepsize;
        coord_t ynext = ysrc + ydiff / len * stepsize;
        if (len <= stepsize)
        {
            xnext = xdst;
            ynext = ydst;
        }
        bool take_step = false;
        if (all)
        {
            // check for anything
            place_empty(out, xnext, ynext);
            if (out.cond())
            {
                take_step = true;
            }
        }
        else
        {
            // check for solid
            place_free(out, xnext, ynext);
            if (out.cond())
            {
                take_step = true;
            }
        }

        if (take_step)
        {
            staticExecutor.m_self->m_data.m_position.x = xnext;
            staticExecutor.m_self->m_data.m_position.y = ynext;

            frame.queue_update_collision(staticExecutor.m_self);
        }

        out = (xnext == xdst && ynext == ydst);
    }
}
