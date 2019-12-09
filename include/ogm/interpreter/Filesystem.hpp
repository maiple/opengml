#pragma once

#include "ogm/common/types.hpp"
#include <fstream>
#include <vector>
#include <string>

namespace ogm { namespace interpreter
{
    typedef int32_t file_handle_id_t;

    enum class FileAccessType
    {
        read,
        write,
        append
    };

    struct FileHandle
    {
        bool m_active;
        FileAccessType m_type;
        std::string m_path;
        union
        {
            std::ifstream* m_ifstream;
            std::ofstream* m_ofstream;
        };

        inline void cleanup()
        {
            m_active = false;
            switch (m_type)
            {
            case FileAccessType::read:
                if (m_ifstream) delete m_ifstream;
                m_ifstream = nullptr;
                break;
            case FileAccessType::write:
            case FileAccessType::append:
                if (m_ofstream) delete m_ofstream;
                m_ofstream = nullptr;
                break;
            }
        }

        ~FileHandle()
        {
            cleanup();
        }

        FileHandle()=default;
        FileHandle(FileHandle&& fh)
            : m_active(fh.m_active)
            , m_type(fh.m_type)
            , m_path(fh.m_path)
            , m_ifstream(fh.m_ifstream)
        {
            fh.m_ifstream = nullptr;
        }
    };

    // resolves paths to "working directory paths", which are either in
    // the save folder or in the included files directory.
    class Filesystem
    {
        // open files list
        std::vector<FileHandle> m_files;

    public:
        // TODO: move this to %APPDATA% or wherever.
        std::string m_working_directory = "." + std::string(1, PATH_SEPARATOR);
        std::string m_included_directory = "";

        bool file_exists(const std::string&);

        // returns 0 on failure
        template<FileAccessType, bool binary=false>
        file_handle_id_t open_file(const std::string&);
        FileHandle& get_file_handle(file_handle_id_t id)
        {
            return m_files.at(id - 1);
        }
        void close_file(file_handle_id_t);

        // checks for included files first, then checks other directories.
        bool file_is_included(const std::string& path);
        std::string resolve_file_path(const std::string& path, bool write=false);
    };
}}
