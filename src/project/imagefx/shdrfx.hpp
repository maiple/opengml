#pragma once

namespace ogm::project::fx
{
    #ifdef SHDRFX_SUPPORT
    bool shdrfx_begin();
    
    void shdrfx_end();
    #endif
}