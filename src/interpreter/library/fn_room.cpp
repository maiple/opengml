#include "libpre.h"
    #include "fn_room.h"
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

void ogm::interpreter::fn::room_goto(VO out, V rm)
{
    asset_index_t room_index = frame.get_asset_index_from_variable<AssetRoom>(rm);
    frame.m_data.m_room_goto_queued = room_index;
}

void ogm::interpreter::fn::room_goto_next(VO out)
{
    frame.m_data.m_room_goto_queued = frame.m_data.m_room_index + 1;
}

void ogm::interpreter::fn::room_goto_previous(VO out)
{
    frame.m_data.m_room_goto_queued = frame.m_data.m_room_index - 1;
}

void ogm::interpreter::fn::getv::room_first(VO out)
{
    asset_index_t asset_index = 0;
    for (const ogm::asset::Asset* asset : frame.m_assets.get_asset_vector_NOMUTEX())
    {
        if (dynamic_cast<const ogm::asset::AssetRoom*>(asset))
        {
            out = static_cast<real_t>(asset_index);
            return;
        }
        asset_index++;
    }
}

void ogm::interpreter::fn::getv::room_last(VO out)
{
    asset_index_t asset_index = 0;
    for (const ogm::asset::Asset* asset : frame.m_assets.get_asset_vector_NOMUTEX())
    {
        if (dynamic_cast<const ogm::asset::AssetRoom*>(asset))
        {
            out = static_cast<real_t>(asset_index);
        }
        asset_index++;
    }
}

void ogm::interpreter::fn::getv::room(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_room_index);
}

void ogm::interpreter::fn::setv::room(V val)
{
    Variable out;
    room_goto(out, val);
}

void ogm::interpreter::fn::getv::room_width(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_room_dimension.x);
}

void ogm::interpreter::fn::getv::room_height(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_room_dimension.y);
}

void ogm::interpreter::fn::setv::room_speed(V c)
{
    frame.m_data.m_desired_fps = c.castCoerce<real_t>();
}

void ogm::interpreter::fn::getv::room_speed(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_desired_fps);
}

void ogm::interpreter::fn::setv::background_colour(V c)
{
    frame.m_data.m_background_colour = c.castCoerce<int32_t>();
}

void ogm::interpreter::fn::getv::background_colour(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_background_colour);
}

void ogm::interpreter::fn::setv::background_showcolour(V c)
{
    frame.m_data.m_show_background_colour = c.castCoerce<bool>();
}

void ogm::interpreter::fn::getv::background_showcolour(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_show_background_colour);
}

#ifdef OGM_2DARRAY
#define OGM_2DARRAY_i V i,
#else
#define OGM_2DARRAY_i
#endif

void ogm::interpreter::fn::setv::background_visible(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_visible = vis.cond();
}

void ogm::interpreter::fn::getv::background_visible(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = frame.m_background_layers.at(bg_id).m_visible;
}

void ogm::interpreter::fn::setv::background_index(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_background_index = vis.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::getv::background_index(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = static_cast<real_t>(frame.m_background_layers.at(bg_id).m_background_index);
}

void ogm::interpreter::fn::setv::background_x(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_position.x = vis.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::background_x(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = static_cast<real_t>(frame.m_background_layers.at(bg_id).m_position.x);
}

void ogm::interpreter::fn::setv::background_y(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_position.y = vis.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::background_y(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = static_cast<real_t>(frame.m_background_layers.at(bg_id).m_position.y);
}

void ogm::interpreter::fn::setv::background_hspeed(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_velocity.x = vis.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::background_hspeed(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = static_cast<real_t>(frame.m_background_layers.at(bg_id).m_velocity.x);
}

void ogm::interpreter::fn::setv::background_vspeed(VO out, OGM_2DARRAY_i V j, V vis)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    frame.m_background_layers.at(bg_id).m_velocity.y = vis.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::background_vspeed(VO out, OGM_2DARRAY_i V j)
{
    size_t bg_id = j.castCoerce<size_t>();
    if (bg_id >= frame.m_background_layers.size())
    {
        throw MiscError("Background index out-of-bounds.");
    }
    out = static_cast<real_t>(frame.m_background_layers.at(bg_id).m_velocity.y);
}
