#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/geometry/Vector.hpp"

#include <string>
#include <map>

namespace ogm::asset {
    class AssetRoom;
}

namespace ogm::project {

class ResourceRoom : public Resource
{
public:
    struct CreationCode
    {
        bytecode_index_t m_bytecode_index;
        std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t> m_ast;
        std::string m_name;
        std::string m_source;
    };

    // nota bene: these differ slightly from the AssetRoom versions.
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

        // nani
        bool m_locked = false;
        
        #ifdef OGM_LAYERS
        // index of layer in this room's m_layers vector.
        size_t m_layer_index;
        #endif
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

        bool m_locked = false;
    };

    struct BackgroundLayerDefinition
    {
        std::string m_background_name;
        geometry::Vector<coord_t> m_position;
        geometry::Vector<coord_t> m_velocity;
        bool m_tiled_x, m_tiled_y;
        bool m_visible;
        bool m_foreground;
        bool m_stretch;
    };

    ResourceRoom(const char* path, const char* name);

    void load_file() override;
    bool save_file() override;
    void precompile(bytecode::ProjectAccumulator&) override;
    void compile(bytecode::ProjectAccumulator&) override;
    const char* get_type_name() override { return "room"; };

    std::string m_path;
    std::string m_comment;
    CreationCode m_cc_room;
    std::vector<CreationCode> m_cc_instance;
    std::vector<InstanceDefinition> m_instances;
    std::vector<TileDefinition> m_tiles;
    std::vector<BackgroundLayerDefinition> m_backgrounds;
    
    #ifdef OGM_LAYERS
    bool m_layers_enabled = false;
    struct LayerDefinition
    {
        enum
        {
            lt_folder,
            lt_background,
            lt_instance,
            // TODO: more of these.
        } m_type;
        std::string m_name;
        real_t m_depth = 0;
        uint32_t m_colour = 0xffffffff;
        bool m_vtile : 1;        // for background layers
        bool m_htile : 1;        // for background layers
        std::string m_asset_name; // for background layers
        geometry::Vector<coord_t> m_position{ 0, 0 };
        geometry::Vector<coord_t> m_velocity{ 0, 0 };
        
        bool m_visible = true;
    };
    std::vector<LayerDefinition> m_layers;
    #else
    static constexpr bool m_layers_enabled = false;
    #endif

    geometry::Vector<coord_t> m_snap{ 16, 16 };
    bool m_isometric = false;
    bool m_persistent = false;

    // stores all room data except instance, tiles, and backgrounds.
    asset::AssetRoom m_data;

    // actual asset
    asset::AssetRoom* m_asset_room;

private:
    void load_file_xml();
    void load_file_arf();
    void load_file_json();
    void load_file_json_layer(const void* json);

    bool save_file_xml(std::ofstream&);
    bool save_file_arf(std::ofstream&);
};

}