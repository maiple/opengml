#if defined(__unix__) || defined(__APPLE__)

#include "external.h"
#include "shltype.h"
#include <dlfcn.h>
#ifdef __unix__
    #include <link.h>
    #include <elf.h>
#endif

namespace ogm::interpreter::ffi
{
#ifdef PELOADER
external_id_t external_define_peloader_impl(const char* path, const char* fnname, CallType ct, VariableType rettype, uint32_t argc, const VariableType* argt)
{
    ExternalDefinition ed;
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
    ExternalDefinition ed;
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

    throw MiscError("Error loading library \"" + std::string(path) + "\": " + dlerror());
}

void external_list_impl(std::vector<std::string>& outNames, const std::string& path)
{
    #ifdef __unix__
    SharedLibraryType shtype = getSharedLibraryTypeFromPath(path);
    
    // TODO: list zugbruecke, list peloader
    if (!shtype.platmatch())
    {
        return;
    }
    
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

void external_free_impl(external_id_t id)
{
    auto iter = g_dlls.find(id);
    if (iter != g_dlls.end())
    {
        if (std::get<1>(*iter).m_dl)
        {
            if (--g_dll_refc[std::get<1>(*iter).m_dl] == 0)
            {
                #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
                if (std::get<1>(*iter).m_zugbruecke)
                {
                    zugbruecke_free_base(std::get<1>(*iter));
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

        #if defined(EMBED_ZUGBRUECKE) && defined(PYTHON)
        if (std::get<1>(*iter).m_zugbruecke)
        {
            zugbruecke_free_addr(std::get<1>(*iter));
        }
        #endif
    }
}

}

#endif