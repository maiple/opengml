#pragma once

#include "ogm/common/types.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Variable.hpp"

#include <string>
#include <vector>
#include <map>

namespace ogm::interpreter::ffi
{
    typedef uint32_t external_id_t;
    
    enum class CallType
    {
        _CDECL,
        _STDCALL
    };
    
    struct ExternalDefinition
    {
        #if defined(_WIN32)
        HINSTANCE m_dl;
        FARPROC m_dll_fn_address;
        #else
        void* m_dl;
        void* m_dll_fn_address;
        #endif
        std::string m_dl_path;
        std::string m_function_name;
        CallType m_ct;
        
        // signature comprises a string of r,s repeated
        //
        std::string m_sig;

        #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
        bool m_zugbruecke = false;
        #endif
    };
    
    extern std::map<external_id_t, ExternalDefinition> g_dlls;
    extern std::map<std::string, void*> g_path_to_dll;
    extern std::map<void*, size_t> g_dll_refc;

    external_id_t get_next_id();
    
    void external_list_impl(std::vector<std::string>& outNames, const std::string& path);
    external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt);
    void external_free_impl(external_id_t id);
    
    #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
    bool zugbruecke_init();
    external_id_t external_define_zugbruecke_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt);
    void external_call_dispatch_zugbruecke(Variable& out, std::string sig, void* fn, byte argc, const Variable* argv, std::string function_name="");
    void zugbruecke_free_base(ExternalDefinition& ed);
    void zugbruecke_free_addr(ExternalDefinition& ed);
    #endif
    
    char sig_char(VariableType vc);
}