#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

namespace
{

enum class CallType
{
    cdecl,
    stdcall
};

typedef uint32_t external_id_t;

char sig_char(VariableType vc)
{
    if (vc == VT_REAL) return 'r';
    else if (vc == VT_STRING) return 's';
    else throw MiscError("Variable type for external call must be either string or real.");
}

#define IFSIG(a) if (sig == #a)
#define DEFN_external_call(calltype, fntype) \
void external_call_dispatch##calltype(VO out, std::string sig, fntype fn, byte argc, const Variable* argv) \
{ \
    IFSIG(r) \
    { \
        out = ((real_t (calltype *)())(fn))(); \
        return; \
    } \
    else IFSIG(s) \
    { \
        out = ((const char* (calltype *)())(fn))(); \
        return; \
    } \
    else IFSIG(rr) \
    { \
        out = ((real_t (calltype *)(real_t))(fn))( \
            argv[0].castCoerce<real_t>() \
        ); \
        return; \
    } \
    \
    throw MiscError("Invalid call signature for external call."); \
} // TODO: finish remaining signatures (str/real up to argc3, real up to 10).

// forward declarations
external_id_t external_define_impl(const char* path, const char* fnname, CallType, VariableType rettype, uint32_t argc, VariableType* argtype);
void external_call_impl(VO out, external_id_t, byte argc,  const Variable* argv);
void external_free_impl(external_id_t);

#ifdef __unix__
#define RESOLVED

#include <dlfcn.h>

DEFN_external_call(, void*);

struct ExternalDefinitionDL
{
    void* m_dl;
    std::string m_dl_path;
    void* m_dll_fn_address;
    CallType m_ct;
    std::string m_sig;
};

std::map<external_id_t, ExternalDefinitionDL> g_dlls;
std::map<std::string, void*> g_path_to_dll;
std::map<void*, size_t> g_dll_refc;

inline external_id_t get_next_id()
{
    external_id_t i = 0;
    while (g_dlls.find(i++) != g_dlls.end());
    return i;
}

external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, VariableType* argt)
{
    ExternalDefinitionDL ed;
    ed.m_ct = ct; 
    ed.m_sig.push_back(rettype);
    ed.m_dl_path = path;
    for (size_t i = 0; i < argc; ++i)
    {
        ed.m_sig.push_back(sig_char(argt[i]));
    }
    
    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        ed.m_dl = dlopen(path, RTLD_LAZY);
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
        ed.m_dll_fn_address = dlsym(ed.m_dl, fnname);
        if (ed.m_dll_fn_address)
        {
            external_id_t id = get_next_id();
            g_dlls[id] = ed;
            return id;
        }
        else
        {
            throw MiscError("Error loading library \"" + std::string(path) + "\": library contains no symbol \"" + fnname + "\" found");
        }
    }
    else
    {
        throw MiscError("Error loading library \"" + std::string(path) + "\": file not found.");
    }
}

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    ExternalDefinitionDL& ed = g_dlls.at(id);
    external_call_dispatch(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
    throw MiscError("Invalid call signature for external call.");
}

void external_free_impl(external_id_t id)
{
    auto iter = g_dlls.find(id);
    if (iter != g_dlls.end())
    {
        if (--g_dll_refc[std::get<1>(*iter).m_dl] == 0)
        {
            dlclose(std::get<1>(*iter).m_dl);
            g_path_to_dll.erase(std::get<1>(*iter).m_dl_path);
        }
        g_dlls.erase(iter);
    }
}

#endif

#if defined(_WIN32) || defined(WIN32)
#define RESOLVED

#include <windows.h>

struct ExternalDefinitionWin32
{
    HINSTANCE m_dll;
    std::string m_dll_path;
    FARPROC m_dll_fn_address;
    CallType m_ct;
    std::string m_sig;
};
std::map<external_id_t, ExternalDefinitionWin32> g_dlls;
std::map<std::string, HINSTANCE> g_path_to_dll;
std::map<HINSTANCE, size_t> g_dll_refc;

inline external_id_t get_next_id()
{
    external_id_t i = 0;
    while (g_dlls.find(i++) != g_dlls.end());
    return i;
}

external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, VariableType* argt)
{
    ExternalDefinitionWin32 ed;
    ed.m_ct = ct; 
    ed.m_sig.push_back(rettype);
    ed.m_dll_path = path;
    for (size_t i = 0; i < argc; ++i)
    {
        ed.m_sig.push_back(sig_char(argt[i]));
    }
    
    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        ed.m_dll = LoadLibrary(TEXT(path));
        g_path_to_dll[path] = ed.m_dll;
        g_dll_refc[ed.m_dll] = 1;
    }
    else
    {
        ed.m_dll = g_path_to_dll[path];
        ++g_dll_refc[ed.m_dll];
    }
    
    if (ed.m_dll)
    {
        ed.m_dll_fn_address = GetProcAddress(ed.m_dll, fnname);
        if (ed.m_dll_fn_address)
        {
            external_id_t id = get_next_id();
            g_dlls[id] = ed;
            return id;
        }
        else
        {
            throw MiscError("Error loading DLL \"" + std::string(path) + "\": dll contains no symbol \"" + fnname + "\" found");
        }
    }
    else
    {
        throw MiscError("Error loading DLL \"" + std::string(path) + "\": file not found.");
    }
}

DEFN_external_call(__cdecl, FARPROC);
DEFN_external_call(__stdcall, FARPROC);

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    ExternalDefinitionWin32& ed = g_dlls.at(id);
    switch (ed.m_ct)
    {
    case CallType::stdcall:
        {
            external_call_dispatch__stdcall(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
        }
        break;
    case CallType::cdecl:
        {
            external_call_dispatch__cdecl(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
        }
        break;
    }
    
    throw MiscError("Invalid call signature for external call.");
}

void external_free_impl(external_id_t id)
{
    auto iter = g_dlls.find(id);
    if (iter != g_dlls.end())
    {
        if (--g_dll_refc[std::get<1>(*iter).m_dll] == 0)
        {
            FreeLibrary(std::get<1>(*iter).m_dll);
            g_path_to_dll.erase(std::get<1>(*iter).m_dll_path);
        }
        g_dlls.erase(iter);
    }
}

#endif

#ifndef RESOLVED
// default implementation.
external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, VariableType* agrt)
{
    std::cout << "WARNING: external loading not supported on this platform."
    return 0;
}

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    out = 0;
}

void external_free_impl(external_id_t id)
{ }

#endif

}

void ogmi::fn::external_define(VO out, byte argc, const Variable* argv)
{
    if (argc < 5) throw MiscError("external_define requires at least 5 arguments.");
    
    // marshall arguments
    string_t path = argv[0].castCoerce<string_t>();
    string_t fnname = argv[1].castCoerce<string_t>();
    CallType ct = static_cast<CallType>(argv[2].castCoerce<size_t>());
    VariableType rt = static_cast<VariableType>(argv[3].castCoerce<size_t>());
    size_t nargs = argv[4].castCoerce<size_t>();
    if (nargs != argc + 5) throw MiscError("external_define wrong number of arguments");
    
    VariableType argt[32];
    assert(nargs < 32);
    
    // argument types
    for (size_t i = 0; i < nargs; ++i)
    {
        argt[i] =  static_cast<VariableType>(argv[5 + i].castCoerce<size_t>());
    }
    
    out = external_define_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt);
}

void ogmi::fn::external_call(VO out, byte argc, const Variable* argv)
{
    if (argc == 0) throw MiscError("external_call requires id argument.");
    external_call_impl(out, argv[0].castCoerce<external_id_t>(), argc, argv + 1);
}

void ogmi::fn::external_free(VO out, V id)
{
    external_free_impl(id.castCoerce<external_id_t>());
}