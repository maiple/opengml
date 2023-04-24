#include "libpre.h"
    #include "fn_external.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include <functional>

#ifdef PELOADER
    extern "C"
    {
    #include "fn_external_c.h"
    }
#endif

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>
#include "ffi/shltype.h"

#include <ffi.h>

using namespace ogm::interpreter;
using namespace ogm::interpreter::ffi;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::ogm_external_list(VO out, V vpath)
{
    string_t path = staticExecutor.m_frame.m_fs.resolve_file_path(vpath.castCoerce<string_t>());

    std::vector<std::string> symbols;

    path_transform(path);
    SharedLibraryType shtype = getSharedLibraryTypeFromPath(path);

    if (shtype.compatible())
    {
        external_list_impl(symbols, path);
    }

    if (symbols.empty())
    {
        out = 0.0;
        return;
    }

    // OPTIMIZE: reserve array
    out.array_ensure();
    if (out.array_height() == 0)
    {
        out.array_get(OGM_2DARRAY_DEFAULT_ROW 0) = "";
    }

    size_t i = 0;
    for (const std::string& symbol: symbols)
    {
        out.array_get(OGM_2DARRAY_DEFAULT_ROW i++) = symbol;
    }
}

void ogm::interpreter::fn::external_define(VO out, byte argc, const Variable* argv)
{
    if (argc < 5) throw MiscError("external_define requires at least 5 arguments.");

    // marshall arguments
    string_t path = staticExecutor.m_frame.m_fs.resolve_file_path(argv[0].castCoerce<string_t>());
    string_t fnname = argv[1].castCoerce<string_t>();
    CallType ct = static_cast<CallType>(argv[2].castCoerce<size_t>());
    VariableType rt = static_cast<VariableType>(argv[3].castCoerce<size_t>());
    size_t nargs = argv[4].castCoerce<size_t>();
    if (nargs + 5 != argc) throw MiscError("external_define wrong number of arguments");

    VariableType argt[32];
    ogm_assert(nargs < 32);

    // argument types
    for (size_t i = 0; i < nargs; ++i)
    {
        argt[i] =  static_cast<VariableType>(argv[5 + i].castCoerce<size_t>());
    }

    std::string orgpath = path;
    path_transform(path); // try loading .so instead of .dll if possible
    SharedLibraryType shtype = getSharedLibraryTypeFromPath(path);
    if (!shtype.compatible())
    {
        throw MiscError("Unable to load incompatible dll: " + path);
    }
    
    if (shtype.platmatch())
    {
        out = static_cast<real_t>(external_define_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt));
    }
    #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
    else if (shtype.os == SharedLibraryType::WINDOWS && zugbruecke_init())
    {
        out = static_cast<real_t>(external_define_zugbruecke_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt));
    }
    #endif
    else
    {
        throw MiscError("Shared library " + path + " compatible but no handler found.");
    }
}

namespace
{
    template<typename T, typename K>
    void helper(VO out, void(*f)(ffi_cif*, K, void*, void**), ffi_cif* cif, void* addr, void** values)
    {
        T rv;
        f(cif, (K)addr, &rv, values);
        out.set(rv);
    }
    
    void external_call_addr(VO out, void* addr, CallType ct, const char* argt, byte argc, const Variable* argv)
    {
        // TODO
        // OPTIMIZE: maybe this can all be done without malloc/new!
        if (argc != strlen(argt) + 1)
        {
            throw MiscError("incorrect number of arguments for external call.");
        }
        
        ffi_type** argtypes = new ffi_type*[argc+1];
        ogm_defer(delete [] argtypes);
        void** values = new void*[argc];
        ogm_defer({
            for (size_t i = 0; i < argc; ++i)
            {
                if (argt[i+1] == 's')
                {
                    free(*(char**)values[i]);
                }
                free(values[i]);
            }
            delete [] values;
        });
        
        ffi_cif cif;
        
        for (size_t i = 0; i < argc+1; ++i)
        {
            argtypes[i] = (argt[i] == 's') ? &ffi_type_pointer : &ffi_type_double;
            
            // non-return-args
            if (i > 0)
            {
                if (argt[i] == 's')
                {
                    // TODO: don't copy-allocate the string if it can be avoided.
                    std::string s {argv[i].string_view()};
                    char** nv = alloc<char*>();
                    *nv = _strdup(s.c_str());
                    values[i-1] = nv;
                }
                else
                {
                    double* d = alloc<double>();
                    *d = argv[i].castCoerce<real_t>();
                    values[i-1] = d;
                }
            }
        }
        
        #ifdef _WIN32
        ffi_abi abi = (ct == CallType::_STDCALL) ? FFI_STDCALL : FFI_DEFAULT_ABI;
        #else
        ffi_abi abi = FFI_DEFAULT_ABI;
        #endif
        
        if (ffi_prep_cif(&cif, abi, argc, argtypes[0], argtypes+1) == FFI_OK)
        {
            if (argt[0] == 's')
            {
                helper<char*>(out, ffi_call, &cif, addr, values);
            }
            else
            {
                helper<real_t>(out, ffi_call, &cif, addr, values);
            }
        }
        else
        {
            throw MiscError("Unable to prepare FFI callsite");
        }
    }
    
    void external_call(VO out, external_id_t id, byte argc, const Variable* argv)
    {
        if (g_dlls.find(id) != g_dlls.end())
        {
            ExternalDefinition& ed = g_dlls.at(id);
            try
            {
                #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
                if (ed.m_zugbruecke)
                {
                    external_call_dispatch_zugbruecke(out, ed.m_sig, ed.m_dll_fn_address, argc, argv, ed.m_function_name);
                }
                else
                #endif
                {
                    external_call_addr(out, ed.m_dll_fn_address, ed.m_ct, ed.m_sig.c_str(), argc, argv);
                }
            }
            catch (std::exception& e)
            {
                throw MiscError("Error invoking \"" + ed.m_function_name + "\" in \"" + ed.m_dl_path + "\":\n" + e.what());
            }
        }
        else
        {
            throw MiscError("external call id not recognized.");
        }
    }
    
    struct callargs
    {
        int32_t m_canary;
        Variable* m_out;
        external_id_t m_id;
        int m_argc;
        const Variable* m_argv;
    };

    #define K_CANARY 0x43054546

    #ifdef _WIN32
    DWORD WINAPI
    #else
    int32_t
    #endif
    call(void* pc)
    {
        callargs& c = *(callargs*)pc;
        ogm_assert(c.m_canary == K_CANARY);
        external_call(*c.m_out, c.m_id, c.m_argc, c.m_argv);
        return 0;
    }
}

void ogm::interpreter::fn::external_call(VO out, byte argc, const Variable* argv)
{
    if (argc == 0) throw MiscError("external_call requires id argument.");

    callargs args{
        K_CANARY,
        &out,
        argv[0].castCoerce<external_id_t>(),
        argc - 1,
        argv + 1
    };

    (void)call(&args);
}

void ogm::interpreter::fn::external_free(VO out, V id)
{
    external_free_impl(id.castCoerce<external_id_t>());
}
