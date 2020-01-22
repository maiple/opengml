#pragma once

#include "ogm/interpreter/serialize.hpp"

// serialize global (internal linkage) variables in fn_ implementations.

namespace ogm::interpreter
{
    template<bool write>
    void _serialize_fn_real_rand(typename state_stream<write>::state_stream_t& s);

    template<bool write>
    void _serialize_all(typename state_stream<write>::state_stream_t& s)
    {
        _serialize_fn_real_rand<write>(s);
    }
}
