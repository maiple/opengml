#pragma once

#include "external.h"
#include "ogm/common/util.hpp"
#include "ogm/sys/util_sys.hpp"

namespace ogm::interpreter::ffi
{
    struct SharedLibraryType
    {
        enum {
            ERROR,
            WINDOWS,
            UNIX,
            APPLE,
        } os = ERROR;
        
        enum {
            UNKNOWN,
            x86,
            x64
        } arch = UNKNOWN;
    
        bool archmatch() const
        {
            return (arch == x86 && is_32_bit()) || (arch == x64 && is_64_bit());
        }
        
        bool osmatch() const
        {
            #if defined(__unix__)
            return os == UNIX;
            #endif
            
            #if defined(_WIN32) || defined(WIN32)
            return os == WINDOWS;
            #endif
            
            #if defined(__APPLE__)
            return os == APPLE;
            #endif
            
            return false;
        }
        
        bool platmatch() const
        {
            return archmatch() && osmatch();
        }
        
        bool compatible() const
        {
            if (platmatch()) return true;
            
            #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
            if (os == WINDOWS && zugbruecke_init())
            {
                return true;
            }
            #endif
            
            return false;
        }
    };
    
    SharedLibraryType getSharedLibraryTypeFromPath(const std::string& path);
    
    void path_transform(std::string& path);
}
