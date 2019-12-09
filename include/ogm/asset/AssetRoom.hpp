#pragma once

#include "Asset.hpp"

#include "ogm/geometry/aabb.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/collision/collision.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"

#include <vector>

namespace ogm
{
namespace asset
{
using namespace ogm::geometry;

class AssetRoom : public Asset
{
public:
    struct InstanceDefinition
    {
        instance_id_t m_id;
        asset_index_t m_object_index;
        Vector<coord_t> m_position;
        Vector<coord_t> m_scale = { 1, 1 };
        real_t m_angle = 0;
        uint32_t m_blend = 0xffffffff;

        // creation code
        bytecode_index_t m_cc = bytecode::k_no_bytecode;
    };

    struct TileDefinition
    {
        asset_index_t m_background_index;
        instance_id_t m_id;
        Vector<coord_t> m_position;

        // left, top
        Vector<coord_t> m_bg_position;

        // internal dimension (right - left, bottom - top).
        Vector<coord_t> m_dimension;
        Vector<coord_t> m_scale;

        uint32_t m_blend;
        real_t m_alpha = 1;
    };

    struct TileLayerDefinition
    {
        real_t m_depth;
        std::vector<TileDefinition> m_contents;
    };

    struct BackgroundLayerDefinition
    {
        asset_index_t m_background_index;
        Vector<coord_t> m_position;
        Vector<coord_t> m_velocity;
        bool m_tiled_x, m_tiled_y;
        bool m_visible;
        bool m_foreground;
    };

    struct ViewDefinition
    {
        bool m_visible;
        Vector<coord_t> m_position;
        Vector<coord_t> m_dimension;
    };

    Vector<coord_t> m_dimensions;

    // room creation code
    bytecode_index_t m_cc;

    // instances in the room
    std::vector<InstanceDefinition> m_instances;

    // tile layers (including tiles) in the room
    std::vector<TileLayerDefinition> m_tile_layers;

    std::vector<BackgroundLayerDefinition> m_bg_layers;

    bool m_enable_views;

    std::vector<ViewDefinition> m_views;

    real_t m_speed;
    int32_t m_colour;
    bool m_show_colour;
};

}
}
