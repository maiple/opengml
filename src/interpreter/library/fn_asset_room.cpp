#include "libpre.h"
    #include "fn_asset.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::room_exists(VO out, V rm)
{
    asset_index_t index = rm.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetRoom*>(index);
}

void ogm::interpreter::fn::room_get_name(VO out, V rm)
{
    asset_index_t index = rm.castCoerce<asset_index_t>();
    if (frame.m_assets.get_asset<AssetRoom*>(index))
    {
        out = frame.m_assets.get_asset_name(index);
    }
    else
    {
        out = "<undefined>";
    }
}

void ogm::interpreter::fn::room_get_width(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    out = static_cast<real_t>(room->m_dimensions.x);
}

void ogm::interpreter::fn::room_get_height(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    out = static_cast<real_t>(room->m_dimensions.y);
}

void ogm::interpreter::fn::room_get_view_enabled(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    out = static_cast<real_t>(room->m_enable_views);
}

void ogm::interpreter::fn::room_get_view_visible(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = static_cast<real_t>(room->m_views.at(index).m_visible);
}

void ogm::interpreter::fn::room_get_view_wview(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = static_cast<real_t>(room->m_views.at(index).m_dimension.x);
}

void ogm::interpreter::fn::room_get_view_hview(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    ogm_assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = static_cast<real_t>(room->m_views.at(index).m_dimension.y);
}

void ogm::interpreter::fn::room_add(VO out)
{
    asset_index_t asset_index;
    AssetRoom* r = frame.m_assets.add_asset<AssetRoom>("<dynamic room>", &asset_index);
    r->m_dimensions = { 0, 0 };
    r->m_cc = k_no_bytecode;
    r->m_enable_views = false;
    r->m_speed = 30;
    r->m_colour = 0x0;
    r->m_show_colour = false;
    for (size_t i = 0; i < 8; ++i)
    {
        AssetRoom::BackgroundLayerDefinition bld;
        bld.m_background_index = -1;
        bld.m_position = { 0, 0 };
        bld.m_velocity = { 0, 0 };
        bld.m_tiled_x = false;
        bld.m_tiled_y = false;
        bld.m_visible = false;
        bld.m_foreground = false;
        r->m_bg_layers.push_back(bld);
    }
    out = static_cast<real_t>(asset_index);
}

void ogm::interpreter::fn::room_duplicate(VO out, V fr)
{
    asset_index_t asset_index;
    AssetRoom* from = frame.get_asset_from_variable<AssetRoom>(fr);
    AssetRoom* r = frame.m_assets.add_asset<AssetRoom>("<dynamic room>", &asset_index);

    // copy everything wholesale.
    *r = *from;

    out = static_cast<real_t>(asset_index);
}

void ogm::interpreter::fn::room_instance_clear(VO out, V vr)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_instances.clear();
}

void ogm::interpreter::fn::room_tile_clear(VO out, V vr)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_tile_layers.clear();
}


void ogm::interpreter::fn::room_instance_add(VO out, V vr, V x, V y, V obj)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    AssetRoom::InstanceDefinition def;
    def.m_object_index = obj.castCoerce<asset_index_t>();
    if (! frame.m_assets.get_asset<AssetObject*>(def.m_object_index))
    {
        out = static_cast<real_t>(k_noone);
        return;
    }
    def.m_position = { x.castCoerce<coord_t>(), y.castCoerce<coord_t>() };
    def.m_id = frame.m_config.m_next_instance_id++; // TODO: sub200k ID?
    r->m_instances.push_back(def);
    out = static_cast<real_t>(def.m_id);
}

void ogm::interpreter::fn::room_tile_add(VO out, V vr, V bg, V l, V t, V w, V h, V x, V y, V d)
{
    room_tile_add_ext(out, vr, bg, l, t, w, h, x, y, d, 1, 1, 1);
}

void ogm::interpreter::fn::room_tile_add_ext(VO out, V vr, V bg, V l, V t, V w, V h, V x, V y, V d, V xscale, V yscale, V alpha)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    AssetRoom::TileDefinition td;
    td.m_id = frame.m_config.m_next_instance_id++; // TODO: sub200k ID?
    td.m_background_index = bg.castCoerce<asset_index_t>();
    if (! frame.m_assets.get_asset<AssetBackground*>(td.m_background_index))
    {
        out = static_cast<real_t>(k_noone);
        return;
    }
    td.m_position = { x.castCoerce<coord_t>(), y.castCoerce<coord_t>() };
    td.m_bg_position = { l.castCoerce<coord_t>(), t.castCoerce<coord_t>() };
    td.m_dimension = { w.castCoerce<coord_t>(), h.castCoerce<coord_t>() };
    td.m_scale = { xscale.castCoerce<coord_t>(), yscale.castCoerce<coord_t>() };
    td.m_alpha = alpha.castCoerce<real_t>();
    td.m_blend = 0xffffff; // white

    real_t depth = d.castCoerce<real_t>();

    // find a tile layer to insert in
    bool added = false;
    for (auto iter = r->m_tile_layers.begin(); iter != r->m_tile_layers.end(); ++iter)
    {
        if (iter->m_depth == depth)
        {
            iter->m_contents.push_back(td);
            added = true;
            break;
        }
        else if (iter->m_depth > depth)
        {
            // insert new layer here.
            added = true;
            r->m_tile_layers.insert(iter, { depth, { td } });
            break;
        }
    }

    if (!added)
    {
        r->m_tile_layers.push_back({ depth, { td } });
    }

    out = static_cast<real_t>(td.m_id);
}

void ogm::interpreter::fn::room_set_view_enabled(VO out, V vr, V enable)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_enable_views = enable.cond();
}

void ogm::interpreter::fn::room_set_background(VO out, V vr, V index, V visible, V fg, V id, V x, V y, V htile, V vtile, V hspeed, V vspeed, V alpha)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    if (r->m_bg_layers.size() < index.castCoerce<size_t>())
    {
        r->m_bg_layers.resize(index.castCoerce<size_t>());
    }
    AssetRoom::BackgroundLayerDefinition& bld = r->m_bg_layers[index.castCoerce<size_t>()];
    bld.m_background_index = id.castCoerce<asset_index_t>();
    bld.m_position = { x.castCoerce<coord_t>(), y.castCoerce<coord_t>() };
    bld.m_velocity = { hspeed.castCoerce<coord_t>(), vspeed.castCoerce<coord_t>() };
    bld.m_tiled_x = htile.cond();
    bld.m_tiled_y = vtile.cond();
    bld.m_visible = visible.cond();
    bld.m_foreground = fg.cond();
}

void ogm::interpreter::fn::room_set_background_colour(VO out, V vr, V col, V show)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_colour = col.castCoerce<int32_t>();
    r->m_show_colour = show.cond();
}

void ogm::interpreter::fn::room_set_width(VO out, V vr, V w)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_dimensions.x = w.castCoerce<coord_t>();
}

void ogm::interpreter::fn::room_set_height(VO out, V vr, V w)
{
    AssetRoom* r = frame.get_asset_from_variable<AssetRoom>(vr);
    r->m_dimensions.y = w.castCoerce<coord_t>();
}
