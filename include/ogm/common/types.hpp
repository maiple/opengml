#pragma once

#include <cstdint>
#include <array>

namespace ogm
{
    #if UINTPTR_MAX > (1ULL<<32)    
      typedef double floatptr_t;
    #else
      typedef float  floatptr_t;
    #endif

    typedef double real_t;
    typedef real_t coord_t;
    
    // matrices are length-16 arrays are indexed as (y,x) -> 4*y + x
    // in other words, ROW-MAJOR order.
    typedef std::array<real_t, 16> matrix_t;
    
    // allows e.g. -1_r
    inline real_t operator"" _r(long double t) {return t;}
    inline coord_t operator"" _c(long double t) {return t;}
    inline real_t operator"" _r(unsigned long long t) {return t;}
    inline coord_t operator"" _c(unsigned long long t) {return t;}
    
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
    
    // gmv2
    typedef uint32_t layer_id_t;
    typedef id_t layer_elt_id_t;
    
    // this compresses layer elements to be 12 bytes
    // layer_id_t is be 24 bits
    #define COMPRESS_OGM_LAYER_ELTS

    // a direct instance id refers directly to an instance id, not
    // to a special value or an object id.
    typedef instance_id_t direct_instance_id_t;

    // and expanded instance id can be either a direct instance id
    // or it can be a special value or object id.
    typedef instance_id_t ex_instance_id_t;
    
    // constants
    static const asset_index_t k_no_asset = -1;
    
    #ifdef  COMPRESS_OGM_LAYER_ELTS
      static const layer_id_t k_no_layer = 0x00ffffff;
    #else
      static const layer_id_t k_no_layer = -1;
    #endif
}
