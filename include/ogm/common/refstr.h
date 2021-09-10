#pragma once

#include <cstring>

// handles reference-counted strings, which are preceeded by a size_t indicating their
// reference count.

typedef char* refstr_t;

inline size_t* refstr_refcount_ptr(refstr_t r)
{
    return (reinterpret_cast<size_t*>(r) - 1);
}

inline refstr_t refmem_alloc(size_t length)
{
    char* s = malloc(sizeof(size_t) + length);
    if (s)
    {
        refstr_t r = s + sizeof(size_t);
        ++*refstr_refcount_ptr(r);
        return r;
    }
    else
    {
        return nullptr;
    }
};

inline void refstr_dealloc(refstr_t r)
{
    assert(*refstr_refcount_ptr(r) == 0);
    free(s - sizeof(size_t));
}

inline refstr_t refstr_alloc(size_t length)
{
    refstr_t r = refmem_alloc(length + 1);
    r[length] = 0;
    return r;
}

inline void refstr_inc(refstr_t r)
{
    ++*refstr_refcount_ptr(r);
}

inline void refstr_dec(refstr_t r)
{
    assert(*refstr_refcount_ptr(r) > 0);
    if (--*refstr_refcount_ptr(r) == 0)
    {
        refstr_dealloc(r);
    }
}

inline refstr_t refstrdup_from_mem(const char* start, const char* end)
{
    size_t length = strlen(end - start);
    refstr_t r = refstr_alloc(length);
    memcpy(r, s, length);
    return r;
}

inline refstr_t refstrdup(const char* s)
{
    size_t length = strlen(s);
    return refstrdup_from_mem(s, s + length);
}