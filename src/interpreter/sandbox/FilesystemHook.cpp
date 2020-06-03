#ifdef OGM_FS_HOOK

#include "ogm/common/util.hpp"
#include "FilesystemHook.hpp"

#ifdef WIN32
    #include <Windows.h>
    #include <shlwapi.h>
    #include <windef.h>
    #include <Ntdef.h>
    #include <stdio.h>
    #include <tchar.h>
    #include <string.h>
    #include <psapi.h>

    #ifdef _MSC_VER
        #include <WInternl.h>
        #include <FileAPI.h>
    #else
        #ifndef _WINTERNL_
            // from mingw-x64
            // we could get this from <WInternl.h>, but not all compilers support that header.

            extern "C" {
                typedef struct _IO_STATUS_BLOCK {
                  union {
                    NTSTATUS Status;
                    PVOID Pointer;
                  };
                  ULONG_PTR Information;
                } IO_STATUS_BLOCK,*PIO_STATUS_BLOCK;


                NTSTATUS NTAPI NtOpenFile(PHANDLE FileHandle,ACCESS_MASK DesiredAccess,POBJECT_ATTRIBUTES ObjectAttributes,PIO_STATUS_BLOCK IoStatusBlock,ULONG ShareAccess,ULONG OpenOptions);

                WINBASEAPI DWORD WINAPI GetFinalPathNameByHandleA(HANDLE, LPSTR, DWORD, DWORD);
            }
        #endif
    #endif
#endif

#ifdef __linux__
    #include <sys/mman.h>
    #include "mprotect.h"
#endif

#include <cassert>
#include <iostream>

namespace ogm::interpreter
{
    bool can_hook()
    {
        // disabled because this is not fully implemented yet.
        #if 0
            #ifdef _WIN32
                return true;
            #else
                return false;
            #endif
        #else
            return false;
        #endif
    }

    FunctionHook::FunctionHook(const std::string& targetName)
        : targetName(targetName)
    {
    }

    // FunctionHook implementation adapted from https://github.com/lapinozz/Vaccinator
    inline void FunctionHook::unprotect()
    {
        #ifdef _WIN32
        DWORD _old_protect;
        VirtualProtect(targetFunction, 0xf, 0x40, &_old_protect);
        old_protect = _old_protect;
        #endif

        #ifdef __linux__
        old_protect = read_mprotection(targetFunction);
        mprotect(targetFunction, 0xf, PROT_WRITE | PROT_READ);
        #endif
    }

    inline void FunctionHook::protect()
    {
        #ifdef _WIN32
        DWORD _old_protect = old_protect;
        VirtualProtect(targetFunction, 0xf, old_protect, &_old_protect);
        #endif

        #ifdef __linux__
        mprotect(targetFunction, 0xf, old_protect);
        #endif
    }

    void FunctionHook::init(void* hookFunction)
    {
        #ifdef _WIN32
        targetFunction = reinterpret_cast<void*>(GetProcAddress(GetModuleHandle("kernel32"), targetName.c_str()));
        if (!targetFunction)
        {
            targetFunction = reinterpret_cast<void*>(GetProcAddress(GetModuleHandle("ntdll"), targetName.c_str()));
        }
        #endif

        shellcode = generateShellcode(reinterpret_cast<uintptr_t>(hookFunction));
        originalCode.resize(shellcode.size());

        // obtain original code
        unprotect();
        memcpy(originalCode.data(), targetFunction, originalCode.size());
        protect();

        hook();
    }

    void FunctionHook::hook()
    {
        unprotect();
        memcpy(targetFunction, shellcode.data(), shellcode.size());
        protect();
    }

    void FunctionHook::unhook()
    {
        unprotect();
        memcpy(targetFunction, originalCode.data(), originalCode.size());
        protect();
    }

    FunctionHook::RawCode FunctionHook::generateShellcode(uintptr_t hook_pointer)
    {
        if (sizeof(uintptr_t) == 8)
        {
            std::vector<uint8_t> hook_bytes = {
                0xFF, 0x35, 0x01, 0x00, 0x00, 0x00,					// PUSH [RIP+1]
                0xC3,												// RET
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };	// HOOK POINTER
            std::memcpy(hook_bytes.data() + 0x7, &hook_pointer, sizeof(hook_pointer));

            return hook_bytes;
        }
        else if (sizeof(uintptr_t) == 4)
        {
            std::vector<uint8_t> hook_bytes = {
                0x68, 0x00, 0x00, 0x00, 0x00,					// PUSH immediate
                0xC3,											// RET;
            };
            std::memcpy(hook_bytes.data() + 0x1, &hook_pointer, sizeof(hook_pointer));

            return hook_bytes;
        }
        else
        {
            throw MiscError();
        }
    }

    #ifdef _WIN32

    namespace
    {
        FunctionHook NtOpenFileHook("NtOpenFile");

        // https://stackoverflow.com/a/17387176
        //Returns the last Win32 error, in string format. Returns an empty string if there is no error.
        std::string GetLastErrorAsString()
        {
            //Get the error message, if any.
            DWORD errorMessageID = ::GetLastError();
            if(errorMessageID == 0)
                return std::string(); //No error message has been recorded

            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                         NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

            std::string message(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);

            return message;
        }

        NTSTATUS NTAPI NtOpenFileFunc(PHANDLE file_handle, ACCESS_MASK desired_access, POBJECT_ATTRIBUTES object_attributes, PIO_STATUS_BLOCK io_status_block, ULONG share_access, ULONG open_options)
        {
            #if 0
            std::cout << "Hook on fs access. ";
            if (object_attributes)
            {
                printf("String: %wZ\n", object_attributes->ObjectName);
            }
            else
            {
                std::cout << "(nullptr)\n";
            }
            #endif

        	return NtOpenFileHook.callOriginal<decltype(&NtOpenFile)>
                (file_handle, desired_access, object_attributes, io_status_block, share_access, open_options);
        }
    }
    #endif

    void hook_fs_open()
    {
        #ifdef _WIN32
        assert(can_hook());
        void* openfile = reinterpret_cast<void*>(&NtOpenFileFunc);
        std::cout << "hooking NtOpenFile at " << openfile << std::endl;
        NtOpenFileHook.init(openfile);
        #endif
    }
}

#endif
