#include "libpre.h"
    #include "fn_time.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>

#include <cctype>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <ctime>

#ifdef _WIN32
    #if __has_include(<sysinfoapi.h>)
        #define SYSINFOAPI_ENABLED
        #include <windows.h>
        #include <sysinfoapi.h>
        #include <time.h>
    #endif
#endif

#ifdef APPLE
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>
#endif

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

namespace
{
    // true: local
    // false: true
    bool g_timezone_local = true;
}

void ogm::interpreter::fn::date_set_timezone(VO out, V d)
{
    g_timezone_local = !d.cond();
}

void ogm::interpreter::fn::date_get_timezone(VO out)
{
    out = static_cast<real_t>(!g_timezone_local);
}

namespace
{
    std::time_t g_time;
    
    std::tm* get_time()
    {
        g_time = std::time(0);
        std::tm* now;
        if (g_timezone_local)
        {
            now = std::localtime(&g_time);
        }
        else
        {
            now = std::gmtime(&g_time);
        }
        return now;
    }
}

void ogm::interpreter::fn::getv::current_second(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_sec);
}

void ogm::interpreter::fn::getv::current_minute(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_min);
}

void ogm::interpreter::fn::getv::current_hour(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_hour);
}

void ogm::interpreter::fn::getv::current_day(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_mday);
}

void ogm::interpreter::fn::getv::current_weekday(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_wday);
}

void ogm::interpreter::fn::getv::current_month(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_mon + 1);
}

void ogm::interpreter::fn::getv::current_year(VO out)
{
    std::tm* now = get_time();
    out = static_cast<real_t>(now->tm_year + 1900);
}

// https://stackoverflow.com/a/30095652
void ogm::interpreter::fn::getv::current_time(VO out)
{
    uint64_t uptime;

    #if defined(_WIN32) && defined(SYSINFOAPI_ENABLED)
    uptime = std::chrono::milliseconds(GetTickCount64());
    #endif

    #if defined(__linux__) || (defined(_WIN32) && !defined(SYSINFOAPI_ENABLED))
    std::chrono::milliseconds _uptime(0u);
    double uptime_seconds;
    if (std::ifstream("/proc/uptime", std::ios::in) >> uptime_seconds)
    {
      _uptime = std::chrono::milliseconds(
        static_cast<unsigned long long>(uptime_seconds)*1000ULL
      );
    }
    uptime = _uptime.count();
    #endif

    #ifdef APPLE
    std::chrono::milliseconds _uptime(0u);
    struct timeval ts;
    std::size_t len = sizeof(ts);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &ts, &len, NULL, 0) == 0)
    {
      _uptime = std::chrono::milliseconds(
        static_cast<unsigned long long>(ts.tv_sec)*1000ULL +
        static_cast<unsigned long long>(ts.tv_usec)/1000ULL
      );
    }
    uptime = _uptime.count();
    #endif

    out = static_cast<real_t>(uptime);
}
