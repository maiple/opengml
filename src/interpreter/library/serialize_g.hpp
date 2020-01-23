#pragma once

#include "ogm/interpreter/serialize.hpp"


namespace ogm::interpreter
{
    // serialize the global (internal linkage) variables in fn_ implementations.
    template<bool write>
    void _serialize_fn_real_rand(typename state_stream<write>::state_stream_t& s);

    template<bool write>
    void _serialize_static_lib_data(typename state_stream<write>::state_stream_t& s)
    {
        _serialize_fn_real_rand<write>(s);
    }

    // serialize game.
    template<bool write>
    void serialize_all(typename state_stream<write>::state_stream_t& s);
}
