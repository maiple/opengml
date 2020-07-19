#include "xbr.hpp"

#include "ogm/common/types.hpp"
#include "ogm/common/util.hpp"

namespace ogm::project::fx
{
    #ifdef SHDRFX_SUPPORT
    uint8_t * xbr(const uint8_t *, uint16_t w, uint16_t h, uint16_t& ow, uint16_t& oh, void * param)
    {
        const float rads = positive_modulo<float>(*reinterpret_cast<floatptr_t*>(&param), TAU);
        
        return nullptr;
    }
    #endif
}