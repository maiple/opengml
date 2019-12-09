#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "serialize_g.hpp"

#include "ogm/common/error.hpp"
#include <cmath>
#include <stdlib.h>
#include <random>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

namespace
{
    typedef std::default_random_engine rng_t;
    rng_t g_rng;
    rng_t::result_type g_seed;
}

// serialize globals

template<>
void ogm::interpreter::_serialize_fn_real_rand<true>(typename state_stream<true>::state_stream_t& s)
{
    s << g_rng;
    _serialize<true>(s, g_seed);
}

template<>
void ogm::interpreter::_serialize_fn_real_rand<false>(typename state_stream<false>::state_stream_t& s)
{
    s >> g_rng;
    _serialize<false>(s, g_seed);
}

void ogm::interpreter::fn::randomize(VO out)
{
    g_rng.seed(std::random_device()());
}

void ogm::interpreter::fn::random_set_seed(VO out, V r)
{
    g_seed = r.castCoerce<rng_t::result_type>();
    g_rng.seed(g_seed);
}

void ogm::interpreter::fn::random_get_seed(VO out)
{
    out = static_cast<real_t>(g_seed);
}

void ogm::interpreter::fn::choose(VO out, byte n, const Variable* v)
{
    if (n == 0)
    {
        throw MiscError("choose requires at least 1 argument.");
    }
    std::uniform_int_distribution<rng_t::result_type> udist(0, n - 1);
    out.copy(v[udist(g_rng)]);
}

void ogm::interpreter::fn::irandom(VO out, V v)
{
    if (v < 0)
    {
        throw UnknownIntendedBehaviourError("irandom() on negative range");
    }

    uint64_t max = v.castCoerce<uint64_t>();

    std::uniform_int_distribution<rng_t::result_type> udist(0, max);
    out = static_cast<real_t>(udist(g_rng));
}

void ogm::interpreter::fn::random(VO out, V v)
{
    real_t max = v.castCoerce<real_t>();

    if (max < 0)
    {
        std::uniform_real_distribution<real_t> udist(0, -max);
        out = -udist(g_rng);
    }
    else
    {
        std::uniform_real_distribution<real_t> udist(0, max);
        out = udist(g_rng);
    }
}

void ogm::interpreter::fn::irandom_range(VO out, V a, V b)
{
    int32_t min = a.castCoerce<int32_t>();
    int32_t max = b.castCoerce<int32_t>();

    if (max < min)
    {
        std::swap(min, max);
    }

    std::uniform_int_distribution<rng_t::result_type> udist(min, max);
    out = static_cast<real_t>(udist(g_rng));
}

void ogm::interpreter::fn::random_range(VO out, V a, V b)
{
    real_t min = a.castCoerce<real_t>();
    real_t max = b.castCoerce<real_t>();

    if (max < min)
    {
        std::swap(min, max);
    }

    std::uniform_real_distribution<real_t> udist(min, max);
    out = udist(g_rng);
}
