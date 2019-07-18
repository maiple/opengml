#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::tile_add(VO out, V _bg, V _left, V _top, V _width, V _height, V _x, V _y, V _depth)
{
    real_t depth = _depth.castCoerce<real_t>();
    Tile tile;
    tile.m_background_index = frame.get_asset_index_from_variable<AssetBackground>(_bg);
    tile.m_bg_position.x = _left.castCoerce<real_t>();
    tile.m_bg_position.y = _top.castCoerce<real_t>();
    tile.m_dimension.x = _width.castCoerce<real_t>();
    tile.m_dimension.y = _height.castCoerce<real_t>();
    tile.m_position.x = _x.castCoerce<real_t>();
    tile.m_position.y = _y.castCoerce<real_t>();
    tile.m_visible = true;

    out = frame.add_tile(std::move(tile));
}

void ogmi::fn::tile_delete(VO out, V id)
{
    if (frame.m_tiles.tile_exists(id.castCoerce<real_t>()))
    {
        frame.m_tiles.tile_delete(id.castCoerce<real_t>());
    }
}

void ogmi::fn::tile_exists(VO out, V id)
{
    out = frame.m_tiles.tile_exists(id.castCoerce<real_t>());
}

void ogmi::fn::tile_get_background(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_background_index;
}

void ogmi::fn::tile_get_depth(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_depth;
}

void ogmi::fn::tile_get_width(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_dimension.x;
}

void ogmi::fn::tile_get_height(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_dimension.y;
}

void ogmi::fn::tile_get_left(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_bg_position.x;
}

void ogmi::fn::tile_get_top(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_bg_position.y;
}

void ogmi::fn::tile_get_visible(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_visible;
}

void ogmi::fn::tile_get_x(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_position.x;
}

void ogmi::fn::tile_get_y(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_position.y;
}

void ogmi::fn::tile_get_xscale(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_scale.x;
}

void ogmi::fn::tile_get_yscale(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_scale.y;
}

void ogmi::fn::tile_get_blend(VO out, V id)
{
    out = frame.m_tiles.get_tile(id.castCoerce<real_t>()).m_blend;
}

void ogmi::fn::tile_get_alpha(VO out, V id)
{
    throw NotImplementedError("tile_get_alpha");
}

void ogmi::fn::tile_get_count(VO out, V id)
{
    out = frame.m_tiles.get_count();
}

void ogmi::fn::tile_get_ids_at_depth(VO out, V depth)
{
    // OPTIMIZE: reserve array
    out.array_ensure();
    size_t i = 0;
    for (tile_id_t index : frame.m_tiles.get_tile_layer_at_depth(depth.castCoerce<real_t>()).m_contents)
    {
        assert(frame.m_tiles.tile_exists(index));
        out.array_get(0, i++) = index;
    }

    // can't return a zero-length array under any circumstance!
    if (i == 0)
    {
        out = 0;
    }
}

void ogmi::fn::tile_get_ids(VO out)
{
    // OPTIMIZE: reserve array
    out.array_ensure();
    size_t i = 0;
    for (auto& pair : frame.m_tiles.get_tiles())
    {
        tile_id_t id = std::get<0>(pair);
        assert(frame.m_tiles.tile_exists(id));
        out.array_get(0, i++) = id;
    }

    // can't return a zero-length array under any circumstance!
    if (i == 0)
    {
        out = 0;
    }
}

void ogmi::fn::tile_layer_find(VO out, V depth, V x, V y)
{
    TileLayer& layer = frame.m_tiles.get_tile_layer_at_depth(depth.castCoerce<real_t>());
    ogm::geometry::Vector<coord_t> position{ x.castCoerce<real_t>(), y.castCoerce<real_t>() };
    tile_id_t tile_id = -1;
    layer.m_collision.iterate_vector(position,
        [&tile_id](ogm::collision::entity_id_t id, const ogm::collision::Entity<coord_t, tile_id_t>& e) -> bool
    {
        tile_id = e.m_payload;
        return false;
    });

    out = tile_id;
}

void ogmi::fn::tile_layer_delete_at(VO out, V depth, V x, V y)
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
    });

    if (found)
    {
        frame.m_tiles.tile_delete(tile_id);
    }
}

void ogmi::fn::tile_layer_delete(VO out, V depth)
{
    // OPTIMIZE heavily
    real_t d = depth.castCoerce<real_t>();
    while (frame.m_tiles.get_tile_layer_count(d))
    {
        tile_id_t id = frame.m_tiles.get_tile_layer_at_depth(d).m_contents.front();
        frame.m_tiles.tile_delete(id);
    }
}

void ogmi::fn::tile_layer_hide(VO out, V depth)
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

void ogmi::fn::tile_layer_show(VO out, V depth)
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
