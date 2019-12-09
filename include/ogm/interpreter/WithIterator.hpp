#ifndef OGMI_WITH_ITER_HPP
#define OGMI_WITH_ITER_HPP

#include "ogm/common/error.hpp"
#include <cstdlib>

namespace ogm { namespace interpreter {

class Instance;
class Frame;

class WithIterator
{
friend class Frame;
    void* m_instance;
    size_t m_at = 0;
    size_t m_count : 31;
    bool m_single : 1;

public:
    WithIterator() {}

    WithIterator(WithIterator&& other)
    {
        m_instance = other.m_instance;
        m_at = other.m_at;
        m_count = other.m_count;
        m_single = other.m_single;
        other.m_instance = nullptr;
    }

    inline bool complete() const
    {
        if (m_single)
        {
            return !m_instance;
        }
        else
        {
            return m_at >= m_count;
        }
    }

    // there are optimizations for single- or zero-instance-iterators.
    // note that even non-single iterators might have exactly one or zero instances.
    inline bool is_single() const
    {
        return m_single;
    }

    // only safe to call for singles.
    inline Instance* get_single_instance() const
    {
        ogm_assert(is_single());
        return static_cast<Instance*>(m_instance);
    }

    inline Instance* operator*() const
    {
        if (m_single)
        {
            return get_single_instance();
        }
        else
        {
            return static_cast<Instance**>(m_instance)[m_at];
        }
    }

    inline WithIterator& operator++()
    {
        if (m_single)
        {
            m_instance = nullptr;
        }
        else
        {
            m_at++;
        }
        return *this;
    }

    ~WithIterator()
    {
        if (!m_single)
        {
            delete[] static_cast<Instance**>(m_instance);
        }
    }
};

}}

#endif
