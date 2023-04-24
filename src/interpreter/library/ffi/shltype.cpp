#include "shltype.h"

namespace ogm::interpreter::ffi
{

using namespace std::string_literals;

static char shlheader[0x100];
    
SharedLibraryType getSharedLibraryTypeFromPath(const std::string& path)
{
    SharedLibraryType shtype;
    
    if (path_exists(path) && can_read_file(path))
    {
        memset(shlheader, 0, sizeof(shlheader));
        
        if (get_extension(path) == ".so"s)
        {
            shtype.os = SharedLibraryType::UNIX;
        }
        else if (get_extension(path) == ".dll"s)
        {
            shtype.os = SharedLibraryType::WINDOWS;
        }
        if (get_extension(path) == ".dylib"s)
        {
            shtype.os = SharedLibraryType::APPLE;
        }
        
        size_t len = sizeof(shlheader);
        if (!read_file_contents_fixedlength(path.c_str(), shlheader, len))
        {
            return shtype;
        }
        
        switch(shtype.os)
        {
        case SharedLibraryType::UNIX:
            if (len < 6) return shtype;
            if (shlheader[5] == 1)
            {
                shtype.arch = SharedLibraryType::x86;
            }
            if (shlheader[5] == 2)
            {
                shtype.arch = SharedLibraryType::x64;
            }
            break;
            
        case SharedLibraryType::WINDOWS:
            if (len < 64) return shtype;
            {
                uint16_t magic = *(uint16_t*)(shlheader);
                uint32_t offset = *(uint32_t*)(shlheader + 0x3c);
                
                char buff[6];
                size_t result = sizeof(buff);
                bool success = read_file_contents_fixedlength(path.c_str(), buff, result, offset);
                if (!success) return shtype;
                if (result < 6) return shtype;
                uint16_t sig = *(uint16_t*)(buff);
                uint16_t machine = *(uint16_t*)(buff + 4);
                
                if (sig != 0x4550)
                {
                    return shtype;
                }
                else switch (machine)
                {
                    case 0x014c:
                        // i386
                        shtype.arch = SharedLibraryType::x86;
                        break;
                    case 0x0200:
                        // IA64
                        shtype.arch = SharedLibraryType::x64;
                        break;
                    case 0x8664:
                        // AMD64
                        shtype.arch = SharedLibraryType::x64;
                        break;
                    default:
                        // unknown
                        break;
                }
            }
            break;
            
        case SharedLibraryType::APPLE:
            // TODO
            break;
        }
    }
    
    return shtype;
}

static std::vector<std::string> extensions = {
    ".dll",
    ".so",
    ".dylib",
    ".dll.32",
    ".so.32",
    ".dylib.32",
    ".dll.64",
    ".so.64",
    ".dylib.64",
    ".32.dll",
    ".32.so",
    ".32.dylib",
    ".64.dll",
    ".64.so",
    ".64.dylib",
    ".dll.x86",
    ".so.x86",
    ".dylib.x86",
    ".dll.x64",
    ".so.x64",
    ".dylib.x64",
    ".x86.dll",
    ".x86.so",
    ".x86.dylib",
    ".x64.dll",
    ".x64.so",
    ".x64.dylib",
};

// platform-dependent path transformation
void path_transform(std::string& path)
{
    // swap .dll out for .so if one is available.
    std::string pathnodll = path;
    for (const std::string& extension : extensions)
    {
        if (ends_with(path, extension))
        {
            pathnodll = remove_suffix(path, extension);
        }
    }
    
    std::vector<std::pair<std::string, SharedLibraryType>> shltypes;
    for (const std::string& extension : extensions)
    {
        std::string pathwe = pathnodll + extension;
        SharedLibraryType shltype = getSharedLibraryTypeFromPath(pathwe);
        shltypes.emplace_back(pathwe, shltype);
        
        if (shltype.archmatch() && shltype.osmatch())
        {
            path = pathwe;
            return;
        }
    }
    
    for (const auto& [pathwe, shltype] : shltypes)
    {
        if (shltype.compatible())
        {
            path = pathwe;
            return;
        }
    }
}
}