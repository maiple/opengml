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

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::asset_get_type(VO out, V asset)
{
    asset_index_t index = asset.castCoerce<asset_index_t>();
    Asset* a = frame.m_assets.get_asset<Asset*>(index);
    if (dynamic_cast<AssetObject*>(a))
    {
        out = 1;
    }
    else if (dynamic_cast<AssetSprite*>(a))
    {
        out = 0;
    }
    else if (dynamic_cast<AssetBackground*>(a))
    {
        out = 2;
    }
    else if (dynamic_cast<AssetRoom*>(a))
    {
        out = 4;
    }
    else
    {
        out = -1;
    }
}

void ogm::interpreter::fn::object_exists(VO out, V obj)
{
    asset_index_t index = obj.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetObject*>(index);
}

void ogm::interpreter::fn::object_get_parent(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_parent;
}

void ogm::interpreter::fn::object_get_name(VO out, V obj)
{
    asset_index_t index = obj.castCoerce<asset_index_t>();
    if (frame.m_assets.get_asset<AssetObject*>(index))
    {
        out = frame.m_assets.get_asset_name(index);
    }
    else
    {
        out = "";
    }
}

void ogm::interpreter::fn::object_get_sprite(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_sprite_index;
}

void ogm::interpreter::fn::object_get_mask(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_mask_index;
}

void ogm::interpreter::fn::object_get_depth(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_depth;
}

void ogm::interpreter::fn::object_get_solid(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_solid;
}

void ogm::interpreter::fn::object_get_persistent(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_persistent;
}

void ogm::interpreter::fn::object_get_visible(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_init_visible;
}

void ogm::interpreter::fn::object_is_ancestor(VO out, V obj, V ancestor)
{
    out = false;
    asset_index_t obji = obj.castCoerce<asset_index_t>();
    asset_index_t ancestori = ancestor.castCoerce<asset_index_t>();
    while (obji != k_no_asset && obji != ancestori)
    {
        AssetObject* object = frame.m_assets.get_asset<AssetObject*>(obji);
        if (object->m_parent == ancestori)
        {
            out = true;
            break;
        }
        else
        {
            obji = object->m_parent;
        }
    }
}

void ogm::interpreter::fn::object_set_sprite(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_sprite_index = v.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::object_set_mask(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_mask_index = v.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::object_set_depth(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_depth = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_persistent(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_persistent = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_visible(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_visible = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_solid(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_solid = v.castCoerce<real_t>();
}

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
        out = "";
    }
}

void ogm::interpreter::fn::asset_get_index(VO out, V a)
{
    std::string asset_name = a.castCoerce<std::string>();
    asset_index_t index;
    frame.m_assets.get_asset(asset_name.c_str(), index);

    if (index == k_no_asset)
    {
        out = -1;
    }
    else
    {
        out = index;
    }
}

void ogm::interpreter::fn::script_exists(VO out, V a)
{
    asset_index_t index = a.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetScript*>(index);
}

void ogm::interpreter::fn::script_get_name(VO out, V a)
{
    asset_index_t index = a.castCoerce<asset_index_t>();
    const char* asset_name = frame.m_assets.get_asset_name(index);
    if (asset_name)
    {
        out = asset_name;
    }
    else
    {
        //CHECK
        out.copy(k_undefined_variable);
    }
}

void ogm::interpreter::fn::script_execute(VO out, uint8_t argc, const Variable* argv)
{
    if (argc == 0)
    {
        throw MiscError("Need script to execute");
    }
    else
    {
        asset_index_t index = argv[0].castCoerce<asset_index_t>();
        const AssetScript* script = dynamic_cast<AssetScript*>(frame.m_assets.get_asset(index));
        bytecode_index_t bytecode_index = script->m_bytecode_index;
        Bytecode bytecode = frame.m_bytecode.get_bytecode(bytecode_index);

        // OPTIMIZE: instead of copying in variables from argv, exploit that
        // they are already on the stack and supplied in the correct order when
        // script_execute is called. (Offset by one to account for the script argument0.)
        uint8_t argc_supplied = argc - 1;
        if (bytecode.m_argc == static_cast<uint8_t>(-1) || bytecode.m_argc == argc_supplied)
        {
            Variable retv[16];
            assert(16 >= bytecode.m_retc);
            call_bytecode(bytecode, retv, argc_supplied, argv + 1);
            if (bytecode.m_retc > 0)
            {
                out = std::move(retv[0]);
                for (size_t i = 1; i < bytecode.m_retc; ++i)
                {
                    retv[i].cleanup();
                }
            }
        }
        else
        {
            throw MiscError("Incorrect number of arguments for executed script \""
                + std::string(frame.m_assets.get_asset_name(index)) + "\"; supplied " + std::to_string(argc_supplied)
                + ", signature is " + std::to_string(bytecode.m_argc));
        }
    }
}

void ogm::interpreter::fn::room_get_width(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    out = room->m_dimensions.x;
}

void ogm::interpreter::fn::room_get_height(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    out = room->m_dimensions.y;
}

void ogm::interpreter::fn::room_get_view_enabled(VO out, V r)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    out = room->m_enable_views;
}

void ogm::interpreter::fn::room_get_view_visible(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = room->m_views.at(index).m_visible;
}

void ogm::interpreter::fn::room_get_view_wview(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = room->m_views.at(index).m_dimension.x;
}

void ogm::interpreter::fn::room_get_view_hview(VO out, V r, V i)
{
    AssetRoom* room = frame.get_asset_from_variable<AssetRoom>(r);
    assert(room);
    size_t index = i.castCoerce<size_t>();
    if (index >= room->m_views.size())
    {
        throw MiscError("No view number " + std::to_string(index));
    }
    out = room->m_views.at(index).m_dimension.y;
}

void ogm::interpreter::fn::sprite_exists(VO out, V vs)
{
    asset_index_t index = vs.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetBackground*>(index);
}

void ogm::interpreter::fn::sprite_get_number(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->image_count();
}

void ogm::interpreter::fn::sprite_get_width(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_dimensions.x;
}

void ogm::interpreter::fn::sprite_get_height(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_dimensions.y;
}

void ogm::interpreter::fn::sprite_get_xoffset(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_offset.x;
}

void ogm::interpreter::fn::sprite_get_yoffset(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_offset.y;
}

void ogm::interpreter::fn::sprite_get_bbox_left(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_aabb.m_start.x;
}

void ogm::interpreter::fn::sprite_get_bbox_top(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_aabb.m_start.y;
}

void ogm::interpreter::fn::sprite_get_bbox_right(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_aabb.m_end.x;
}

void ogm::interpreter::fn::sprite_get_bbox_bottom(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    assert(s);
    out = s->m_aabb.m_end.y;
}

void ogm::interpreter::fn::background_exists(VO out, V bg)
{
    asset_index_t index = bg.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetBackground*>(index);
}

void ogm::interpreter::fn::background_get_width(VO out, V vb)
{
    AssetBackground* bg = frame.get_asset_from_variable<AssetBackground>(vb);
    out = bg->m_dimensions.x;
}

void ogm::interpreter::fn::background_get_height(VO out, V vb)
{
    AssetBackground* bg = frame.get_asset_from_variable<AssetBackground>(vb);
    out = bg->m_dimensions.y;
}

void ogm::interpreter::fn::background_get_name(VO out, V vb)
{
    asset_index_t index = vb.castCoerce<asset_index_t>();
    if (frame.m_assets.get_asset<AssetBackground*>(index))
    {
        out = frame.m_assets.get_asset_name(index);
    }
    else
    {
        out = "";
    }
}

void ogm::interpreter::fn::background_duplicate(VO out, V vb)
{
    AssetBackground* bg = frame.get_asset_from_variable<AssetBackground>(vb);
    out = bg->m_dimensions.x;
    asset_index_t asset_index;
    AssetBackground* new_bg = frame.m_assets.add_asset<AssetBackground>(
        ("?unnamed" + std::to_string(frame.m_assets.asset_count())).c_str(),
        &asset_index
    );
    *new_bg = *bg;
    out = asset_index;
    frame.m_display->bind_asset(asset_index, bg->m_path);
}
