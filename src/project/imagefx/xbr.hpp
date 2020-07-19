#pragma once

#include <cstdint>

namespace ogm::project::fx
{
    // param here should be a floatptr_t angle in rads.
    uint8_t * xbr(const uint8_t *, uint16_t w, uint16_t h, uint16_t& ow, uint16_t& oh, void * param);
}