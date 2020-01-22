#pragma once

#include "ogm/common/error.hpp"
#include "ogm/interpreter/Buffer.hpp"

#include <vector>
#include <cstdlib>

namespace ogm { namespace interpreter {

typedef size_t buffer_id_t;
class BufferManager
{
    // pair: buffer and bool "owned."
    std::vector<std::pair<Buffer*, bool>> m_buffers;

public:
    ~BufferManager()
    {
        clear();
    }

    // remove all buffers
    void clear()
    {
        for (auto& [b, owned] : m_buffers)
        {
            if (owned)
            {
                delete b;
            }
        }
        m_buffers.clear();
    }

    Buffer& get_buffer(size_t id)
    {
        if (id >= m_buffers.size())
        {
            throw MiscError("buffer ID out of bounds.");
        }
        if (m_buffers.at(id).first)
        {
            return *m_buffers.at(id).first;
        }
        else
        {
            throw MiscError("retrieving non-existent buffer.");
        }
    }

    buffer_id_t create_buffer(size_t size, Buffer::Type type, size_t align)
    {
        for (buffer_id_t i = 0; i < m_buffers.size(); ++i)
        {
            if (m_buffers.at(i).first == nullptr)
            {
                m_buffers.at(i).first = new Buffer(size, type, align);
                m_buffers.at(i).second = true; // owned.

                return i;
            }
        }

        m_buffers.emplace_back(new Buffer(size, type, align), true);
        return m_buffers.size() - 1;
    }

    buffer_id_t add_existing_buffer(Buffer* b)
    {
        m_buffers.emplace_back(b, false);
        return m_buffers.size() - 1;
    }

    void remove_existing_buffer(buffer_id_t id)
    {
        if (id >= m_buffers.size())
        {
            throw MiscError("buffer ID out of bounds.");
        }
        if (m_buffers.at(id).second)
        {
            throw MiscError("buffer is owned by manager; shouldn't remove without deleting.");
        }
        m_buffers.at(id) = {nullptr, true};
    }

    void delete_buffer(buffer_id_t id)
    {
        if (id >= m_buffers.size())
        {
            throw MiscError("buffer ID out of bounds.");
        }
        if (m_buffers.at(id).first)
        {
            if (m_buffers.at(id).second)
            {
                delete m_buffers.at(id).first;
                m_buffers.at(id).first = nullptr;
            }
            else
            {
                throw MiscError("Deleting this buffer not permitted.");
            }
        }
        else
        {
            throw MiscError("deleting non-existent buffer.");
        }
    }
};

}
}
