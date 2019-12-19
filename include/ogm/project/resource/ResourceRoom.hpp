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
public:
    struct CreationCode
    {
        bytecode_index_t m_bytecode_index;
        ogm_ast_t* m_ast;
        std::string m_name;
        std::string m_source;
    };

    // note bien: these differ slightly from the AssetRoom versions.
    struct InstanceDefinition
    {
        std::string m_name;
        std::string m_object_name;
        geometry::Vector<coord_t> m_position;
        geometry::Vector<coord_t> m_scale { 1, 1 };
        real_t m_angle = 0;
        uint32_t m_colour = 0xffffff;

        // creation code
        std::string m_code;
    };

    struct TileDefinition
    {
        std::string m_background_name;
        instance_id_t m_id;
        geometry::Vector<coord_t> m_position;

        // left, top
        geometry::Vector<coord_t> m_bg_position;

        // internal dimension (right - left, bottom - top).
        geometry::Vector<coord_t> m_dimensions;
        geometry::Vector<coord_t> m_scale {1, 1};

        uint32_t m_blend = 0xffffff;
        real_t m_alpha = 1;
        real_t m_depth;
    };

    struct BackgroundLayerDefinition
    {
        std::string m_background_name;
        geometry::Vector<coord_t> m_position;
        geometry::Vector<coord_t> m_velocity;
        bool m_tiled_x, m_tiled_y;
        bool m_visible;
        bool m_foreground;
    };

    ResourceRoom(const char* path, const char* name);

    void load_file() override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);
    const char* get_name() { return m_name.c_str(); }

    std::string m_path;
    std::string m_name;
    CreationCode m_cc_room;
    std::vector<CreationCode> m_cc_instance;
    std::vector<InstanceDefinition> m_instances;
    std::vector<TileDefinition> m_tiles;
    std::vector<BackgroundLayerDefinition> m_backgrounds;

    // stores all room data except instance, tiles, and backgrounds.
    asset::AssetRoom m_data;

    // actual asset
    asset::AssetRoom* m_asset_room;

private:
    void load_file_xml();

    void load_file_arf();
};

}}
