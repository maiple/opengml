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

#define dsmm staticExecutor.m_frame.m_ds_map

namespace
{
    template<typename M>
    void map_insert(M& map, V key, V val)
    {
        Variable _key, _val;
        _key.copy(key);
        _val.copy(val);
        if (map.find(key) == map.end())
        {
            map.emplace(std::move(_key), std::move(_val));
        }
        else
        {
            map.at(_key).copy(_val);
        }
    }
}

void ogm::interpreter::fn::ds_map_create(VO out)
{
    out = dsmm.ds_new();
}

void ogm::interpreter::fn::ds_map_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent map datastructure.");
    }

    // clean up contents
    DSMap& map = dsmm.ds_get(index);
    for (auto& pair : map.m_data)
    {
        const_cast<Variable&>(std::get<0>(pair)).cleanup();
        std::get<1>(pair).cleanup();
    }

    dsmm.ds_delete(index);
}

void ogm::interpreter::fn::ds_map_clear(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& map = dsmm.ds_get(index);

    for (auto& pair : map.m_data)
    {
        const_cast<Variable&>(std::get<0>(pair)).cleanup();
        std::get<1>(pair).cleanup();
    }
}

void ogm::interpreter::fn::ds_map_copy(VO out, V vindex, V vindex2)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }

    ds_index_t index2 = vindex2.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index2))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }

    DSMap& ds1 = dsmm.ds_get(index);
    DSMap& ds2 = dsmm.ds_get(index2);
    auto& map1 = ds1.m_data;
    auto& map2 = ds2.m_data;

    // clean up dst
    for (auto& pair : map1)
    {
        const_cast<Variable&>(std::get<0>(pair)).cleanup();
        std::get<1>(pair).cleanup();
    }

    // copy
    for (auto& pair : map2)
    {
        map_insert(map1, std::get<0>(pair), std::get<1>(pair));
    }
}

void ogm::interpreter::fn::ds_map_exists(VO out, V vindex, V key)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    out = map.find(key) != map.end();
}

void ogm::interpreter::fn::ds_map_add(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        out = false;
        return;
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    if (map.find(key) == map.end())
    {
        out = true;
        Variable _key, _val;
        _key.copy(key);
        _val.copy(val);
        map.emplace(std::move(_key), std::move(_val));
    }
    else
    {
        out = false;
    }
}

void ogm::interpreter::fn::ds_map_replace(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    map_insert(map, key, val);
}

void ogm::interpreter::fn::ds_map_delete(VO out, V vindex, V key)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    auto iter = map.find(key);
    if (iter != map.end())
    {
        map.erase(iter);
    }
}

void ogm::interpreter::fn::ds_map_empty(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    out = map.empty();
}

void ogm::interpreter::fn::ds_map_size(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    out = map.size();
}

void ogm::interpreter::fn::ds_map_find_first(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    if (map.empty())
    {
        throw UnknownIntendedBehaviourError("Finding first element of empty map.");
    }
    out.copy(std::get<0>(*map.begin()));
}

void ogm::interpreter::fn::ds_map_find_last(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    if (map.empty())
    {
        throw UnknownIntendedBehaviourError("Finding last element of empty map.");
    }
    out.copy(std::get<0>(*map.rbegin()));
}

void ogm::interpreter::fn::ds_map_find_value(VO out, V vindex, V key)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    auto iter = map.find(key);
    if (iter == map.end())
    {
        out.copy(k_undefined_variable);
        return;
    }
    out.copy(std::get<1>(*iter));
}

void ogm::interpreter::fn::ds_map_find_next(VO out, V vindex, V key)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    auto iter = map.find(key);
    if (iter == map.end())
    {
        out.copy(k_undefined_variable);
        return;
    }
    std::advance(iter, 1);
    if (iter == map.end())
    {
        out.copy(k_undefined_variable);
        return;
    }
    out.copy(std::get<0>(*iter));
}

void ogm::interpreter::fn::ds_map_find_previous(VO out, V vindex, V key)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    auto iter = map.find(key);
    if (iter == map.end())
    {
        out.copy(k_undefined_variable);
        return;
    }
    std::advance(iter, -1);
    if (iter == map.end())
    {
        out.copy(k_undefined_variable);
        return;
    }
    out.copy(std::get<0>(*iter));
}
