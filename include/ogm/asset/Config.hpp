#pragma once

#include "AssetObject.hpp"

// global configuration settings for interpreted process.

namespace ogm { namespace asset {

class Config
{
public:
    bytecode_index_t m_default_events[static_cast<size_t>(StaticEvent::COUNT)] = {
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event
    };

    // this assert warns to remind the editor to initialize the above array
    // up to StaticEvent::COUNT.
    // There is nothing else magical about the particular integral value.
    static_assert(static_cast<size_t>(StaticEvent::COUNT) == 9,
        "m_default_events incorrectly initialized.");
    
    // next instance id start
    direct_instance_id_t m_next_instance_id = 10000000;
};

}}