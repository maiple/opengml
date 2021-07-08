#include "ogm/common/util_sys.hpp"

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

#include <Windows.h>

#ifndef CPP_FILESYSTEM_ENABLED
#include <shellapi.h>
#include <fileapi.h>
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif

namespace ogm {

void sleep(int32_t ms)
{
    if (ms < 0)
    {
        ms = 0;
    }
    Sleep(ms);
}

bool is_terminal()
{
    static int result = -1;

    if (result >= 0) return result;

    HWND consoleWnd = GetConsoleWindow();
    DWORD dwProcessId;
    GetWindowThreadProcessId(consoleWnd, &dwProcessId);
    if (GetCurrentProcessId()!=dwProcessId)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

// https://solarianprogrammer.com/2019/04/08/c-programming-ansi-escape-codes-windows-macos-linux-terminals/
static HANDLE stdoutHandle;
static DWORD outModeInit;
static bool terminal_colours_enabled = false;

void enable_terminal_colours()
{
    if (!is_terminal()) return;
    
    DWORD outMode = 0;
    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if(stdoutHandle == INVALID_HANDLE_VALUE) {
        terminal_colours_are_supported = false;
 		return;
 	}
    if(!GetConsoleMode(stdoutHandle, &outMode)) {
   		terminal_colours_are_supported = false;
        return;
   	}
    outModeInit = outMode;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if(!SetConsoleMode(stdoutHandle, outMode)) {
        terminal_colours_are_supported = false;
 		return;
 	}
    terminal_colours_enabled = true;
}

void restore_terminal_colours()
{
    if (!is_terminal()) return;
    if (!terminal_colours_enabled) return;
    
    // reset colour if set.
    std::cout << ansi_colour("0");
    
    if(SetConsoleMode(stdoutHandle, outModeInit));
}

// returns path to the directory containing the executable.
std::string get_binary_directory()
{
    const size_t bufsize = 1024;
    char buf[bufsize];
    int32_t len = static_cast<int32_t>(GetModuleFileName(
        nullptr,
        buf,
        bufsize
    ));

    // FIXME: handle len == nSize
    if (len <= 0)
    {
        throw MiscError("Failed to determine binary directory name.");
    }
    else
    {
        buf[len] = 0;
        strcpy(buf, path_directory(buf).c_str());
        return static_cast<char*>(buf);
    }
}

#ifndef CPP_FILESYSTEM_ENABLED
#define BUFFLEN 1024

// this is required to ensure double-null terminated paths.

bool create_directory(const std::string& path)
{
    char buff[BUFFLEN];
    if (path.length() >= BUFFLEN - 2)
    {
      throw MiscError("path too long");
    }
    memset(buff, BUFFLEN, 0);
    strcpy(buff, path.c_str());
    return CreateDirectory(buff);
}

bool remove_directory(const std::string& path)
{
    char buff[BUFFLEN];
    if (path.length() >= BUFFLEN - 2)
    {
      throw MiscError("path too long");
    }
    memset(buff, 0, BUFFLEN);
    strcpy(buff, path.c_str());
    SHFILEOPSTRUCT shfo = {
        nullptr,
        FO_DELETE,
        buff,
        nullptr,
        FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION,
        FALSE,
        nullptr,
        nullptr
    };

    return !SHFileOperation(&shfo);
}

std::string get_temp_root()
{
    char buff[BUFFLEN];
    if (GetTempPath2(buff, BUFFLEN / sizeof(TCHAR)) == 0)
    {
        throw MiscError("error occurred getting temp directory");
    }
    else
    {
        if (sizeof(TCHAR) == sizeof(char))
        {
          return buff;
        }
        else
        {
          char buff2[BUFFLEN];
          if (WideCharToMultiByte(CP_UTF8, 0, buff, wcslen(buff), buff2, BUFFLEN, nullptr, 0) == 0)
          {
              throw MiscError("error occurred converting charactersets for temp directory");
          }

          return buff2;
        }
    }
}

std::string create_temp_directory()
{
    throw MiscError("std::filesystem support required");
}

//https://stackoverflow.com/a/20847429
std::vector<std::string> __glob(const std::string& _search_path)
{
   std::string search_path = native_path(_search_path);
   std::vector<std::string> names;
   WIN32_FIND_DATA fd;

   size_t star_index = search_path.find("*");
   std::string dirpath = search_path.substr(0, star_index);

   HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
   if(hFind != INVALID_HANDLE_VALUE) {
       do {
           // read all (real) files in current folder
           // , delete '!' read other 2 default folder . and ..
           if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
               names.push_back(dirpath + fd.cFileName);
           }
       }while(::FindNextFile(hFind, &fd));
       ::FindClose(hFind);
   }
   return names;
}


// external declaration
std::vector<std::string> __glob(const std::string& _search_path);

namespace
{
    template<bool recursive>
    void _list_paths_impl(const std::string& base, std::vector<std::string>& out)
    {
        std::vector<std::string> subs = __glob(base + "*");
        for (const std::string& sub : subs)
        {
            // for debugging windows.
            std::cout << "listing " << sub << std::endl;
        
            out.push_back(sub);
            if (recursive && path_is_directory(sub))
            {
                std::string s;
                std::cout << "  path: " << s << std::endl;
                if (ends_with(s, PATH_SEPARATOR + ""))
                {
                    _list_paths_impl<recursive>(sub, out);
                }
                else
                {
                    _list_paths_impl<recursive>(sub + PATH_SEPARATOR, out);
                }
            }
        }
    }
}

void list_paths_recursive(const std::string& base, std::vector<std::string>& out)
{
    _list_paths_impl<true>(base, out);
}

void list_paths(const std::string& base, std::vector<std::string>& out)
{
    _list_paths_impl<false>(base, out);
}
#endif

}

#endif