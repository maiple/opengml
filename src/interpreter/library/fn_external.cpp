#include "libpre.h"
    #include "fn_external.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#ifdef PELOADER
    extern "C"
    {
    #include "fn_external_c.h"
    }
#endif

#ifdef EMBED_ZUGBRUECKE
    #include <Python.h>
    #include "ogm/interpreter/Debugger.hpp"
    #include <csignal>
#endif

#if defined(_WIN32) || defined(WIN32)
    #ifndef _WIN32
        #define _WIN32
    #endif
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
#endif

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

namespace
{

enum class CallType
{
    _CDECL,
    _STDCALL
};

typedef uint32_t external_id_t;

char sig_char(VariableType vc)
{
    if (vc == VT_REAL) return 'r';
    else if (vc == VT_STRING) return 's';
    else throw MiscError("Variable type for external call must be either string or real.");
}

#include "fn_external_call_def.h"

// forward declarations
external_id_t external_define_impl(const char* path, const char* fnname, CallType, VariableType rettype, uint32_t argc, const VariableType* argtype);
void external_call_impl(VO out, external_id_t, byte argc,  const Variable* argv);
void external_free_impl(external_id_t);
void external_list_impl(std::vector<std::string>& outNames, const std::string& path);

#if defined(__unix__) || defined(__APPLE__)
#define RESOLVED

#include <dlfcn.h>
#ifdef __unix__
    #include <link.h>
    #include <elf.h>
#endif

DEFN_external_call(, void*)

struct ExternalDefinitionDL
{
    void* m_dl;
    std::string m_dl_path;
    std::string m_function_name;
    void* m_dll_fn_address;
    CallType m_ct;
    std::string m_sig;

    #ifdef EMBED_ZUGBRUECKE
    bool m_zugbruecke = false;
    #endif
};

std::map<external_id_t, ExternalDefinitionDL> g_dlls;
std::map<std::string, void*> g_path_to_dll;
std::map<void*, size_t> g_dll_refc;

inline external_id_t get_next_id()
{
    external_id_t i = 0;
    while (g_dlls.find(i) != g_dlls.end())
    {
        ++i;
    }
    ogm_assert(g_dlls.find(i) == g_dlls.end());
    return i;
}

#ifdef EMBED_ZUGBRUECKE
bool g_zugbruecke_setup_complete = false;
bool g_zugbruecke_available;

// zugbruecke module
PyObject* g_zugbruecke = nullptr;
PyObject* g_zugbruecke_cdecl = nullptr;
PyObject* g_zugbruecke_stdcall = nullptr;

void python_add_to_env_path(const std::string& path);

// zugbruecke can steal sigint, so we reattach it.
void reattach_sigint()
{
    if (staticExecutor.m_debugger)
    {
        staticExecutor.m_debugger->on_attach();
    }
    else
    {
        std::signal(SIGINT, SIG_DFL);
    }
}

bool zugbruecke_init()
{
    if (g_zugbruecke_setup_complete)
    {
        return g_zugbruecke_available;
    }

    std::cout << "Initializing zugbruecke (for win32 DLL)..." << std::endl;

    g_zugbruecke_setup_complete = true;
    g_zugbruecke_available = false;

    // setup python
    Py_Initialize();

    // TODO: should be cleaned up witha call to Py_Finalize().

    PyObject *pName;

    pName = PyUnicode_DecodeFSDefault("zugbruecke");

    g_zugbruecke = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!g_zugbruecke)
    {
        std::cerr << "Failed to load module zugbruecke for running win32 dll.\n";
        std::cerr << "Try running `python3 -m pip install zugbruecke`.\n";
        return false;
    }

    PyObject* cdll = PyObject_GetAttrString(g_zugbruecke, "cdll");

    // FIXME: this actually seems like it's apparently the correct behaviour???
    PyObject* windll = PyObject_GetAttrString(g_zugbruecke, "cdll");

    if (cdll)
    {
        g_zugbruecke_cdecl = PyObject_GetAttrString(cdll, "LoadLibrary");
        Py_DECREF(cdll);
    }

    if (windll)
    {
        g_zugbruecke_stdcall = PyObject_GetAttrString(windll, "LoadLibrary");
        Py_DECREF(windll);
    }

    // load libraries from external datafiles location.
    // FIXME: uncomment this when bugs are fixed.
    // python_add_to_env_path(staticExecutor.m_frame.m_fs.m_included_directory.c_str());

    reattach_sigint();

    g_zugbruecke_available = true;

    std::cout << "...zugbruecke initialized." << std::endl;

    return true;
}

std::string _str_pyobject(PyObject* o)
{
    PyObject* repr = PyObject_Repr(o);
    PyObject* str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");;
    std::string s = PyBytes_AS_STRING(str);

    Py_XDECREF(repr);
    Py_XDECREF(str);

    return s;
}

void check_python_error(const std::string& error_prefix, bool fatal = true)
{
    if (PyErr_Occurred())
    {
        PyObject* ptype;
        PyObject* pvalue;
        PyObject* ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);

        ogm_assert(ptype && pvalue);

        if (fatal)
        {
            throw MiscError(
                error_prefix + ":\n"
                + "type: " + _str_pyobject(ptype) + "\n"
                + "value: " + _str_pyobject(pvalue)
            );
        }
        else
        {
            std::cout << error_prefix + ":\n"
                + "type: " + _str_pyobject(ptype) + "\n"
                + "value: " + _str_pyobject(pvalue) + "\n";

            Py_DECREF(ptype);
            Py_DECREF(pvalue);
            Py_DECREF(ptrace);
        }
    }
}

// forward declaration.
PyObject* load_library_zugbruecke(const char* path, CallType ct);
PyObject* load_function_zugbruecke(PyObject* library, const char* fnname, VariableType rettype,  uint32_t argc, const VariableType* argt);

namespace
{
    // is kernel32.dll a library that has been automatically loaded?
    bool g_loaded_kernel32 = false;

    PyObject* g_kernel32;
    PyObject* g_kernel32_SetDllPath;
}

// adds a path to the environment path.
// FIXME this doesn't seem to actually work, even though SetDllDirectoryA is called.
// Dubious, but it could be a path separator problem.
void python_add_to_env_path(const std::string& path)
{
    // zugbruecke.cdecl.SetDllDirectory()
    if (!g_loaded_kernel32)
    {
        g_loaded_kernel32 = true;

        g_kernel32 = load_library_zugbruecke("kernel32.dll", CallType::_STDCALL);

        if (!g_kernel32)
        {
            throw MiscError("(Zugbruecke) failed to load kernel32.dll (required for SetDllDirectory)");
        }

        const VariableType argt = VT_STRING;

        g_kernel32_SetDllPath = load_function_zugbruecke(g_kernel32, "SetDllDirectoryA", VT_BOOL, 1, &argt);

        if (!g_kernel32_SetDllPath)
        {
            throw MiscError("(Zugbruecke) failed to find SetDllDirectory in kernel32.dll.");
        }

        // construct args
        PyObject* arguments = PyTuple_New(1);
        PyObject* arg = PyUnicode_DecodeFSDefault(path.c_str());
        PyTuple_SetItem(arguments, 0, arg);
        // Py_DECREF(arg) //FIXME seems to cause an error

        if (!PyCallable_Check(g_kernel32_SetDllPath))
        {
            throw MiscError("(Zugbruecke) external function not callable. (This is an internal bug.)");
        }

        PyObject* retval = PyObject_CallObject(g_kernel32_SetDllPath, arguments);

        if (retval)
        {
            if (retval == Py_False)
            {
                throw MiscError("SetDllDirectory returned false (error).");
            }
            Py_DECREF(retval);
        }
        else
        {
            // FIXME: this is being thrown actually.
            throw MiscError("No return value for SetDllDirectoryA.");
        }

        Py_DECREF(arguments);
    }
}

PyObject* load_function_zugbruecke(PyObject* library, const char* fnname, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    PyObject* fndl = PyObject_GetAttrString(library, fnname);

    check_python_error("(Zugbruecke) Error locating symbol \"" + std::string(fnname) + "\"in external library.");

    if (!fndl)
    {
        return nullptr;
    }

    PyObject* c_bool = PyObject_GetAttrString(g_zugbruecke, "c_bool");
    PyObject* c_real = PyObject_GetAttrString(g_zugbruecke, "c_double");
    PyObject* c_string = PyObject_GetAttrString(g_zugbruecke, "c_char_p");

    PyObject* restype;
    switch(rettype)
    {
    case VT_REAL:
        restype = c_real;
        break;
    case VT_STRING:
        restype = c_string;
        break;
    case VT_BOOL:
        // only used internally
        restype = c_bool;
        break;
    default:
        throw MiscError("Invalid type");
    }

    PyObject* argtypes = PyList_New(argc);
    for (size_t i = 0; i < argc; ++i)
    {
        PyList_SetItem(argtypes, i,
            sig_char(argt[i]) == 'r'
            ? c_real
            : c_string
        );
    }

    // set return and argument types for function.
    PyObject_SetAttrString(fndl, "argtypes", argtypes);
    PyObject_SetAttrString(fndl, "restype", restype);

    Py_DECREF(c_real);
    Py_DECREF(c_string);
    Py_DECREF(c_bool);
    Py_DECREF(argtypes);
    Py_DECREF(restype);

    return fndl;
}

PyObject* load_library_zugbruecke(const char* path, CallType ct)
{
    PyObject* calltype = (ct == CallType::_CDECL)
        ? g_zugbruecke_cdecl
        : g_zugbruecke_stdcall;

    if (!calltype)
    {
        throw MiscError("zugbreucke missing support for call type " + std::string
            (
                (ct == CallType::_CDECL)
                ? "cdecl (zugbruecke.cdll.LoadLibrary)"
                : "stdcall (zugbruecke.windll.LoadLibrary)"
            )
        );
    }

    PyObject* py_path = PyUnicode_DecodeFSDefault(path);
    PyObject* py_tuple = PyTuple_New(1);

    ogm_defer(Py_DECREF(py_path));
    ogm_defer(Py_DECREF(py_tuple));

    PyTuple_SetItem(py_tuple, 0, py_path);

    check_python_error("(Zugbruecke) error occured prior to loading library \"" + std::string(path) + "\".");

    PyObject* dl = PyObject_CallObject(calltype, py_tuple);

    check_python_error("(Zugbruecke) error occured while loading library \"" + std::string(path) + "\".");

    return dl;
}

external_id_t external_define_zugbruecke_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    ExternalDefinitionDL ed;
    ed.m_ct = ct;
    ed.m_sig.push_back(sig_char(rettype));
    ed.m_dl_path = path;
    ed.m_function_name = fnname;
    ed.m_dl = nullptr;
    ed.m_zugbruecke = true;
    for (size_t i = 0; i < argc; ++i)
    {
        ed.m_sig.push_back(sig_char(argt[i]));
    }

    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        ed.m_dl = load_library_zugbruecke(path, ct);

        if (!ed.m_dl)
        {
            throw MiscError("(Zugbruecke) Failed to load library \"" + std::string(path) + "\"");
        }
        g_path_to_dll[path] = ed.m_dl;
        g_dll_refc[ed.m_dl] = 1;
    }
    else
    {
        ed.m_dl = g_path_to_dll[path];
        ++g_dll_refc[ed.m_dl];
    }

    PyObject* dl = static_cast<PyObject*>(ed.m_dl);
    ed.m_dll_fn_address = load_function_zugbruecke(dl, fnname, rettype, argc, argt);

    if (!ed.m_dll_fn_address)
    {
        throw MiscError("(Zugbruecke) failed to find symbol \"" + std::string(fnname) +
            "\" in library \"" + path + "\".");
    }

    // reattach sigint interrupt if Python stole it.
    reattach_sigint();

    external_id_t id = get_next_id();
    g_dlls[id] = ed;
    return id;
}

// function name: optional, provided for error message context.
void external_call_dispatch_zugbruecke(VO out, std::string sig, void* fn, byte argc, const Variable* argv, std::string function_name="")
{
    PyObject* _fn = static_cast<PyObject*>(fn);

    // construct args
    PyObject* arguments = PyTuple_New(argc);
    for (size_t i = 0; i < argc; ++i)
    {
        PyObject* arg;
        if (sig[i + 1] == 's')
        {
            PyObject* str = PyUnicode_DecodeFSDefault(argv[i].castCoerce<std::string>().c_str());
            arg = PyUnicode_AsUTF8String(str);
            Py_DECREF(str);
        }
        else if (sig[i + 1] == 'r')
        {
            arg = PyFloat_FromDouble(argv[i].castCoerce<real_t>());
        }
        else
        {
            throw MiscError("Can only pass strings or reals to external calls.");
        }

        PyTuple_SetItem(arguments, i, arg);
        //Py_DECREF(arg); //FIXME seems to cause an error
    }

    if (!PyCallable_Check(_fn))
    {
        throw MiscError("(Zugbruecke) external function not callable. (This is an internal bug.)");
    }

    check_python_error("(Zugbruecke) Error occurred prior to invoking external function \"" + function_name + "\"");

    PyObject* retval = PyObject_CallObject(_fn, arguments);

    // TODO: make this check fatal.
    check_python_error("(Zugbruecke) Error occurred while invoking external function \""  + function_name + "\"", false);

    if (retval)
    {
        if (sig[0] == 'r')
        // return value is a real
        {
            out = PyFloat_AsDouble(retval);
        }
        else
        {
            out = std::string(PyBytes_AS_STRING(retval));
        }

        Py_DECREF(retval);
    }
    else
    {
        // should only occur if above error check is commented out.
        out = 0;
    }

    Py_DECREF(arguments);

    // reattach sigint interrupt if Python stole it.
    reattach_sigint();
}
#endif

#ifdef PELOADER
external_id_t external_define_peloader_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    ExternalDefinitionDL ed;
    ed.m_ct = ct;
    ed.m_sig.push_back(sig_char(rettype));
    ed.m_dl_path = path;
    ed.m_function_name = fnname;
    ed.m_dl = nullptr;
    for (size_t i = 0; i < argc; ++i)
    {
        ed.m_sig.push_back(sig_char(argt[i]));
    }

    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        if (_c_load_library(path))
        {
            throw MiscError("(PeLoader) Error loading DLL library.");
        }

        g_path_to_dll[path] = ed.m_dl;
        g_dll_refc[ed.m_dl] = 1;

    }
    else
    {
        ed.m_dl = g_path_to_dll[path];
        ++g_dll_refc[ed.m_dl];
    }

    if (_c_get_export(fnname, &ed.m_dll_fn_address) == -1)
    {
        throw MiscError("unable to find symbol \"" + std::string(fnname) + "\".");
    }

    external_id_t id = get_next_id();
    g_dlls[id] = ed;
    return id;
}
#endif

external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    dlerror();
    ExternalDefinitionDL ed;
    ed.m_ct = ct;
    ed.m_sig.push_back(sig_char(rettype));
    ed.m_dl_path = path;
    ed.m_function_name = fnname;
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
            throw MiscError("Error loading library \"" + std::string(path) + "\" symbol \"" + fnname + "\": " + dlerror());
        }
    }
    else
    {
        throw MiscError("Error loading library \"" + std::string(path) + "\": " + dlerror());
    }
}

void external_list_impl(std::vector<std::string>& outNames, const std::string& path)
{
    #ifdef __unix__
    dlerror();
    void* dl;
    bool close = false;
    if (g_path_to_dll.find(path) == g_path_to_dll.end())
    {
        dl = dlopen(path.c_str(), RTLD_LAZY);
        close = true;
    }
    else
    {
        dl = g_path_to_dll[path];
    }

    // on exit, close the dll if we opened it.
    ogm_defer(
        ([close, dl]{
            if (close) dlclose(dl);
        })
    );

    if (dl)
    {
        struct Data {
            std::vector<std::string>& oNames;
            const std::string& path;
        } data { outNames, path };

        dl_iterate_phdr(
            [](dl_phdr_info *info, size_t size, void* vdata) -> int
            {
                Data& data = *static_cast<Data*>(vdata);
                std::vector<std::string>& outNames = data.oNames;

                if (info->dlpi_name == data.path)
                // this ELF header matches the library we want.
                {
                    for (size_t i = 0; i < info->dlpi_phnum; ++i)
                    {
                        const ElfW(Phdr)* phdr = info->dlpi_phdr + i;
                        if (phdr->p_type == PT_DYNAMIC)
                        // we've found the dynamic section of the ELF.
                        {
                            char* strtab = nullptr;
                            ElfW(Word) sym_c = 0;
                            ElfW(Word*) hash = nullptr;

                            // iterate over dynamic table entries
                            // see https://stackoverflow.com/a/16897138
                            for (
                                ElfW(Dyn*) dyn = reinterpret_cast<ElfW(Dyn)*>(info->dlpi_addr +  phdr->p_vaddr);
                                dyn->d_tag != DT_NULL;
                                ++dyn
                            )
                            {
                                if (dyn->d_tag == DT_HASH)
                                {
                                    /* Get a pointer to the hash */
                                    hash = (ElfW(Word*))dyn->d_un.d_ptr;

                                    /* The second word is the number of symbols */
                                    sym_c = hash[1];
                                }
                                if (dyn->d_tag == DT_GNU_HASH)
                                {
                                    // TODO
                                }
                                else if (dyn->d_tag == DT_STRTAB)
                                {
                                    /* Get the pointer to the string table */
                                    strtab = (char*)dyn->d_un.d_ptr;
                                }
                                else if (dyn->d_tag == DT_SYMTAB)
                                {
                                    // entry order is messed up. Fail.
                                    if (!strtab) return -1;

                                    /* Get the pointer to the first entry of the symbol table */
                                    ElfW(Sym*) sym = (ElfW(Sym*))dyn->d_un.d_ptr;

                                    /* Iterate over the symbol table */
                                    for (ElfW(Word) sym_index = 0; sym_index < sym_c; sym_index++)
                                    {
                                        /* get the name of the i-th symbol.
                                         * This is located at the address of st_name
                                         * relative to the beginning of the string table. */
                                        const char* sym_name = &strtab[sym[sym_index].st_name];

                                        outNames.emplace_back(sym_name);
                                    }
                                }
                            }

                            // stop iteration
                            return 1;
                        }
                    }
                }

                return 0; // continue;
            },
            &data
        );
    }
    else
    {
        throw MiscError("Error loading library \"" + std::string(path) + "\": " + dlerror());
    }
    #else
    // TODO: __APPLE__
    #endif
}

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    if (g_dlls.find(id) != g_dlls.end())
    {
        ExternalDefinitionDL& ed = g_dlls.at(id);
        try
        {
            #ifdef EMBED_ZUGBRUECKE
            if (ed.m_zugbruecke)
            {
                external_call_dispatch_zugbruecke(out, ed.m_sig, ed.m_dll_fn_address, argc, argv, ed.m_function_name);
                return;
            }
            #endif
            external_call_dispatch(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
            return;
        }
        catch (std::exception& e)
        {
            throw MiscError("Error invoking \"" + ed.m_function_name + "\" in \"" + ed.m_dl_path + "\":\n" + e.what());
        }
    }

    throw MiscError("external call id not recognized.");
}

void external_free_impl(external_id_t id)
{
    auto iter = g_dlls.find(id);
    if (iter != g_dlls.end())
    {
        if (std::get<1>(*iter).m_dl)
        {
            if (--g_dll_refc[std::get<1>(*iter).m_dl] == 0)
            {
                #ifdef EMBED_ZUGBRUECKE
                if (std::get<1>(*iter).m_zugbruecke)
                {
                    PyObject* dl = static_cast<PyObject*>(std::get<1>(*iter).m_dl);
                    Py_DECREF(dl);
                }
                else
                #endif
                {
                    dlclose(std::get<1>(*iter).m_dl);
                }
                g_path_to_dll.erase(std::get<1>(*iter).m_dl_path);
            }
            g_dlls.erase(iter);
        }

        #ifdef EMBED_ZUGBRUECKE
        if (std::get<1>(*iter).m_zugbruecke)
        {
            PyObject* fndl = static_cast<PyObject*>(std::get<1>(*iter).m_dll_fn_address);
            Py_DECREF(fndl);
        }
        #endif
    }
}

#endif

#if defined(_WIN32)
#define RESOLVED

namespace
{
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
    bool g_set_dll_directory = false;
}

inline external_id_t get_next_id()
{
    external_id_t i = 0;
    while (g_dlls.find(i) != g_dlls.end())
    {
        ++i;
    }
    ogm_assert(g_dlls.find(i) == g_dlls.end());
    return i;
}

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

    ExternalDefinitionWin32 ed;
    ed.m_ct = ct;
    ed.m_sig.push_back(sig_char(rettype));
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

DEFN_external_call(__cdecl, FARPROC)
DEFN_external_call(__stdcall, FARPROC)

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    ExternalDefinitionWin32& ed = g_dlls.at(id);
    switch (ed.m_ct)
    {
    case CallType::_STDCALL:
        {
            external_call_dispatch__stdcall(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
        }
        break;
    case CallType::_CDECL:
        {
            external_call_dispatch__cdecl(out, ed.m_sig, ed.m_dll_fn_address, argc, argv);
        }
        break;
    default:
        throw MiscError("Invalid call type, expected stdcall or cdecl");
    }
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
external_id_t external_define_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* agrt)
{
    std::cout << "WARNING: external loading not supported on this platform.";
    return 0;
}

void external_list_impl(std::vector<std::string>& outNames, const std::string& path)
{ }

void external_call_impl(VO out, external_id_t id, byte argc,  const Variable* argv)
{
    out = static_cast<real_t>(0);
}

void external_free_impl(external_id_t id)
{ }

#endif

}

#ifdef __unix__
    #define DLL_REPLACEMENT_EXTENSION ".so"
#elif defined(__APPLE__)
    #define DLL_REPLACEMENT_EXTENSION ".dylib"
#endif

namespace
{
    // platform-dependent path transformation
    void path_transform(std::string& path)
    {
        #ifdef DLL_REPLACEMENT_EXTENSION
        // swap .dll out for .so if one is available.
        if (ends_with(path, ".dll"))
        {
            std::string pathnodll = remove_suffix(path, ".dll");
            if (staticExecutor.m_frame.m_fs.file_exists(pathnodll + DLL_REPLACEMENT_EXTENSION))
            {
                path = pathnodll + DLL_REPLACEMENT_EXTENSION;
            }
        }
        // try .so.64 if needed
        if (is_64_bit() && ends_with(path, DLL_REPLACEMENT_EXTENSION))
        {
            if (staticExecutor.m_frame.m_fs.file_exists(path + ".64"))
            {
                path += ".64";
            }
        }
        // try .so.32 if needed
        if (is_32_bit() && ends_with(path, DLL_REPLACEMENT_EXTENSION))
        {
            if (staticExecutor.m_frame.m_fs.file_exists(path + ".32"))
            {
                path += ".32";
            }
        }
        #endif
    }
}

void ogm::interpreter::fn::ogm_external_list(VO out, V vpath)
{
    string_t path = staticExecutor.m_frame.m_fs.resolve_file_path(vpath.castCoerce<string_t>());

    std::vector<std::string> symbols;

    path_transform(path);

    #ifdef EMBED_ZUGBRUECKE
    if (ends_with(path, ".dll"))
    {
        if (zugbruecke_init())
        {
            // TODO.
            goto symbols_to_array;
        }
    }
    #endif
    #ifdef PELOADER
    if (ends_with(path, ".dll"))
    {
        // TODO.
        goto symbols_to_array;
    }
    #endif

    external_list_impl(symbols, path);

symbols_to_array:

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

    path_transform(path);

    #ifdef EMBED_ZUGBRUECKE
    if (ends_with(path, ".dll"))
    {
        if (zugbruecke_init())
        {
            out = static_cast<real_t>(external_define_zugbruecke_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt));
            return;
        }
    }
    #endif
    #ifdef PELOADER
    if (ends_with(path, ".dll"))
    {
        out = static_cast<real_t>(external_define_peloader_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt));
        return;
    }
    #endif

    out = static_cast<real_t>(external_define_impl(path.c_str(), fnname.c_str(), ct, rt, nargs, argt));
}

namespace
{
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
        external_call_impl(*c.m_out, c.m_id, c.m_argc, c.m_argv);
        return 0;
    }

    bool g_call_separate_thread = false;
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

    if (g_call_separate_thread)
    {
        callargs* args_copy = (callargs*)malloc(sizeof(callargs));
        memcpy(args_copy, &args, sizeof(callargs));

        #ifdef _WIN32
        DWORD thread_id;
        DWORD status;
        HANDLE thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            call,                   // function
            args_copy,              // argument to function
            0,                      // use default creation flags
            &thread_id);

        if (thread == NULL)
        {
            goto error;
        }

        status = WaitForSingleObject(thread, INFINITE);

        CloseHandle(thread);

        if (status != WAIT_OBJECT_0)
        {
            goto error;
        }
        #else

        // TODO
        (void) call(args_copy);

        #endif

        if (true)
        {
            free(args_copy);
        }
        else
        {
        error:
            free(args_copy);
            throw MiscError("Thread run failed.");
        }
    }
    else
    {
        (void) call(&args);
    }
}

void ogm::interpreter::fn::external_free(VO out, V id)
{
    external_free_impl(id.castCoerce<external_id_t>());
}
