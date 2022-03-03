#pragma once

#include "ogm/project/resource/Resource.hpp"

// resources that only exist during project build time;
// these have no associated asset.
namespace ogm::project {

class ResourceConstant : public Resource {
public:
    std::string m_value; // what the constant is defined to be

    void precompile(bytecode::ProjectAccumulator& acc) override
    {
        acc.m_reflection->set_macro(m_name.c_str(), m_value.c_str(), acc.m_config->m_parse_flags);
    }

    const char* get_type_name() override { return "constant"; }
    
    ResourceConstant(const std::string& name, const std::string& value)
        : Resource(name)
        , m_value(value)
    { }
};

class ResourceAudioGroup : public Resource {
    const char* get_type_name() override { return "audiogroup"; };
};

}