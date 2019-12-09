#pragma once

#include "ogm/common/util.hpp"
#include <string>
#include <ostream>
#include <functional>

//#define NO_USE_COW_STRING

class COWString
{
    struct COWStringData
    {
        std::string m_str;
        uint32_t m_refcount;
        COWStringData(const std::string& s, uint32_t refcount)
            : m_str(s)
            , m_refcount(refcount)
        { }
    };
private:
    #ifndef NO_USE_COW_STRING
    COWStringData* m_s;
    #else
    std::string m_s;
    #endif
public:
    COWString()
        : COWString("")
    { }
    #ifndef NO_USE_COW_STRING
    COWString(const std::string& s)
        : m_s{ new COWStringData(s, 1) }
    { }
    COWString(const COWString& other)
        : m_s(other.m_s)
    {
        ++m_s->m_refcount;
    }
    COWString(COWString&& other)
        : m_s(other.m_s)
    {
        other.m_s = nullptr;
    }
    inline COWString& operator=(const COWString& other)
    {
        decrement();
        m_s = other.m_s;
        ++m_s->m_refcount;
        return *this;
    }
    #else
    COWString(const std::string& s)
        : m_s{ s }
    { }
    COWString(const COWString& other)
        : m_s(other.m_s)
    { }
    #endif

    const std::string* operator->() const
    {
        #ifndef NO_USE_COW_STRING
        return &m_s->m_str;
        #else
        return &m_s;
        #endif
    }

    const std::string& operator*() const
    {
        #ifndef NO_USE_COW_STRING
        return m_s->m_str;
        #else
        return m_s;
        #endif
    }

    // (presently unused)
    /*FORCEINLINE std::string& edit()
    {
        #ifndef NO_USE_COW_STRING
        if (m_s->m_refcount == 1) return m_s->m_str;
        --m_s->m_refcount;
        m_s = new COWStringData(m_s->m_str, 1);
        return m_s->m_str;
        #else
        return m_s;
        #endif
    }*/

    FORCEINLINE ~COWString()
    {
        decrement();
    }

    // convenience
    bool operator==(const std::string& other) const
    {
        return **this == other;
    }

    bool operator==(const COWString& other) const
    {
        return **this == *other;
    }

    bool operator!=(const std::string& other) const
    {
        return **this != other;
    }

    bool operator!=(const COWString& other) const
    {
        return **this != *other;
    }

    friend std::ostream& operator<<( std::ostream &o, const COWString &s ) {
       return o << *s;
    }

    friend std::string operator+(const std::string& lhs, const COWString &s) {
        return lhs + *s;
    }

    friend std::string operator+(const COWString &s, const std::string& rhs) {
        return *s + rhs;
    }

private:
    void decrement()
    {
        #ifndef NO_USE_COW_STRING
        if (!m_s) return;
        --m_s->m_refcount;
        if (m_s->m_refcount == 0) delete m_s;
        #endif
    }
};

namespace std
{
    template <>
    struct hash<COWString>
    {
        std::size_t operator()(const COWString& s) const
        {
            return hash<std::string>()(*s);
        }
    };
}
