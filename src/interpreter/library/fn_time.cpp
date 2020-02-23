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
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>
#include <chrono>
#include <fstream>

#ifdef _WIN32
#ifdef SYSINFOAPI_ENABLED
#include <windows.h>
#include <sysinfoapi.h>
#endif
#endif

#ifdef APPLE
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>
#endif

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

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
