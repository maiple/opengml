#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::tile_add(VO out, V _bg, V _left, V _top, V _width, V _height, V _x, V _y, V _depth)
{
    Tile tile;
    tile.m_background_index = frame.get_asset_index_from_variable<AssetBackground>(_bg);
    tile.m_bg_position.x = _left.castCoerce<real_t>();
    tile.m_bg_position.y = _top.castCoerce<real_t>();
    tile.m_dimension.x = _width.castCoerce<real_t>();
    tile.m_dimension.y = _height.castCoerce<real_t>();
    tile.m_position.x = _x.castCoerce<real_t>();
    tile.m_position.y = _y.castCoerce<real_t>();
    tile.m_visible = true;
    tile.m_depth = _depth.castCoerce<real_t>();
    tile.m_scale = { 1, 1 };
    tile.m_blend = 0xffffffff;

    out = static_cast<real_t>(frame.add_tile(std::move(tile)));
}

void ogm::interpreter::fn::tile_delete(VO out, V id)
{
    if (frame.m_tiles.tile_exists(id.castCoerce<real_t>()))
    {
        frame.m_tiles.tile_delete(id.castCoerce<real_t>());
    }
}

void ogm::interpreter::fn::tile_exists(VO out, V id)
{
    out = frame.m_tiles.tile_exists(id.castCoerce<real_t>());
}

void ogm::interpreter::fn::tile_set_alpha(VO out, V id, V alpha)
{
    Tile& tile = frame.m_tiles.get_tile(id.castCoerce<real_t>());
    tile.m_alpha = alpha.castCoerce<real_t>();
}

void ogm::interpreter::fn::tile_set_blend(VO out, V id, V blend)
{
    Tile& tile = frame.m_tiles.get_tile(id.castCoerce<real_t>());
    tile.m_blend = blend.castCoerce<uint32_t>();
}

void ogm::interpreter::fn::tile_set_position(VO out, V vid, V x, V y)
{
    tile_id_t id = vid.castCoerce<tile_id_t>();
    Tile& tile = frame.m_tiles.get_tile(id);
    tile.m_position = {
        x.castCoerce<coord_t>(),
        y.castCoerce<coord_t>()
    };
    frame.m_tiles.tile_update_collision(id);
}

void ogm::interpreter::fn::tile_set_region(VO out, V vid, V left, V top, V width, V height)
{
    tile_id_t id = vid.castCoerce<tile_id_t>();
    Tile& tile = frame.m_tiles.get_tile(id);
    tile.m_bg_position = {
        left.castCoerce<coord_t>(),
        top.castCoerce<coord_t>()
    };
    tile.m_dimension = {
        width.castCoerce<coord_t>(),
        height.castCoerce<coord_t>()
    };
    frame.m_tiles.tile_update_collision(id);
}

void ogm::interpreter::fn::tile_set_visible(VO out, V vid, V visible)
{
    Tile& tile = frame.m_tiles.get_tile(vid.castCoerce<tile_id_t>());
    tile.m_visible = visible.cond();
}

void ogm::interpreter::fn::tile_set_background(VO out, V vid, V bg)
{
    Tile& tile = frame.m_tiles.get_tile(vid.castCoerce<tile_id_t>());
    asset_index_t index;
    AssetBackground* bga = frame.get_asset_from_variable<AssetBackground>(bg, index);
    tile.m_background_index = index;
}

void ogm::interpreter::fn::tile_set_scale(VO out, V vid, V sx, V sy)
{
    tile_id_t id = vid.castCoerce<tile_id_t>();
    Tile& tile = frame.m_tiles.get_tile(id);
    tile.m_scale = {
        sx.castCoerce<coord_t>(),
        sy.castCoerce<coord_t>()
    };
    frame.m_tiles.tile_update_collision(id);
}

void ogm::interpreter::fn::tile_set_depth(VO out, V vid, V d)
{
    real_t depth = d.castCoerce<real_t>();
    tile_id_t id = vid.castCoerce<tile_id_t>();
    Tile tile = frame.m_tiles.get_tile(id);
    real_t prev_depth = tile.m_depth;
    if (prev_depth == depth) return; // early out.
    tile.m_depth = depth;

    frame.m_tiles.tile_delete(id);
    frame.m_tiles.add_tile_as(std::move(tile), id);
}

void ogm::interpreter::fn::tile_get_background(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_background_index);
}

void ogm::interpreter::fn::tile_get_depth(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_depth);
}

void ogm::interpreter::fn::tile_get_width(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_dimension.x);
}

void ogm::interpreter::fn::tile_get_height(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_dimension.y);
}

void ogm::interpreter::fn::tile_get_left(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_bg_position.x);
}

void ogm::interpreter::fn::tile_get_top(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_bg_position.y);
}

void ogm::interpreter::fn::tile_get_visible(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_visible;
}

void ogm::interpreter::fn::tile_get_x(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_position.x);
}

void ogm::interpreter::fn::tile_get_y(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_position.y);
}

void ogm::interpreter::fn::tile_get_xscale(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_scale.x);
}

void ogm::interpreter::fn::tile_get_yscale(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_scale.y);
}

void ogm::interpreter::fn::tile_get_blend(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_blend);
}

void ogm::interpreter::fn::tile_get_alpha(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_alpha);
}

void ogm::interpreter::fn::tile_get_count(VO out, V id)
{
    out = static_cast<real_t>(frame.m_tiles.get_count());
}

void ogm::interpreter::fn::tile_get_ids_at_depth(VO out, V depth)
{
    // OPTIMIZE: reserve array
    out.array_ensure();
    if (out.array_height() == 0)
    {
        out.array_get(0, 0) = 0;
    }

    size_t i = 0;
    TileLayer* layer = frame.m_tiles.find_tile_layer_at_depth(depth.castCoerce<real_t>());
    bool added = false;
    if (layer)
    {
        for (tile_id_t index : layer->m_contents)
        {
            ogm_assert(frame.m_tiles.tile_exists(index));
            out.array_get(0, i++) = index;
            added = true;
        }
    }

    if (!added)
    {
        // return 0 instead
        out.cleanup();
        out = 0.0;
    }
}

void ogm::interpreter::fn::tile_get_ids(VO out)
{
    // OPTIMIZE: reserve array
    out.array_ensure();
    size_t i = 0;
    for (auto& pair : frame.m_tiles.get_tiles())
    {
        tile_id_t id = std::get<0>(pair);
        ogm_assert(frame.m_tiles.tile_exists(id));
        out.array_get(0, i++) = id;
    }

    // can't return a zero-length array under any circumstance!
    if (i == 0)
    {
        out = static_cast<real_t>(0);
    }
}

void ogm::interpreter::fn::tile_layer_find(VO out, V depth, V x, V y)
{
    TileLayer& layer = frame.m_tiles.get_tile_layer_at_depth(depth.castCoerce<real_t>());
    ogm::geometry::Vector<coord_t> position{ x.castCoerce<real_t>(), y.castCoerce<real_t>() };
    tile_id_t tile_id = -1;
    layer.m_collision.iterate_vector(position,
        [&tile_id](ogm::collision::entity_id_t id, const ogm::collision::Entity<coord_t, tile_id_t>& e) -> bool
    {
        tile_id = e.m_payload;
        return false;
    }, false);

    out = static_cast<real_t>(tile_id);
}

void ogm::interpreter::fn::tile_layer_delete_at(VO out, V depth, V x, V y)
{
    // OPTIMIZE -- finds ID, then deletes, looking up tile by ID.
    TileLayer& layer = frame.m_tiles.get_tile_layer_at_depth(depth.castCoerce<real_t>());
    ogm::geometry::Vector<coord_t> position{ x.castCoerce<real_t>(), y.castCoerce<real_t>() };
    tile_id_t tile_id;
    bool found = false;
    layer.m_collision.iterate_vector(position,
        [&tile_id, &found](ogm::collision::entity_id_t id, const ogm::collision::Entity<coord_t, tile_id_t>& e) -> bool
    {
        tile_id = e.m_payload;
        found = true;
        return false;
    }, false);

    if (found)
    {
        frame.m_tiles.tile_delete(tile_id);
    }
}

void ogm::interpreter::fn::tile_layer_delete(VO out, V depth)
{
    // OPTIMIZE heavily
    real_t d = depth.castCoerce<real_t>();
    while (frame.m_tiles.get_tile_layer_count(d))
    {
        tile_id_t id = frame.m_tiles.get_tile_layer_at_depth(d).m_contents.front();
        frame.m_tiles.tile_delete(id);
    }
}

void ogm::interpreter::fn::tile_layer_hide(VO out, V depth)
{
    // OPTIMIZE heavily
    real_t d = depth.castCoerce<real_t>();
    if (frame.m_tiles.get_tile_layer_count(d))
    {
        for (tile_id_t id : frame.m_tiles.get_tile_layer_at_depth(d).m_contents)
        {
            Tile& t = frame.m_tiles.get_tile(id);
            t.m_visible = false;
        }
    }
}

void ogm::interpreter::fn::tile_layer_show(VO out, V depth)
{
    // OPTIMIZE heavily
    real_t d = depth.castCoerce<real_t>();
    if (frame.m_tiles.get_tile_layer_count(d))
    {
        for (tile_id_t id : frame.m_tiles.get_tile_layer_at_depth(d).m_contents)
        {
            Tile& t = frame.m_tiles.get_tile(id);
            t.m_visible = true;
        }
    }
}

void ogm::interpreter::fn::tile_layer_shift(VO out, V depth, V x, V y)
{
    ogm::geometry::Vector<coord_t> v{ x.castCoerce<real_t>(), y.castCoerce<real_t>() };

    // OPTIMIZE heavily
    real_t d = depth.castCoerce<real_t>();
    if (frame.m_tiles.get_tile_layer_count(d))
    {
        auto& layer = frame.m_tiles.get_tile_layer_at_depth(d);
        for (tile_id_t id : layer.m_contents)
        {
            Tile& t = frame.m_tiles.get_tile(id);
            t.m_position += v;
            frame.m_tiles.tile_update_collision(layer, t, id);
        }
    }
}

void ogm::interpreter::fn::tile_layer_depth(VO out, V dprev, V dnew)
{
    real_t dp = dprev.castCoerce<real_t>();
    real_t dn = dnew.castCoerce<real_t>();

    frame.m_tiles.change_layer_depth(dp, dn);
}
