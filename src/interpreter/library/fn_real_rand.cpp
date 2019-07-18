#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <cassert>
#include <cmath>
#include <stdlib.h>
#include <random>

using namespace ogmi;
using namespace ogmi::fn;

namespace
{
    typedef std::default_random_engine rng_t;
    rng_t g_rng;
    rng_t::result_type g_seed;
}

void ogmi::fn::randomize(VO out)
{
    g_rng.seed(std::random_device()());
}

void ogmi::fn::random_set_seed(VO out, V r)
{
    g_seed = r.castCoerce<rng_t::result_type>();
    g_rng.seed(g_seed);
}

void ogmi::fn::random_get_seed(VO out)
{
    out = g_seed;
}

void ogmi::fn::choose(VO out, byte n, const Variable* v)
{
    std::uniform_int_distribution<rng_t::result_type> udist(0, n);
    out.copy(v[udist(g_rng)]);
}

void ogmi::fn::irandom(VO out, V v)
{
    
    if (v < 0)
    {
        throw UnknownIntendedBehaviourError("irandom() on negative range");
    }
    
    uint64_t max = v.castCoerce<uint64_t>();
    
    std::uniform_int_distribution<rng_t::result_type> udist(0, max);
    out = udist(g_rng);
}

void ogmi::fn::random(VO out, V v)
{
    
    if (v < 0)
    {
        throw UnknownIntendedBehaviourError("random() on negative range");
    }
    
    real_t max = v.castCoerce<real_t>();
    
    std::uniform_real_distribution<real_t> udist(0, max);
    out = udist(g_rng);
}

void ogmi::fn::irandom_range(VO out, V a, V b)
{
    uint64_t min = a.castCoerce<uint64_t>();
    uint64_t max = b.castCoerce<uint64_t>();
    
    if (max < min)
    {
        throw UnknownIntendedBehaviourError("irandom_range() on negative range");
    }
    
    std::uniform_int_distribution<rng_t::result_type> udist(min, max);
    out = udist(g_rng);
}

void ogmi::fn::random_range(VO out, V a, V b)
{
    real_t min = a.castCoerce<real_t>();
    real_t max = b.castCoerce<real_t>();
    
    if (max < min)
    {
        throw UnknownIntendedBehaviourError("random_range() on negative range");
    }
    
    std::uniform_real_distribution<real_t> udist(min, max);
    out = udist(g_rng);
}