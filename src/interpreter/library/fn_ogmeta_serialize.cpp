#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "serialize_g.hpp"

#include <sstream>
#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

namespace
{
    typedef int32_t state_id_t;

    std::map<state_id_t, std::stringstream> g_state_stream;
    bool g_queued_save = false;
    bool g_queued_load = false;
}

void ogm::interpreter::fn::ogm_save_state(VO out)
{
    ogm_save_state(out, 0);
}

void ogm::interpreter::fn::ogm_load_state(VO out)
{
    ogm_load_state(out, 0);
}

void ogm::interpreter::fn::ogm_save_state(VO out, V n)
{
    std::stringstream& s = g_state_stream[
        n.castCoerce<state_id_t>()
    ];
    g_queued_save = false;
    s.seekp(std::ios_base::beg);
    s.clear();
    ogm_assert(s.good());
    auto a = s.tellg();
    staticExecutor.serialize<true>(s);
    _serialize_canary<true>(s);
    ogm_assert(s.good());
    _serialize_all<true>(s);
    ogm_assert(s.good());
    _serialize_canary<true>(s);
    ogm_assert(s.good());
    // assert no read occurred.
    ogm_assert(a == s.tellg());
}

void ogm::interpreter::fn::ogm_load_state(VO out, V n)
{
    auto iter = g_state_stream.find(n.castCoerce<state_id_t>());
    if (iter != g_state_stream.end())
    {
        std::stringstream& s = iter->second;
        g_queued_load = false;
        s.seekg(std::ios_base::beg);
        s.clear();
        ogm_assert(s.good());
        ogm_assert(!s.eof());
        auto a = s.tellp();
        staticExecutor.serialize<false>(s);
        _serialize_canary<false>(s);
        ogm_assert(s.good());
        _serialize_all<false>(s);
        ogm_assert(s.good());
        _serialize_canary<false>(s);
        ogm_assert(s.good());
        // assert no read occurred.
        ogm_assert(a == s.tellp());
    }
}

void ogm::interpreter::fn::ogm_queue_save_state(VO out)
{
    g_queued_save = true;
}

void ogm::interpreter::fn::ogm_queue_load_state(VO out)
{
    g_queued_load = true;
}

void ogm::interpreter::fn::ogm_save_state_queued(VO out)
{
    out = g_queued_save;
}

void ogm::interpreter::fn::ogm_load_state_queued(VO out)
{
    out = g_queued_load;
}
