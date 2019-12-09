#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/geometry/Vector.hpp"

#include <string>
#include <map>

namespace ogm { namespace asset {
    class AssetRoom;
}}

namespace ogm { namespace project {

class ResourceRoom : public Resource
{
    struct CreationCode
    {
        bytecode_index_t m_bytecode_index;
        ogm_ast_t* m_ast;
        std::string m_name;
        std::string m_source;
    };

public:
    ResourceRoom(const char* path, const char* name);

    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);
    const char* get_name() { return m_name.c_str(); }

    std::string m_path;
    std::string m_name;
    geometry::Vector<coord_t> m_dimension;
    real_t m_speed;
    CreationCode m_cc_room;
    std::vector<CreationCode> m_cc_instance;
    asset::AssetRoom* m_asset_room;
};

}}
