#pragma once

#include "ogm/common/error.hpp"

#include <vector>
#include <cstdlib>

namespace ogm { namespace interpreter {

class Buffer
{
public:
    enum Type
    {
        FIXED = 0,
        GROW = 1,
        WRAP = 2,
        FAST = 3
    };
private:
    Type m_type;

    std::vector<unsigned char> m_data;
    size_t m_alignment;
    size_t m_position;

    // adjusts position or size if necessary
    // returns true on failure
    bool bound()
    {
        if (m_position >= m_data.size())
        {
            switch(m_type)
            {
            case FAST:
            case FIXED:
                // can't insert into a full fixed buffer.
                return true;
                break;
            case GROW:
                m_data.resize(std::max<size_t>(std::max<size_t>(64, m_position * 2), m_data.size() * 2));
                break;
            case WRAP:
                m_position = 0;
                break;
            }
        }
        return false;
    }

    // move to next aligned position.
    // returns true if failed
    bool align()
    {
        // align
        while (m_position % m_alignment)
        {
            if (bound()) return true;
            m_position++;
        }

        return false;
    }

public:
    Buffer(size_t size, Type type, size_t align)
        : m_type{ type }
        , m_data(size, 0)
        , m_alignment{ align > 0 ? align : 1 }
        , m_position{ 0 }
    { }

    void write_n(const char* data, size_t count)
    {
        if (align()) return;

        for (size_t i = 0; i < count; ++i)
        {
            if (bound()) return;

            // increment m_position
            m_data[m_position] =
                data[i];
            m_position++;
        }
    }

    // returns amount of data read.
    size_t read_n(char* out, size_t count)
    {
        if (align()) return 0;

        for (size_t i = 0; i < count; ++i)
        {
            if (bound()) return i;

            // increment m_position
            out[i] = m_data[m_position];
            m_position++;
        }

        return count;
    }

    // returns amount of data read.
    size_t peek_n(char* out, size_t count)
    {
        if (align()) return 0;

        size_t m_position_prev = m_position;

        for (size_t i = 0; i < count; ++i)
        {
            if (bound())
            {
                m_position_prev = m_position;
                return i;
            }

            // increment m_position
            out[i] = m_data[m_position];
            m_position++;
        }

        m_position_prev = m_position;
        return count;
    }

    template<typename T>
    void write(const T& t)
    {
        if (align()) return;

        for (size_t i = 0; i < sizeof(T); ++i)
        {
            if (bound()) return;

            // increment m_position
            m_data[m_position] =
                static_cast<unsigned char*>(
                    static_cast<void*>(
                        const_cast<T*>(&t)
                    )
                )[i];
            m_position++;
        }
    }

    template<typename T>
    T read()
    {
        T t;
        if (align()) return 0;

        for (size_t i = 0; i < sizeof(T); ++i)
        {
            if (bound()) return 0;

            // increment m_position
            static_cast<unsigned char*>(
                static_cast<void*>(&t)
            )[i] = m_data[m_position];
            m_position++;
        }
        return t;
    }

    void* get_address()
    {
        return &m_data.at(0);
    }

    void seek(size_t l)
    {
        m_position = l;
        if (bound())
        {
            m_position = m_data.size();
        }
    }

    size_t tell()
    {
        return m_position;
    }

    size_t size()
    {
        return m_data.size();
    }
};

class BufferManager
{
    std::vector<Buffer*> m_buffers;

public:
    ~BufferManager()
    {
        clear();
    }

    // remove all buffers
    void clear()
    {
        for (Buffer* b : m_buffers)
        {
            delete b;
        }
        m_buffers.clear();
    }

    Buffer& get_buffer(size_t id)
    {
        if (id >= m_buffers.size())
        {
            throw MiscError("buffer ID out of bounds.");
        }
        if (m_buffers.at(id))
        {
            return *m_buffers.at(id);
        }
        else
        {
            throw MiscError("retrieving non-existent buffer.");
        }
    }

    size_t create_buffer(size_t size, Buffer::Type type, size_t align)
    {
        for (size_t i = 0; i < m_buffers.size(); ++i)
        {
            if (m_buffers.at(i) == nullptr)
            {
                m_buffers.at(i) = new Buffer(size, type, align);
                return i;
            }
        }

        m_buffers.push_back(new Buffer(size, type, align));
        return m_buffers.size() - 1;
    }

    void delete_buffer(size_t id)
    {
        if (id >= m_buffers.size())
        {
            throw MiscError("buffer ID out of bounds.");
        }
        if (m_buffers.at(id))
        {
            delete m_buffers.at(id);
            m_buffers.at(id) = nullptr;
        }
        else
        {
            throw MiscError("deleting non-existent buffer.");
        }
    }
};

}
}
