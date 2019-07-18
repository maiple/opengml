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
#include <chrono>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::parameter_count(VO out)
{
    out = frame.m_data.m_clargs.size() - 1;
}

void ogmi::fn::parameter_string(VO out, V i)
{
    size_t _i = i.castCoerce<size_t>();
    if (_i < frame.m_data.m_clargs.size())
    {
        out = frame.m_data.m_clargs[_i];
    }
}

namespace
{
// https://stackoverflow.com/a/41582957
uint64_t get_time()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t g_start_time = get_time();
}

void ogmi::fn::get_timer(VO out)
{
    out = get_time() - g_start_time;
}
