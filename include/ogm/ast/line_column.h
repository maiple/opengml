#ifndef OGM_LINE_COLUMN_H
#define OGM_LINE_COLUMN_H

#include <cstdint>
#include <cstring>
#include <stdlib.h>

/* line/column/source file type
   accessible from C, constructible/editable/copyable only in C++.

   contains position in original parsed string,
   and an optional position in a source file.
   These may be different if line numbers are remapped with the #line macro.
*/
typedef struct ogm_ast_line_column
{
    // first line is 0
    int m_line;

    // first column is 0
    int m_column;

    // first line is 0
    int m_source_line;

    // first column is 0
    int m_source_column;

    // shared (reference-counted) string describing filename of source file.
    const char* m_source;

    #ifdef __cplusplus
    ogm_ast_line_column(int line=0, int column=0, int source_line=0, int source_column=0, const char* source=nullptr)
        : m_line(line)
        , m_column(column)
        , m_source_line(source_line)
        , m_source_column(source_column)
        , m_source(nullptr)
    {
        if (source)
        {
            char* source_copy = (char*)malloc(strlen(source) + 1 + sizeof(uint32_t)) + sizeof(uint32_t);
            strcpy(source_copy, source);
            m_source = source_copy;
            refs() = 1;
        }
    }

    ogm_ast_line_column(const ogm_ast_line_column& other)
        : m_line(other.m_line)
        , m_column(other.m_column)
        , m_source_line(other.m_source_line)
        , m_source_column(other.m_source_column)
        , m_source(other.m_source)
    {
        if (m_source)
        {
            ++refs();
        }
    }

    ogm_ast_line_column& operator=(const ogm_ast_line_column& other)
    {
        cleanup();
        m_line = other.m_line;
        m_column = other.m_column;
        m_source = other.m_source;
        m_source_line = other.m_source_line;
        m_source_column = other.m_source_column;
        if (m_source) ++refs();

        return *this;
    }

    ogm_ast_line_column& operator=(ogm_ast_line_column&& other)
    {
        cleanup();
        m_line = other.m_line;
        m_column = other.m_column;
        m_source = other.m_source;
        m_source_line = other.m_source_line;
        m_source_column = other.m_source_column;
        other.m_source = nullptr;

        return *this;
    }

    ogm_ast_line_column(ogm_ast_line_column&& other)
        : m_line(other.m_line)
        , m_column(other.m_column)
        , m_source_line(other.m_source_line)
        , m_source_column(other.m_source_column)
        , m_source(other.m_source)
    {
        other.m_source = nullptr;
    }

    void cleanup()
    {
        if (m_source)
        {
            if (--refs() == 0)
            {
                free(const_cast<char*>(m_source) - sizeof(uint32_t));
            }

            // paranoia.
            m_source = nullptr;
        }
    }

    ~ogm_ast_line_column()
    {
        cleanup();
    }

    uint32_t& refs()
    {
        return *(reinterpret_cast<uint32_t*>(const_cast<char*>(m_source)) - 1);
    }
    #endif

} ogm_ast_line_column_t;

#ifdef __cplusplus
inline bool operator<(const ogm_ast_line_column_t& a, const ogm_ast_line_column_t& b)
{
    if (a.m_line == b.m_line)
    {
        return a.m_column < b.m_column;
    }
    return a.m_line < b.m_line;
}

inline bool operator<=(const ogm_ast_line_column_t& a, const ogm_ast_line_column_t& b)
{
    if (a.m_line == b.m_line)
    {
        return a.m_column <= b.m_column;
    }
    return a.m_line < b.m_line;
}
#endif

#endif