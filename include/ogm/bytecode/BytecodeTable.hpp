#pragma once

#include "Namespace.hpp"

#include "ogm/common/types.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <memory>
#include <vector>
#include <cassert>
#include <cstring>
#include <map>

namespace ogm
{

namespace bytecode
{

typedef int32_t bytecode_address_t;

inline bool operator<(const ogm_ast_line_column_t& a, const ogm_ast_line_column_t& b)
{
    if (a.m_line == b.m_line)
    {
        return a.m_column < b.m_column;
    }
    return a.m_line < b.m_line;
}

// maps bytecode indices to source lines
class DebugSymbolSourceMap
{
public:
    struct Range
    {
        bytecode_address_t m_address_start;
        bytecode_address_t m_address_end;
        ogm_ast_line_column_t m_source_start;
        ogm_ast_line_column_t m_source_end;

        // this range is associated with a statement.
        bool m_statement;
    };

private:
    // todo: find better data structure
    std::vector<Range> m_ranges;

public:
    inline void add_location(bytecode_address_t address_start, bytecode_address_t address_end, ogm_ast_line_column_t start, ogm_ast_line_column_t end, bool statement=false)
    {
        m_ranges.push_back({ address_start, address_end, start, end, statement});
    }

    void get_locations_at(bytecode_address_t address, std::vector<Range>& out_locations) const
    {
        for (const Range& range : m_ranges)
        {
            if (range.m_address_start <= address && address < range.m_address_end)
            {
                out_locations.push_back(range);
            }
        }
    }

    // gets the strictest range for the given address
    // returns false if no location found
    bool get_location_at(bytecode_address_t address, Range& out_range) const
    {
        std::vector<Range> ranges;
        get_locations_at(address, ranges);
        if (ranges.empty())
        // no ranges
        {
            return false;
        }
        else
        // at least one range
        {
            // return the range which has the greates start point.
            std::sort(ranges.begin(), ranges.end(),
                [](const Range& a, const Range& b) -> bool
                {
                    return a.m_source_start < b.m_source_start;
                }
            );
            out_range = ranges.back();
            return true;
        }
    }
};

// the debug symbols for a bytecode section.
struct DebugSymbols
{
public:
    Namespace m_namespace_local;
    const char* m_name;
    const char* m_source;
    DebugSymbolSourceMap m_source_map;

public:
    DebugSymbols()
        : m_namespace_local()
        , m_name(nullptr)
        , m_source(nullptr)
        , m_source_map()
    { }

    DebugSymbols(const char* name, const char* source)
        : m_namespace_local()
        , m_name(nullptr)
        , m_source(nullptr)
        , m_source_map()
    {
        if (name)
        {
            m_name = new_c_str(name);
        }

        if (source)
        {
            m_source = new_c_str(source);
        }
    }

    DebugSymbols(DebugSymbols&& other)
        : m_namespace_local(std::move(other.m_namespace_local))
        , m_name(other.m_name)
        , m_source(other.m_source)
        , m_source_map(std::move(other.m_source_map))
    {
        other.m_name = nullptr;
        other.m_source = nullptr;
    }

    ~DebugSymbols() noexcept
    {
    }
};

struct Bytecode
{
    std::shared_ptr<const char> m_data;
    size_t m_length;

    // metadata
    uint8_t m_retc, m_argc;

    std::shared_ptr<const ogm::bytecode::DebugSymbols> m_debug_symbols;

    Bytecode(const char* data, size_t length, uint8_t retc = 0, uint8_t argc = 0, ogm::bytecode::DebugSymbols* symbols = nullptr)
        : m_data(memdup(data, length), std::default_delete<const char[]>())
        , m_length(length)
        , m_retc(retc)
        , m_argc(argc)
        , m_debug_symbols(symbols)
    { }

    Bytecode()
        : m_data(nullptr)
        , m_length(0)
        , m_retc(0)
        , m_argc(0)
        , m_debug_symbols(nullptr)
    { }

    Bytecode(const Bytecode& other)=default;
    Bytecode(Bytecode&& other)=default;
    Bytecode& operator=(const Bytecode& other)=default;
    Bytecode& operator=(Bytecode&& other)=default;
};

struct BytecodeStream
{
    Bytecode m_bytecode;
    size_t m_pos;

    BytecodeStream(const Bytecode& bytecode)
        : m_bytecode(bytecode)
        , m_pos(0)
    { }

    BytecodeStream()
        : m_bytecode()
        , m_pos(0)
    { }

    BytecodeStream(const Bytecode& bytecode, size_t pos)
        : m_bytecode(bytecode)
        , m_pos(pos)
    { }

    BytecodeStream(const BytecodeStream& other)
        : m_bytecode(other.m_bytecode)
        , m_pos(other.m_pos)
    { }

    BytecodeStream& operator=(const BytecodeStream& other) = default;

    BytecodeStream(BytecodeStream&& other)
        : m_bytecode(std::move(other.m_bytecode))
        , m_pos(other.m_pos)
    { }

    template<typename T, int size=sizeof(T)>
    void read(T& t)
    {
        assert(m_pos + size <= m_bytecode.m_length);
        for (size_t i = 0; i < size; i++)
        {
            reinterpret_cast<char*>(&t)[i] = m_bytecode.m_data.get()[m_pos++];
        }
    }

	template<typename T>
	void operator>>(T& t)
	{
		read(t);
	}

    size_t tellg() const
    {
        return m_pos;
    }

    void seekg(size_t pos)
    {
        assert(pos <= m_bytecode.m_length);
        m_pos = pos;
    }
};

static const bytecode_index_t k_no_bytecode = -1;

class BytecodeTable
{
    std::vector<Bytecode> m_bytecode;
    #ifdef PARALLEL_COMPILE
    mutable std::mutex m_mutex;
    #endif
public:
    inline size_t count() const
    {
        READ_LOCK(m_mutex);
        return m_bytecode.size();
    }

    inline bool has_bytecode(bytecode_index_t bytecode_index) const
    {
        READ_LOCK(m_mutex);
        return has_bytecode_NOMUTEX(bytecode_index);
    }

    inline bool has_bytecode_NOMUTEX(bytecode_index_t bytecode_index) const
    {
        if (bytecode_index >= m_bytecode.size())
        {
            return false;
        }
        return !!m_bytecode.at(bytecode_index).m_data;
    }

    inline Bytecode get_bytecode(bytecode_index_t bytecode_index) const
    {
        READ_LOCK(m_mutex);
        assert(bytecode_index != k_no_bytecode);
        assert(bytecode_index < m_bytecode.size());
        return m_bytecode.at(bytecode_index);
    }

    inline void add_bytecode(const char* bytecode, size_t length, bytecode_index_t bytecode_index, uint8_t retc, uint8_t argc)
    {
        WRITE_LOCK(m_mutex);
        assert(bytecode_index != k_no_bytecode);
        if (has_bytecode_NOMUTEX(bytecode_index))
        {
            remove_bytecode_NOMUTEX(bytecode_index);
        }
        m_bytecode.resize(std::max<size_t>(bytecode_index + 1, m_bytecode.size()));
        char* b = new char[length];
        memcpy(b, bytecode, length);
        m_bytecode[bytecode_index] = { b, length, retc, argc };
    }

    inline void add_bytecode(const char* bytecode, size_t length, bytecode_index_t bytecode_index, uint8_t retc, uint8_t argc, bytecode::DebugSymbols&& symbols)
    {
        assert(bytecode_index != k_no_bytecode);
        m_bytecode.resize(std::max<size_t>(bytecode_index + 1, m_bytecode.size()));
        char* b = new char[length];
        memcpy(b, bytecode, length);
        m_bytecode[bytecode_index] = { b, length, retc, argc, new DebugSymbols(std::move(symbols)) };
    }

    inline void add_bytecode(bytecode_index_t bytecode_index, const Bytecode& bytecode)
    {
        WRITE_LOCK(m_mutex);
        m_bytecode.resize(std::max<size_t>(bytecode_index + 1, m_bytecode.size()));
        m_bytecode[bytecode_index] = bytecode;
    }

    inline void add_bytecode(bytecode_index_t bytecode_index, Bytecode&& bytecode)
    {
        WRITE_LOCK(m_mutex);
        m_bytecode.resize(std::max<size_t>(bytecode_index + 1, m_bytecode.size()));
        m_bytecode[bytecode_index] = std::move(bytecode);
    }

    inline void remove_bytecode(bytecode_index_t bytecode_index)
    {
        WRITE_LOCK(m_mutex);
        remove_bytecode_NOMUTEX(bytecode_index);
    }

    inline void remove_bytecode_NOMUTEX(bytecode_index_t bytecode_index)
    {
        assert(bytecode_index != k_no_bytecode);
        assert(has_bytecode_NOMUTEX(bytecode_index));
        Bytecode& bytecode = m_bytecode.at(bytecode_index);

        if (bytecode.m_data)
        {
            bytecode.m_data = nullptr;
        }

        if (bytecode.m_debug_symbols)
        {
            bytecode.m_debug_symbols = nullptr;
        }
    }

    inline void reserve(size_t count)
    {
        WRITE_LOCK(m_mutex);
        m_bytecode.reserve(count);
    }

    // strips debug symbols, deleting them.
    void strip()
    {
        WRITE_LOCK(m_mutex);
        for (Bytecode& bytecode : m_bytecode)
        {
            if (bytecode.m_debug_symbols)
            {
                // smart pointer requires no explicit deletion.
                bytecode.m_debug_symbols = nullptr;
            }
        }
    }

    void clear()
    {
        WRITE_LOCK(m_mutex);
        for (Bytecode& bytecode : m_bytecode)
        {
            if (bytecode.m_data)
            {
                // smart pointer requires no explicit deletion.
                bytecode.m_data = nullptr;
            }

            if (bytecode.m_debug_symbols)
            {
                // smart pointer requires no explicit deletion.
                bytecode.m_debug_symbols = nullptr;
            }
        }
    }

    BytecodeTable()
    { }

    ~BytecodeTable()
    {
         for (Bytecode& bytecode : m_bytecode)
         {
             if (bytecode.m_data)
             {
                 // smart pointer requires no explicit deletion.
                 bytecode.m_data = nullptr;
             }

             if (bytecode.m_debug_symbols)
             {
                 // smart pointer requires no explicit deletion.
                 bytecode.m_debug_symbols = nullptr;
             }
         }
    }
};

extern const BytecodeTable defaultBytecodeTable;

}
}
