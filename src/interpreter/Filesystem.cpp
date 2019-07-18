#include "ogm/interpreter/Filesystem.hpp"

namespace ogmi
{
using namespace ogm;

bool Filesystem::file_exists(std::string path)
{
    std::ifstream ifs(path);
    return ifs.good();
}

template<FileAccessType type, bool binary>
file_handle_id_t Filesystem::open_file(std::string path)
{
    auto access_mode = (type == FileAccessType::read)
        ? std::ios::in
        : std::ios::out;
    if (type == FileAccessType::append)
    {
        access_mode |= std::ios::app;
    }

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
file_handle_id_t Filesystem::open_file<FileAccessType::read, false>(std::string path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::write, false>(std::string path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::append, false>(std::string path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::read, true>(std::string path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::write, true>(std::string path);
template
file_handle_id_t Filesystem::open_file<FileAccessType::append, true>(std::string path);

void Filesystem::close_file(file_handle_id_t id)
{
    if (id <= m_files.size())
    {
        m_files.at(id - 1).cleanup();
    }
}

}
