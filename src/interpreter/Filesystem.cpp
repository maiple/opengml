#include "ogm/common/util.hpp"
#include "ogm/interpreter/Filesystem.hpp"

#ifdef WIN32
#include <shlwapi.h>
#endif

namespace ogm { namespace interpreter
{
using namespace ogm;

bool Filesystem::file_exists(const std::string& path)
{
    std::ifstream ifs(resolve_file_path(path));
    return ifs.good();
}

template<FileAccessType type, bool binary>
file_handle_id_t Filesystem::open_file(const std::string& _path)
{

    // TODO: investigate why this is necessary
    #ifdef _MSC_VER
    int
    #else
    auto
    #endif
    access_mode = (type == FileAccessType::read)
        ? std::ios::in
        : std::ios::out;
    if (type == FileAccessType::append)
    {
        access_mode |= std::ios::app;
    }

    std::string path = resolve_file_path(_path, type != FileAccessType::read);

    // TODO: instead of emplacing at back, scan for first non-active (m_active) file,
    // and replace it.
    m_files.emplace_back();
    FileHandle& fh = m_files.back();
    fh.m_active = true;
    fh.m_type = type;
    if (type == FileAccessType::read)
    {
        fh.m_ifstream = new std::ifstream(path, access_mode);
    }
    else
    {
        fh.m_ofstream = new std::ofstream(path, access_mode);
    }
    if (type == FileAccessType::read && !fh.m_ifstream->good())
    {
        m_files.pop_back();
        return -1;
    }
    else if (type != FileAccessType::read && !fh.m_ofstream->good())
    {
        m_files.pop_back();
        return -1;
    }
    else
    {
        return m_files.size();
    }
}

template
file_handle_id_t Filesystem::open_file<FileAccessType::read, false>(const std::string& path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::write, false>(const std::string& path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::append, false>(const std::string& path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::read, true>(const std::string& path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::write, true>(const std::string& path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::append, true>(const std::string& path);

void Filesystem::close_file(file_handle_id_t id)
{
    if (id <= m_files.size())
    {
        m_files.at(id - 1).cleanup();
    }
}

bool Filesystem::file_is_included(const std::string& path)
{
    if (m_included_directory == "") return false;
    std::ifstream infile(case_insensitive_native_path(m_included_directory, path));
    return infile.good();
}

std::string Filesystem::resolve_file_path(const std::string& path, bool write)
{
    bool absolute = false;

    #if defined(WIN32)
    if (!PathIsRelative(path.c_str()))
    {
        absolute = true;
    }
    #else
    if (starts_with(path, "/"))
    {
        absolute = true;
    }
    #endif


    if (absolute)
    {
        #if defined(WIN32)
        char buff[MAX_PATH];
        for (size_t i = 0; i < MAX_PATH; ++i)
        {
            buff[i] = 0;
        }
        strcpy(buff, path.c_str());
        PathStripToRootA(buff);
        return case_insensitive_native_path(buff, path.substr(strlen(buff)));
        #else
        return case_insensitive_native_path("/", path.substr(1));
        #endif
    }

    if (write)
    {
    working_path:
        return case_insensitive_native_path(m_working_directory, path);
    }
    else
    {
        if (file_is_included(path))
        {
            return case_insensitive_native_path(m_included_directory, path);
        }
        else
        {
            goto working_path;
        }
    }
}

}}
