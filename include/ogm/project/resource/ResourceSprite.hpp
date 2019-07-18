#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"

#include <string>
#include <map>

namespace ogm { namespace project {

class ResourceSprite : public Resource
{
public:
    ResourceSprite(const char* path, const char* name);

    void precompile(bytecode::ProjectAccumulator&);

    asset::AssetSprite* m_sprite_asset;
    std::string m_path;
    std::string m_name;
};

}}
