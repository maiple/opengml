#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/ds/List.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define dssm staticExecutor.m_frame.m_ds_stack

void ogm::interpreter::fn::ds_stack_create(VO out)
{
    out = static_cast<real_t>(dssm.ds_new());
}

void ogm::interpreter::fn::ds_stack_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent stack datastructure.");
    }
    DSStack& map = dssm.ds_get(index);

    for (Variable& v : map.m_data)
    {
        v.cleanup();
    }
    map.m_data.clear();
    dssm.ds_delete(index);
}

void ogm::interpreter::fn::ds_stack_clear(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent stack datastructure.");
    }
    DSStack& map = dssm.ds_get(index);

    for (Variable& v : map.m_data)
    {
        v.cleanup();
    }
    map.m_data.clear();
}

void ogm::interpreter::fn::ds_stack_push(VO out, V vindex, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        out = false;
        return;
    }
    DSStack& ds = dssm.ds_get(index);
    auto& map = ds.m_data;
    map.emplace_back();
    map.back().copy(val);
    map.back().make_root();
}

void ogm::interpreter::fn::ds_stack_pop(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent stack datastructure.");
    }
    DSStack& ds = dssm.ds_get(index);
    if (!ds.m_data.empty())
    {
        out.copy(ds.m_data.back());
        ds.m_data.back().cleanup();
        ds.m_data.pop_back();
    }
}

void ogm::interpreter::fn::ds_stack_top(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent stack datastructure.");
    }
    DSStack& ds = dssm.ds_get(index);
    if (!ds.m_data.empty())
    {
        out.copy(ds.m_data.back());
    }
}


void ogm::interpreter::fn::ds_stack_size(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent stack datastructure.");
    }
    DSStack& ds = dssm.ds_get(index);
    out = static_cast<real_t>(ds.m_data.size());
}


void ogm::interpreter::fn::ds_stack_empty(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dssm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent stack datastructure.");
    }
    DSStack& ds = dssm.ds_get(index);
    out = ds.m_data.empty();
}
