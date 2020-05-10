#pragma once

#include <cstdint>

namespace ogm
{
    typedef double real_t;
    typedef real_t coord_t;
    typedef uint32_t variable_id_t;
    // these should be at most 24 bits.
    typedef uint32_t bytecode_index_t;
    typedef uint32_t struct_id_t;
    typedef uint32_t asset_index_t;

    // instance/asset id mixed namespace.
    // uint32_t IDs to last 1 day if 1000 instances are created every frame at 60hz.
    // int64_t ID is enough to last for 100 years creating 100,000,000 instances per frame at 30hz.
    typedef int64_t id_t;
    
    typedef id_t tile_id_t;

    typedef id_t instance_id_t;

    // a direct instance id refers directly to an instance id, not
    // to a special value or an object id.
    typedef instance_id_t direct_instance_id_t;

    // and expanded instance id can be either a direct instance id
    // or it can be a special value or object id.
    typedef instance_id_t ex_instance_id_t;
    
    // constants
    static const asset_index_t k_no_asset = -1;
}
