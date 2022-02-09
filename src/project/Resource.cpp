#include "ogm/project/resource/Resource.hpp"

namespace ogm::project {

bool Resource::mark_progress(ResourceProgress rp)
{
    if (m_progress & rp)
    {
        return true;
    }
    m_progress = static_cast<ResourceProgress>(rp | m_progress);
    return false;
}

namespace
{
    const std::string k_id_chars = "0123456789abcdefghijklmnopqrstuvwxyz";
}    

bool Resource::is_id(const std::string& id)
{
    if (id.length() == 36)
    {
        if (id.find_first_not_of(k_id_chars, 0) != 8) return false;
        if (id.at(8) != '-') return false;
        if (id.find_first_not_of(k_id_chars, 9) != 13) return false;
        if (id.at(13) != '-') return false;
        if (id.find_first_not_of(k_id_chars, 14) != 18) return false;
        if (id.at(18) != '-') return false;
        if (id.find_first_not_of(k_id_chars, 19) != 23) return false;
        if (id.at(23) != '-') return false;
        if (id.find_first_not_of(k_id_chars, 24) != std::string::npos) return false;
        
        return true;
    }
    else
    {
        return false;
    }
}

// stores an id-to-name map if id matches the id format
bool Resource::store_v2_id(bytecode::ProjectAccumulator& acc, const std::string& id, const std::string& name)
{
    if (is_id(id))
    {
        acc.m_id_map[id] = name;
        return true;
    }
    return false;
}

// replaces the given reference with what its id maps to, if the id is in the id map.
bool Resource::lookup_v2_id(bytecode::ProjectAccumulator& acc, std::string& io_reference)
{
    if (is_id(io_reference))
    {
        auto iter = acc.m_id_map.find(io_reference);
        if (iter != acc.m_id_map.end())
        {
            io_reference = iter->second;
            return true;
        }
    }
    return false;
}

void Resource::assign_id(bytecode::ProjectAccumulator& acc)
{
    // this has no effect if m_v2_id is not set.
    store_v2_id(acc, m_v2_id, m_name);
}

bool resource_name_nil(const char* name)
{
    if (strcmp(name, "")) return true;
    if (strcmp(name, "<undefined>")) return true;
    if (strcmp(name, "&lt;undefined&gt;")) return true;
    if (strcmp(name, "00000000-0000-0000-0000-000000000000")) return true;
    return false;
}

bool resource_name_nil(const std::string& name)
{
    return resource_name_nil(name.c_str());
}

}