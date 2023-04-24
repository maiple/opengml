#include "external.h"

namespace ogm::interpreter::ffi
{
    std::map<external_id_t, ExternalDefinition> g_dlls;
    std::map<std::string, void*> g_path_to_dll;
    std::map<void*, size_t> g_dll_refc;
    
    char sig_char(VariableType vc)
    {
        if (vc == VT_REAL) return 'r';
        else if (vc == VT_STRING) return 's';
        else throw MiscError("Variable type for external call must be either string or real.");
    }
    
    external_id_t get_next_id()
    {
        external_id_t i = 0;
        while (g_dlls.find(i) != g_dlls.end())
        {
            ++i;
        }
        ogm_assert(g_dlls.find(i) == g_dlls.end());
        return i;
    }
}