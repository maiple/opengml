#pragma once

#include <cstdint>

namespace ogm::project::fx
{
    // fx_t takes buffer and dimensions as arguments, returns a malloc'd buffer.
    // caller is reponsible for calling free().
    typedef uint8_t* (*fx_t)(const uint8_t*, uint16_t w, uint16_t h, uint16_t& ow, uint16_t& oh, void* param);
    
    // retrieves an effect by name.
    fx_t get_fx(const char* name);
    
    void begin();
    void end();
}