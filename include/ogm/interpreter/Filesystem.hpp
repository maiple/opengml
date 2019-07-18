#pragma once

#include "ogm/common/types.hpp"
#include <fstream>
#include <vector>
#include <string>

namespace ogmi
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
    };

    class Filesystem
    {
        // open files list
        std::vector<FileHandle> m_files;

    public:

        bool file_exists(std::string);

        // returns 0 on failure
        template<FileAccessType, bool binary=false>
        file_handle_id_t open_file(std::string);
        FileHandle& get_file_handle(file_handle_id_t id)
        {
            return m_files.at(id - 1);
        }
        void close_file(file_handle_id_t);
    };
}
