#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/ds/List.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define dslm staticExecutor.m_frame.m_ds_list

void ogm::interpreter::fn::ds_list_create(VO out)
{
    out = dslm.ds_new();
}

void ogm::interpreter::fn::ds_list_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent list.");
    }

    // clean up contents
    for (Variable& v : dslm.ds_get(index).m_data)
    {
        v.cleanup();
    }

    dslm.ds_delete(index);
}

void ogm::interpreter::fn::ds_list_clear(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }
    DSList& list = dslm.ds_get(index);

    // clean up contents
    for (Variable& v : list.m_data)
    {
        v.cleanup();
    }
    list.m_size = 0;
    list.m_data.clear();
}

void ogm::interpreter::fn::ds_list_empty(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }
    DSList& list = dslm.ds_get(index);
    assert((list.m_size == 0) == list.m_data.empty());
    out = list.m_size == 0;
}

void ogm::interpreter::fn::ds_list_size(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }
    DSList& list = dslm.ds_get(index);

    out = list.m_size;
}

void ogm::interpreter::fn::ds_list_add(VO out, V vindex, V value)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }
    DSList& list = dslm.ds_get(index);

    // TODO: this takes linear time. Could be improved. (Use a skip list?)
    auto iter = list.m_data.before_begin();
    std::advance(iter, list.m_size++);
    list.m_data.emplace_after(iter)->copy(value);
}

void ogm::interpreter::fn::ds_list_add(VO out, unsigned char argc, const Variable* argv)
{
    if (argc < 2)
    {
        throw MiscError("ds_list_add requires at least 2 arguments.");
    }

    const V& vindex = argv[0];
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }
    DSList& list = dslm.ds_get(index);

    // TODO: retrieving end iterator takes linear time. Could be improved. (Use a skip list?)
    auto iter = list.m_data.before_begin();
    std::advance(iter, list.m_size);
    for (size_t i = 1; i < argc; i++)
    {
        iter = list.m_data.emplace_after(iter);
        iter->copy(argv[i]);
    }

    list.m_size += argc - 1;
}

void ogm::interpreter::fn::ds_list_set(VO out, V vindex, V vlindex, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    size_t lindex = vlindex.castCoerce<size_t>();

    if (lindex < list.m_size)
    {
        ogm::interpreter::fn::ds_list_replace(out, vindex, vlindex, val);
    }
    else
    {
        // pad with zeros
        auto iter = list.m_data.before_begin();
        std::advance(iter, list.m_size);
        size_t zero_count = lindex - list.m_size;
        while (zero_count--)
        {
            iter = list.m_data.emplace_after(iter, 0);
        }

        // append element.
        list.m_data.emplace_after(iter)->copy(val);
        list.m_size = lindex + 1;
    }
}

void ogm::interpreter::fn::ds_list_delete(VO out, V vindex, V vlindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    size_t lindex = vlindex.castCoerce<size_t>();

    if (lindex >= list.m_size)
    {
        throw MiscError("Deleting non-existent element in list");
    }
    else
    {
        auto iter = list.m_data.before_begin();
        std::advance(iter, lindex);
        list.m_data.erase_after(iter);
        list.m_size--;
    }
}

void ogm::interpreter::fn::ds_list_find_index(VO out, V vindex, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    auto iter = std::find(list.m_data.begin(), list.m_data.end(), val);
    if (iter == list.m_data.end())
    {
        out = -1;
    }
    else
    {
        // TODO: this is another linear call ontop of the previous std::find.
        size_t distance = std::distance(list.m_data.begin(), iter);
        out = distance;
    }
}

void ogm::interpreter::fn::ds_list_find_value(VO out, V vindex, V vlindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    size_t lindex = vlindex.castCoerce<size_t>();

    if (lindex >= list.m_size)
    {
        out.copy(k_undefined_variable);
    }
    else
    {
        auto iter = list.m_data.begin();
        std::advance(iter, lindex);
        out.copy(*iter);
    }
}

void ogm::interpreter::fn::ds_list_sort(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    list.m_data.sort();
}

void ogm::interpreter::fn::ds_list_insert(VO out, V vindex, V vlindex, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    size_t lindex = vlindex.castCoerce<size_t>();

    if (lindex > list.m_size)
    {
        throw MiscError("Inserting after end of list.");
    }
    else
    {
        auto iter = list.m_data.before_begin();
        std::advance(iter, lindex);
        list.m_data.emplace_after(iter)->copy(val);
        list.m_size++;
    }
}

void ogm::interpreter::fn::ds_list_replace(VO out, V vindex, V vlindex, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    size_t lindex = vlindex.castCoerce<size_t>();

    if (lindex >= list.m_size)
    {
        throw MiscError("Replacing non-existent element in list.");
    }

    auto iter = list.m_data.begin();
    std::advance(iter, lindex);
    iter->cleanup();
    iter->copy(val);
}

void ogm::interpreter::fn::ds_list_shuffle(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent list.");
    }

    DSList& list = dslm.ds_get(index);
    std::vector<Variable> tmp(list.m_size);
    std::move(list.m_data.begin(), list.m_data.end(), tmp.begin());
    // because we use std::rand, we cannot use the stl implementation.
    for (size_t i = 0; i < list.m_size - 1; i++)
    {
        size_t remaining = list.m_size - i;

        // this is biased.
        size_t swap_index = rand() % remaining;

        std::swap(tmp[i], tmp[i + swap_index]);
    }
    std::move(tmp.begin(), tmp.end(), list.m_data.begin());
}

void ogm::interpreter::fn::ds_list_copy(VO out, V dst, V src)
{
    ds_index_t src_index = src.castCoerce<ds_index_t>();
    ds_index_t dst_index = dst.castCoerce<ds_index_t>();
    if (!dslm.ds_exists(src_index))
    {
        throw MiscError("Source list in copy does not exist.");
    }
    if (!dslm.ds_exists(dst_index))
    {
        throw MiscError("Destination list in copy does not exist.");
    }

    ogm::interpreter::fn::ds_list_clear(out, dst);
    DSList& src_list = dslm.ds_get(src_index);
    DSList& dst_list = dslm.ds_get(dst_index);

    auto src_iter = src_list.m_data.begin();
    auto dst_iter = dst_list.m_data.before_begin();
    while (src_iter != src_list.m_data.end())
    {
        dst_iter = dst_list.m_data.emplace_after(dst_iter);
        dst_iter->copy(*(src_iter++));
    }

    dst_list.m_size = src_list.m_size;
}
