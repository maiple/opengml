#include "libpre.h"
    #include "fn_misc.h"
    #include "ogm/fn_ogmeta.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include <cctype>
#include <cstdlib>
#include <chrono>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::parameter_count(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_clargs.size());
}

void ogm::interpreter::fn::parameter_string(VO out, V i)
{
    size_t _i = i.castCoerce<size_t>();
    if (_i < frame.m_data.m_clargs.size())
    {
        out = frame.m_data.m_clargs[_i];
    }
}

void ogm::interpreter::fn::environment_get_variable(VO out, V name)
{
    std::string _name = name.castCoerce<std::string>();
    if (const char* value = getenv(_name.c_str()))
    {
        out = std::string(value);
    }
    else
    {
        // TODO: confirm.
        out = -1.0;
    }
}

namespace
{
    // https://stackoverflow.com/a/41582957
    uint64_t get_time()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // time program starts.
    const uint64_t g_start_time = get_time();

    uint64_t g_delta_time = 0;
}

void ogm::interpreter::fn::get_timer(VO out)
{
    out = static_cast<real_t>(get_time() - g_start_time);
}

void ogm::interpreter::fn::getv::delta_time(VO out)
{
    out = static_cast<real_t>(g_delta_time);
}

void ogm::interpreter::fn::ogm_set_delta_time(VO out, V in)
{
    g_delta_time = in.castCoerce<uint64_t>();
}

namespace
{
    real_t g_fps_sampled = 0.0;
}

void ogm::interpreter::fn::ogm_fps_sample(VO out, V us)
{
    real_t microseconds = us.castCoerce<real_t>();
    g_fps_sampled = 1000000.0 / microseconds;
}

void ogm::interpreter::fn::getv::fps_real(VO out)
{
    out = static_cast<real_t>(std::floor(g_fps_sampled));
}

void ogm::interpreter::fn::getv::fps(VO out)
{
    out = static_cast<real_t>(std::floor(std::min(g_fps_sampled, frame.m_data.m_desired_fps)));
}

void ogm::interpreter::fn::setv::health(V a)
{
    frame.m_data.m_health = a.castCoerce<real_t>();
}

void ogm::interpreter::fn::setv::score(V a)
{
    frame.m_data.m_score = a.castCoerce<real_t>();
}

void ogm::interpreter::fn::setv::lives(V a)
{
    frame.m_data.m_lives = a.castCoerce<real_t>();
}

void ogm::interpreter::fn::getv::health(VO out)
{
    out = frame.m_data.m_health;
}

void ogm::interpreter::fn::getv::score(VO out)
{
    out = frame.m_data.m_score;
}

void ogm::interpreter::fn::getv::lives(VO out)
{
    out = frame.m_data.m_lives;
}

void ogm::interpreter::fn::getv::debug_mode(VO out)
{
    out = !!staticExecutor.m_debugger;
}