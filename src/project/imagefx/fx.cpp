/// shader execution.

#include <cstring>
#include "fx.hpp"

#include "xbr.hpp"
#include "shdrfx.hpp"

#include "ogm/common/error.hpp"

namespace ogm::project::fx
{
    static bool init = false;
    
    fx_t get_fx(const char* name)
    {
        if (!init) return nullptr;
        
        #ifdef SHDRFX_SUPPORT
        if (strcmp(name, "xbr") == 0)
        {
            return xbr;
        }
        #endif
        
        return nullptr;
    }
    
    void begin()
    {
        if (init) return;
        init = true;
        
        #ifdef SHDRFX_SUPPORT
        if (shdrfx_begin())
        {
            throw MiscError("Failed to init shdrfx for compilation.");
        }
        #endif
    }
    
    void end()
    {
        if (!init) return;
        init = false;
        
        #ifdef SHDRFX_SUPPORT
        shdrfx_end();
        #endif
    }
}