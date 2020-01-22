#pragma once

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/collision/collision.hpp"
#include "ogm/interpreter/serialize.hpp"
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
    real_t m_alpha = 1;
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
    ogm::collision::World<coord_t, tile_id_t, collision::SpatialHash<coord_t, 359>> m_collision;

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s)
    {
        _serialize<write>(s, m_depth);
        _serialize_vector_replace<write>(s, m_contents);

        // TileWorld will handle our collision data.
    }
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

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s)
    {
        _serialize_map<write>(s, m_tiles);
        _serialize_vector_replace<write>(s, m_tile_layers);

        // update collision
        if (!write)
        {
            for (TileLayer& layer : m_tile_layers)
            {
                layer.m_collision.clear();
                for (tile_id_t tile_id : layer.m_contents)
                {
                    ogm_assert(m_tiles.find(tile_id) != m_tiles.end());
                    Tile& tile = m_tiles.at(tile_id);
                    ogm::collision::Entity<coord_t, tile_id_t> entity
                    { ogm::collision::ShapeType::rectangle,
                        tile.get_aabb(), tile_id};
                    layer.m_collision.emplace_entity(entity);
                }
            }
        }
    }

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

    void tile_update_collision(TileLayer& layer, Tile& t, tile_id_t id)
    {
        ogm_assert(tile_exists(id));
        ogm_assert(&get_tile(id) == &t);
        ogm::collision::Entity<coord_t, tile_id_t> entity
        { ogm::collision::ShapeType::rectangle,
            t.get_aabb(), id};
        layer.m_collision.replace_entity(t.m_collision_id, entity);
    }

    // as above, but requires only tile id.
    void tile_update_collision(tile_id_t id)
    {
        Tile& tile = get_tile(id);
        TileLayer* t = find_tile_layer_at_depth(tile.m_depth);
        ogm_assert(t); // tile layer must exist for a tile which exists.
        tile_update_collision(*t, tile, id);
    }

    void tile_delete(tile_id_t id)
    {
        ogm_assert(tile_exists(id));
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
        ogm_assert(tile_exists(id));
        return m_tiles.at(id);
    }

    const Tile& get_tile(tile_id_t id) const
    {
        ogm_assert(tile_exists(id));
        return m_tiles.at(id);
    }

    TileLayer* find_tile_layer_at_depth(real_t depth)
    {
        // OPTIMIZE
        size_t i = 0;
        while (i < m_tile_layers.size())
        {
            if (m_tile_layers.at(i).m_depth >= depth)
            {
                if (m_tile_layers.at(i).m_depth == depth)
                {
                    return &m_tile_layers.at(i);
                }
                return nullptr;
            }
            ++i;
        }
        return nullptr;
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

    void change_layer_depth(real_t from, real_t to)
    {
        // find layer to move:
        int32_t i = -1;
        while (i < m_tile_layers.size())
        {
            if (m_tile_layers.at(i).m_depth == from)
            {
                break;
            }
        }

        if (i >= 0)
        {
            auto iterbeg = m_tile_layers.begin() + i;

            // edit depth of tiles.
            // OPTIMIZE: must tiles really store their own depth..?
            for (tile_id_t tile_id : iterbeg->m_contents)
            {
                Tile& tile = get_tile(tile_id);
                tile.m_depth = to;
            }

            // perform layer rotation
            auto iterend = m_tile_layers.begin();
            bool exact_match = false;
            for (; iterend != m_tile_layers.end(); ++iterend)
            {
                if (iterend->m_depth >= to)
                {
                    if (iterend->m_depth == to)
                    {
                        exact_match = true;
                    }
                    break;
                }
            }
            if (exact_match)
            // layer already in use; merge
            {
                std::move(iterbeg->m_contents.begin(), iterbeg->m_contents.end(), iterend->m_contents.end());
                iterbeg->m_contents.clear();
                m_tile_layers.erase(iterbeg, iterbeg + 1);
            }
            else
            {
                if (iterbeg < iterend)
                {
                    std::rotate(
                        iterbeg,
                        iterbeg + 1,
                        iterend
                    );
                }
                if (iterend < iterbeg)
                {
                    std::rotate(
                        iterend,
                        iterbeg - 1,
                        iterbeg
                    );
                }

                // edit layer depth. (Tiles' depths edited above.)
                iterend->m_depth = to;
            }
        }
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

    // tiles should be added using the Frame `add_tile` method.
    void add_tile_as(Tile&& tile, tile_id_t id)
    {
        ogm_assert(!tile_exists(id));
        auto& layer = get_tile_layer_at_depth(tile.m_depth);
        layer.m_contents.push_back(id);

        // tile collision
        ogm::collision::Entity<coord_t, tile_id_t> entity{ ogm::collision::ShapeType::rectangle,
            tile.get_aabb(), id};
        tile.m_collision_id = layer.m_collision.emplace_entity(entity);

        m_tiles[id] = std::move(tile);
    }

};
}}
