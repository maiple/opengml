#pragma once

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/collision/collision.hpp"
#include <vector>
#include <map>


namespace ogm { namespace interpreter
{
using namespace ogm;
using namespace ogm::geometry;
class TileWorld;
class Frame;

struct Tile
{
    friend class TileWorld;
    friend class Frame;
    asset_index_t m_background_index;
    Vector<coord_t> m_position;

    // left, top
    Vector<coord_t> m_bg_position;

    // internal dimension (right - left, bottom - top).
    Vector<coord_t> m_dimension;
    Vector<coord_t> m_scale;

    uint32_t m_blend;

    real_t m_depth;
    bool m_visible;

    ogm::geometry::AABB<coord_t> get_aabb()
    {
        return{ m_position, m_position + m_dimension };
    }

private:
    ogm::collision::entity_id_t m_collision_id;
};

struct TileLayer
{
    real_t m_depth;
    std::vector<tile_id_t> m_contents;
    ogm::collision::World<coord_t, tile_id_t> m_collision;
};

// All tiles in the room
class TileWorld
{
    friend class Frame;

private:
    std::map<tile_id_t, Tile> m_tiles;

    // tile layers (sorted by TileLayer::m_depth increasing)
    std::vector<TileLayer> m_tile_layers;

public:

    const decltype(m_tiles)& get_tiles() const
    {
        return m_tiles;
    }

    const decltype(m_tile_layers)& get_tile_layers() const
    {
        return m_tile_layers;
    }

    bool tile_exists(tile_id_t id) const
    {
        return m_tiles.find(id) != m_tiles.end();
    }

    void tile_delete(tile_id_t id)
    {
        assert(tile_exists(id));
        {
            auto& layer = get_tile_layer_at_depth(get_tile(id).m_depth);
            auto iter = std::find(layer.m_contents.begin(), layer.m_contents.end(), id);
            Tile& tile = m_tiles.at(*iter);
            layer.m_collision.remove_entity(tile.m_collision_id);
            m_tiles.erase(m_tiles.find(id));
            layer.m_contents.erase(iter);
            if (layer.m_contents.size() == 0)
            // erase layer
            {
                // OPTIMIZE -- this is illegible and inefficient
                for (size_t i = 0; i < m_tile_layers.size(); ++i)
                {
                    if (m_tile_layers.at(i).m_depth == layer.m_depth)
                    {
                        m_tile_layers.erase(m_tile_layers.begin() + i);
                    }
                }
            }
        }
    }

    // TODO: rename to tile_at?
    Tile& get_tile(tile_id_t id)
    {
        assert(tile_exists(id));
        return m_tiles.at(id);
    }

    const Tile& get_tile(tile_id_t id) const
    {
        assert(tile_exists(id));
        return m_tiles.at(id);
    }

    TileLayer& get_tile_layer_at_depth(real_t depth)
    {
        // OPTIMIZE
        size_t i = 0;
        while (i < m_tile_layers.size())
        {
            if (m_tile_layers.at(i).m_depth == depth)
            {
                return m_tile_layers.at(i);
            }
            if (m_tile_layers.at(i).m_depth > depth)
            {
                auto iter = m_tile_layers.emplace(m_tile_layers.begin() + i);
                iter->m_depth = depth;
                return *iter;
            }
            ++i;
        }
        m_tile_layers.emplace_back();
        m_tile_layers.back().m_depth = depth;
        return m_tile_layers.back();
    }

    size_t get_tile_layer_count(real_t depth)
    {
        // OPTIMIZE
        size_t i = 0;
        while (i < m_tile_layers.size())
        {
            if (m_tile_layers.at(i).m_depth == depth)
            {
                return m_tile_layers.size();
            }
            if (m_tile_layers.at(i).m_depth > depth)
            {
                return 0;
            }
            ++i;
        }
        return 0;
    }

    void clear()
    {
        m_tiles.clear();
        m_tile_layers.clear();
    }

    inline size_t get_count()
    {
        return m_tiles.size();
    }

private:
    // friend to Frame
    // tiles should be added using the Frame `add_tile` method.
    void add_tile_as(Tile&& tile, tile_id_t id)
    {
        assert(!tile_exists(id));
        auto& layer = get_tile_layer_at_depth(tile.m_depth);
        layer.m_contents.push_back(id);

        // tile collision
        ogm::collision::Entity<coord_t, tile_id_t> entity{ ogm::collision::Shape::rectangle,
            tile.get_aabb(), id};
        tile.m_collision_id = layer.m_collision.emplace_entity(entity);

        m_tiles[id] = std::move(tile);
    }

};
}}
