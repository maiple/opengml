#pragma once

#include "ogm/common/error.hpp"
#include <vector>
#include <cstdlib>

namespace ogm::interpreter
{

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
    bool m_good = true;

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
                m_good = false;
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
            if (bound())
            {
                m_good = false;
                return true;
            }
            m_position++;
        }

        return false;
    }

public:
    Buffer(size_t size, Type type, size_t align=1)
        : m_type{ type }
        , m_data(size, 0)
        , m_alignment{ align > 0 ? align : 1 }
        , m_position{ 0 }
    { }

    Buffer() : Buffer(64, GROW, 1)
    { }

    // returns amount of data written.
    size_t write(const char* data, size_t count)
    {
        if (align()) return 0;

        for (size_t i = 0; i < count; ++i)
        {
            if (bound()) return i;

            // increment m_position
            m_data.at(m_position) =
                data[i];
            m_position++;
        }

        return count;
    }

    // returns amount of data read.
    size_t read(char* out, size_t count)
    {
        if (align()) return 0;

        for (size_t i = 0; i < count; ++i)
        {
            if (bound()) return i;

            // increment m_position
            out[i] = m_data.at(m_position);
            m_position++;
        }

        return count;
    }

    // returns amount of data read.
    size_t peek_n(char* out, size_t count)
    {
        size_t m_position_prev = m_position;

        read(out, count);

        m_position = m_position_prev;
        return count;
    }

    // returns true on failure.
    template<typename T>
    bool write(const T& t)
    {
        return write(reinterpret_cast<const char*>(&t), sizeof(T)) < sizeof(T);
    }

    // attempts to read a T; returns true on failure (may corrupt T)
    template<typename T>
    bool read(T& t)
    {
        if (read(reinterpret_cast<char*>(&t), sizeof(T)) < sizeof(T))
        {
            return true;
        }
        return false;
    }

    // legacy
    template<typename T>
    T read()
    {
        T t;
        if (read(t)) return 0;
        return t;
    }

    inline void* get_address()
    {
        if (m_data.empty())
        {
            m_data.resize(64);
        }
        return (&m_data.at(0));
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

    void clear()
    {
        if (m_type != GROW)
        {
            throw MiscError("Can only clear 'grow' buffers.");
        }
        m_good = true;
        m_data.clear();
        m_position = 0;
    }

    bool good()
    {
        return m_good;
    }

    bool eof()
    {
        if (m_type == GROW || m_type == WRAP)
        {
            return false;
        }
        return m_position == m_data.size();
    }

    Type get_type()
    {
        return m_type;
    }

    size_t get_align()
    {
        return m_alignment;
    }

    unsigned char* get_data()
    {
        if (m_data.empty()) throw MiscError("get_data() on empty buffer.");
        
        // this would crash if m_data is empty.
        return &m_data.at(0);
    }
};
}
