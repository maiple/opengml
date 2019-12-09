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

#define frame staticExecutor.m_frame
#define dsmm staticExecutor.m_frame.m_ds_map
#define dslm staticExecutor.m_frame.m_ds_list

namespace
{
    template<typename M>
    void map_insert(M& map, V key, V val, DSMap::MarkedVariable::EntryFlag flag=DSMap::MarkedVariable::NONE)
    {
        Variable _key, _val;
        _val.copy(val);
        auto iter = map.find(key);
        if (iter == map.end())
        {
            _key.copy(key);
            map.emplace(std::move(_key), std::move(_val));
        }
        else
        {
            // clean up sub-ds
            if (iter->second.m_flag == DSMap::MarkedVariable::LIST)
            {
                Variable dummy;
                ds_list_destroy(dummy, *iter->second);
            }
            else if (iter->second.m_flag == DSMap::MarkedVariable::MAP)
            {
                Variable dummy;
                ds_map_destroy(dummy, *iter->second);
            }

            // replace value in map
            iter->second->cleanup();
            iter->second->copy(_val);
            iter->second.m_flag = flag;
        }
    }
}

void ogm::interpreter::fn::ds_map_create(VO out)
{
    out = static_cast<real_t>(dsmm.ds_new());
}

void ogm::interpreter::fn::ds_map_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent map datastructure.");
    }

    // clean up contents
    std::vector<ds_index_t> subs_map;
    std::vector<ds_index_t> subs_list;
    DSMap& map = dsmm.ds_get(index);
    for (auto& pair : map.m_data)
    {
        const_cast<Variable&>(std::get<0>(pair)).cleanup();
        DSMap::MarkedVariable& mv = std::get<1>(pair);
        if (mv.m_flag == DSMap::MarkedVariable::MAP)
        {
            subs_map.push_back(mv->castCoerce<size_t>());
        }
        if (mv.m_flag == DSMap::MarkedVariable::LIST)
        {
            subs_list.push_back(mv->castCoerce<size_t>());
        }
        mv->cleanup();
    }

    map.m_data.clear(); // not strictly necessary

    // delete datastructure
    dsmm.ds_delete(index);

    // delete sub-datastructures
    for (ds_index_t map_index : subs_map)
    {
        dsmm.ds_delete(map_index);
    }
    for (ds_index_t list_index : subs_list)
    {
        dslm.ds_delete(list_index);
    }
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
        std::get<1>(pair)->cleanup();
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
        std::get<1>(pair)->cleanup();
    }

    // copy
    for (auto& pair : map2)
    {
        map_insert(map1, std::get<0>(pair), std::get<1>(pair).v);

        if (std::get<1>(pair).m_flag != DSMap::MarkedVariable::NONE)
        {
            throw MiscError("Copying nested datastructures not yet implemented.");
        }
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

void ogm::interpreter::fn::ds_map_add_map(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    map_insert(map, key, val, DSMap::MarkedVariable::MAP);
}


void ogm::interpreter::fn::ds_map_add_list(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    map_insert(map, key, val, DSMap::MarkedVariable::LIST);
}

void ogm::interpreter::fn::ds_map_replace_map(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    map_insert(map, key, val, DSMap::MarkedVariable::MAP);
}


void ogm::interpreter::fn::ds_map_replace_list(VO out, V vindex, V key, V val)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsmm.ds_exists(index))
    {
        throw MiscError("Accessing non-existent map datastructure.");
    }
    DSMap& ds = dsmm.ds_get(index);
    auto& map = ds.m_data;
    map_insert(map, key, val, DSMap::MarkedVariable::LIST);
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
    out = static_cast<real_t>(map.size());
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
    out.copy(iter->second.v);
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

void ogm::interpreter::fn::ds_map_write(VO out, V id)
{
    // TODO: investigate what format this really is supposed to be.
    json_encode(out, id);
}

void ogm::interpreter::fn::ds_map_read(VO out, V id)
{
    ds_map_read(out, id, false);
}

void ogm::interpreter::fn::ds_map_read(VO out, V id, V fmt)
{
    json_decode(out, id);
}

namespace
{
    std::string ds_map_secure_save_string(V id)
    {
        Variable vs;

        ds_map_write(vs, id);

        std::string s = vs.castCoerce<std::string>();

        // TODO: encrypt s.

        vs.cleanup();
        return s;
    }

    ds_index_t ds_map_secure_load_string(V str)
    {
        Variable vi;

        // TODO: decrypt str.

        ds_map_read(vi, str);

        ds_index_t di = vi.castCoerce<ds_index_t>();


        vi.cleanup();
        return di;
    }
}

void ogm::interpreter::fn::ds_map_secure_save(VO out, V id, V file)
{
    std::string s_out = ds_map_secure_save_string(id);
    std::string fpath = file.castCoerce<std::string>();
    file_handle_id_t fhid = frame.m_fs.open_file<FileAccessType::write, true>(fpath);
    *frame.m_fs.get_file_handle(fhid).m_ofstream << s_out;
    frame.m_fs.close_file(fhid);
}

void ogm::interpreter::fn::ds_map_secure_load(VO out, V file)
{
    std::string fpath = file.castCoerce<std::string>();
    file_handle_id_t fhid = frame.m_fs.open_file<FileAccessType::read, true>(fpath);
    std::stringstream ss;
    ss << frame.m_fs.get_file_handle(fhid).m_ifstream->rdbuf();

    // OPTIMIZE: unnecessary string allocs
    Variable str = ss.str();
    ds_index_t di = ds_map_secure_load_string(str);
    frame.m_fs.close_file(fhid);
    str.cleanup();
    out = static_cast<real_t>(di);
}

void ogm::interpreter::fn::ds_map_secure_save_buffer(VO out, V id, V buff)
{
    std::string s = ds_map_secure_save_string(id);
    Buffer& b = frame.m_buffers.get_buffer(id.castCoerce<size_t>());

    // write to buffer
    for (size_t i = 0; i < s.length(); ++i)
    {
        b.write(static_cast<int8_t>(s[i]));
    }

    // write 0
    b.write(static_cast<int8_t>(0));
}

void ogm::interpreter::fn::ds_map_secure_load_buffer(VO out, V id)
{
    Buffer& b = frame.m_buffers.get_buffer(id.castCoerce<size_t>());
    std::stringstream ss;
    while (true)
    {
        const char c = b.read<char>();
        if (c)
        {
            ss << c;
        }
        else
        {
            break;
        }
    }

    // OPTIMIZE: unnecessary string allocs
    Variable str = ss.str();
    out = static_cast<real_t>(ds_map_secure_load_string(str));
    str.cleanup();
}
