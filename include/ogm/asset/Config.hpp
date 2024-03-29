#pragma once

#include "AssetObject.hpp"

// global configuration settings for interpreted process.

namespace ogm { namespace asset {

class Config
{
public:
    // bytecode indices for default object events.
    // when a project is compiled, some of these are filled in.
    bytecode_index_t m_default_events[static_cast<size_t>(StaticEvent::COUNT)] = {
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event
    };

    // this ogm_assert warns to remind the editor to initialize the above array
    // up to StaticEvent::COUNT.
    // There is nothing else magical about the particular integral value.
    static_assert(static_cast<size_t>(StaticEvent::COUNT) == 9,
        "m_default_events incorrectly initialized.");

    // next instance id start
    direct_instance_id_t m_next_instance_id = 10000000;
    
    #ifdef OGM_LAYERS
    layer_elt_id_t m_next_layer_elt_id = 0;
    layer_id_t m_next_layer_id = 0;
    #endif
    
    std::string m_relative_cache_directory = ".cache";
    bool m_cache = false;
    
    // only matters if PARALLEL_COMPILE is defined.
    bool m_parallel_compile = true;
    
    int m_parse_flags = 0;
    
    // configurations to allow setting different macro/constant values
    std::string m_configuration_name;
};

}}
