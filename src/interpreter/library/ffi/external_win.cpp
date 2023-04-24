
#if defined(_WIN32)

#include "external.h"
#include "shltype.h"

    #ifdef OGM_WIN32_NT_FIX
        // this seems to be necessary to use the SetDllDirectoryA function.
        #ifdef _WIN32_WINNT
            #undef _WIN32_WINNT
        #endif
        #define _WIN32_WINNT 0x0502
        #include <winbase.h>
        // include moved here otherwise we get ambiguous function call errors for _InterlockedExchange
        #include <windows.h>
    #else
        #include <windows.h>
        #include <winbase.h>
    #endif

namespace ogm::interpreter::ffi
{

static bool g_set_dll_directory = false;

void external_list_impl(std::vector<std::string>& outNames, const std::string& path)
{
    // TODO
}

external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    // dll lookup directory
    if (!g_set_dll_directory)
    {
        if (!SetDllDirectory(staticExecutor.m_frame.m_fs.get_included_directory().c_str()))
        {
            std::cout << "Error setting DLL search directory: "
                << GetLastError() << std::endl;
        }
        g_set_dll_directory = true;
    }

    ExternalDefinition ed;
    ed.m_ct = ct;
    ed.m_sig.push_back(sig_char(rettype));
    for (size_t i = 0; i < argc; ++i)
    {
        ed.m_sig.push_back(sig_char(argt[i]));
    }

    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        ed.m_dl = LoadLibrary(TEXT(path));
        g_path_to_dll[path] = ed.m_dl;
        g_dll_refc[ed.m_dl] = 1;
    }
    else
    {
        ed.m_dl = g_path_to_dll[path];
        ++g_dll_refc[ed.m_dl];
    }

    if (ed.m_dl)
    {
        ed.m_dl_fn_address = GetProcAddress(ed.m_dl, fnname);
        if (ed.m_dl_fn_address)
        {
            external_id_t id = get_next_id();
            g_dlls[id] = ed;
            ogm_assert(g_dlls.find(id) != g_dlls.end());
            return id;
        }
        else
        {
            throw MiscError("Error loading DLL \"" + std::string(path) + "\": dll contains no symbol \"" + fnname + "\"");
        }
    }
    else
    {
        throw MiscError("Error loading DLL \"" + std::string(path) + "\": file not found.");
    }
}

void* external_get_fn_impl(external_id_t id)
{
    ExternalDefinition& ed = g_dlls.at(id);
    return reinterpret_cast<void*>(ed.m_dl_fn_address);
}

void external_free_impl(external_id_t id)
{
    auto iter = g_dlls.find(id);
    if (iter != g_dlls.end())
    {
        if (--g_dll_refc[std::get<1>(*iter).m_dl] == 0)
        {
            FreeLibrary(std::get<1>(*iter).m_dl);
            g_path_to_dll.erase(std::get<1>(*iter).m_dl_path);
        }
        g_dlls.erase(iter);
    }
}

}

#endif