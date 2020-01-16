#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/asset/AssetSound.hpp"

#include <string>
#include <map>

namespace ogm { namespace project {

class ResourceSound : public Resource
{
public:
    ResourceSound(const char* path, const char* name);

    void load_file() override;
    void precompile(bytecode::ProjectAccumulator&);

    asset::AssetSound* m_asset;
    std::string m_data_path;
    std::string m_path;
    std::string m_name;

private:
    void load_file_xml();
    void load_file_arf();
};

}}
