#if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)

#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "external.h"

#include <Python.h>
#include <csignal>
#include <stdlib.h>

namespace ogm::interpreter::ffi
{

namespace
{
    bool g_zugbruecke_setup_complete = false;
    bool g_zugbruecke_available;

    // zugbruecke module
    PyObject* g_zugbruecke = nullptr;
    PyObject* g_zugbruecke_cdecl = nullptr;
    PyObject* g_zugbruecke_stdcall = nullptr;
    
    // is kernel32.dll a library that has been automatically loaded?
    bool g_loaded_kernel32 = false;

    PyObject* g_kernel32;
    PyObject* g_kernel32_SetDllPath;
}

void python_add_to_env_path(const std::string& path);

void add_to_env_path(const std::string& path)
{
    const char* pPath;
    pPath = getenv("WINEPATH");
    if (pPath == nullptr)
    {
        pPath = "";
    }
    std::string s = std::string(pPath) + ";" + path;
    setenv("WINEPATH", s.c_str(), 1);
}

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

void zugbruecke_free_base(ExternalDefinition& ed)
{
    ogm_assert(ed.m_zugbruecke);
    PyObject* dl = static_cast<PyObject*>(ed.m_dl);
    Py_DECREF(dl);
}

void zugbruecke_free_addr(ExternalDefinition& ed)
{
    ogm_assert(ed.m_zugbruecke);
    PyObject* fndl = static_cast<PyObject*>(ed.m_dll_fn_address);
    Py_DECREF(fndl);
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
    
    add_to_env_path(staticExecutor.m_frame.m_fs.get_included_directory());

    // setup python
    Py_Initialize();

    // TODO: should be cleaned up witha call to Py_Finalize().

    PyObject *pName = PyUnicode_DecodeFSDefault("zugbruecke.ctypes");
    
    #if 0
    PyObject* pOsName = PyUnicode_DecodeFSDefault("os");
    // TODO: any of these could be null at any point.
    PyObject* os = PyImport_Import(pOsName);
    PyObject* getcwd = PyObject_GetAttrString(os, "getcwd");
    PyObject* chdir = PyObject_GetAttrString(os, "chdir");
    PyObject* emptyTuple = PyTuple_New(0);
    PyObject* prevdir = PyObject_Call(getcwd, emptyTuple, nullptr);
    PyObject* prevDirArgs = PyTuple_New(1);
    PyTuple_SetItem(prevDirArgs, 0, prevdir);
    PyObject* newDir = PyUnicode_DecodeFSDefault(staticExecutor.m_frame.m_fs.get_included_directory().c_str());
    PyObject* newDirArgs = PyTuple_New(1);
    PyTuple_SetItem(newDirArgs, 0, newDir);
    PyObject_Call(chdir, newDirArgs, nullptr);
    #endif
    
    g_zugbruecke = PyImport_Import(pName);
    
    #if 0
    PyObject_Call(chdir, prevDirArgs, nullptr);
    Py_DECREF(pName);
    Py_DECREF(prevdir);
    Py_DECREF(newDir);
    Py_DECREF(pName);
    Py_DECREF(newDirArgs);
    Py_DECREF(getcwd);
    Py_DECREF(chdir);
    Py_DECREF(emptyTuple);
    Py_DECREF(prevDirArgs);
    Py_DECREF(os);
    #endif

    if (!g_zugbruecke)
    {
        std::cerr << "Failed to load module zugbruecke.ctypes for running win32 dll.\n";
        std::cerr << "Try running `python3 -m pip install zugbruecke`.\n";
        return false;
    }

    PyObject* cdll = PyObject_GetAttrString(g_zugbruecke, "cdll");

    // TODO: this was 'cdll' before (redundantly) -- monitor this closely.
    PyObject* windll = PyObject_GetAttrString(g_zugbruecke, "windll");

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
    //python_add_to_env_path(staticExecutor.m_frame.m_fs.get_included_directory());

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

// adds a path to the environment path.
// FIXME this doesn't seem to actually work, even though SetDllDirectoryA is called.
// Dubious, but it could be a path separator problem.
void python_add_to_env_path(const std::string& path)
{
    #if 0
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
        
        check_python_error("(Zugbruecke) Error invoking SetDllDirectory.");

        if (retval)
        {
            if (retval == Py_False)
            {
                throw MiscError("(Zugbruecke) SetDllDirectory returned false (error).");
            }
            Py_DECREF(retval);
        }
        else
        {
            // FIXME: this is being thrown actually.
            throw MiscError("(Zugbruecke) No return value for SetDllDirectoryA.");
        }

        Py_DECREF(arguments);
    }
    #endif
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
    bool needs_memsync = rettype == VT_STRING;
    for (size_t i = 0; i < argc; ++i)
    {
        PyList_SetItem(argtypes, i,
            sig_char(argt[i]) == 'r'
            ? c_real
            : c_string
        );
        if (sig_char(argt[i]) == 's')
        {
            needs_memsync = true;
        }
    }

    // set return and argument types for function.
    PyObject_SetAttrString(fndl, "argtypes", argtypes);
    PyObject_SetAttrString(fndl, "restype", restype);
    
    if (needs_memsync)
    {
        PyObject* memsync = PyList_New(0);
        if (rettype == VT_STRING)
        {
            // return type
            PyObject* dict = PyDict_New();
            {
                PyObject* pointer = PyList_New(0);
                {
                    PyObject* arg = PyUnicode_FromString("r");
                    PyList_Append(pointer, arg);
                    Py_DECREF(arg);
                }
                PyDict_SetItemString(dict, "pointer", pointer);
                PyDict_SetItemString(dict, "null", Py_True);
                
                Py_DECREF(pointer);
            }
            PyList_Append(memsync, dict);
            Py_DECREF(dict);
        }
        
        // argtypes
        for (size_t i = 0; i < argc; ++i)
        {
            if (sig_char(argt[i]) == 's')
            {
                PyObject* dict = PyDict_New();
                {
                    PyObject* pointer = PyList_New(0);
                    {
                        PyObject* arg = PyLong_FromLong(i);
                        PyList_Append(pointer, arg);
                        Py_DECREF(arg);
                    }
                    PyDict_SetItemString(dict, "pointer", pointer);
                    PyDict_SetItemString(dict, "null", Py_True);
                    
                    Py_DECREF(pointer);
                }
                PyList_Append(memsync, dict);
                Py_DECREF(dict);
            }
        }
        PyObject_SetAttrString(fndl, "memsync", memsync);
        Py_DECREF(memsync);
    }

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
        throw MiscError("zugbruecke missing support for call type " + std::string
            (
                (ct == CallType::_CDECL)
                ? "cdecl (zugbruecke.cdll.LoadLibrary)"
                : "stdcall (zugbruecke.windll.LoadLibrary)"
            )
        );
    }
    
    std::string _path_alt;

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
    zugbruecke_init();
    
    ExternalDefinition ed;
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
void external_call_dispatch_zugbruecke(Variable& out, std::string sig, void* fn, byte argc, const Variable* argv, std::string function_name)
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
}
#endif