#include "libpre.h"
    #include "fn_asset.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "ogm/asset/Image.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::sprite_exists(VO out, V vs)
{
    asset_index_t index = vs.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetBackground*>(index);
}

void ogm::interpreter::fn::sprite_get_number(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->image_count());
}

void ogm::interpreter::fn::sprite_get_width(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_dimensions.x);
}

void ogm::interpreter::fn::sprite_get_height(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_dimensions.y);
}

void ogm::interpreter::fn::sprite_get_xoffset(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_offset.x);
}

void ogm::interpreter::fn::sprite_get_yoffset(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_offset.y);
}

void ogm::interpreter::fn::sprite_get_bbox_left(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_aabb.m_start.x);
}

void ogm::interpreter::fn::sprite_get_bbox_top(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_aabb.m_start.y);
}

void ogm::interpreter::fn::sprite_get_bbox_right(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_aabb.m_end.x);
}

void ogm::interpreter::fn::sprite_get_bbox_bottom(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_aabb.m_end.y);
}

void ogm::interpreter::fn::sprite_get_speed(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(s->m_speed);
}

void ogm::interpreter::fn::sprite_get_speed_type(VO out, V vs)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);
    out = static_cast<real_t>(
        s->m_speed_real_time
            ? constant::spritespeed_framespersecond
            : constant::spritespeed_framespergameframe
    );
}

void ogm::interpreter::fn::sprite_set_speed(VO out, V vs, V vspd, V vspdtype)
{
    AssetSprite* s = frame.get_asset_from_variable<AssetSprite>(vs);
    ogm_assert(s);

    s->m_speed = vspd.castCoerce<real_t>();
    s->m_speed_real_time = (vspdtype.castCoerce<real_t>() == constant::spritespeed_framespersecond);
}

void ogm::interpreter::fn::sprite_create_from_surface(VO out, V surface, V vx, V vy, V vw, V vh, V removebackground, V smooth, V xo, V yo)
{
    if (removebackground.cond() || smooth.cond())
    {
      throw MiscError("smooth or removebg not supported yet.");
    }

    asset_index_t asset_index;
    AssetSprite* sprite = frame.m_assets.add_asset<AssetSprite>("<dynamic sprite>", &asset_index);
    sprite->m_dimensions = {
      vw.castCoerce<coord_t>(),
      vh.castCoerce<coord_t>()
    };

    sprite->m_aabb = { { 0, 0 }, sprite->m_dimensions};
    sprite->m_offset = {
      xo.castCoerce<coord_t>(),
      yo.castCoerce<coord_t>()
    };
    sprite->m_subimage_count = 1;

    geometry::Vector<coord_t> src_coord = { vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>()};

    surface_id_t si = surface.castCoerce<uint32_t>();
    TextureView* view = frame.m_display->m_textures.get_surface_texture_view(si);
    frame.m_display->m_textures.bind_asset_copy_texture(
        { asset_index, 0 },
        view,
        geometry::AABB<coord_t>{ src_coord, sprite->m_dimensions + src_coord }
    );
    
    out = static_cast<real_t>(asset_index);
}

void ogm::interpreter::fn::sprite_add(VO out, V fname, V vimgnum, V removeback, V smooth, V xorig, V yorig)
{
    if (removeback.cond() || smooth.cond())
    {
        throw MiscError("smooth or removebg not supported yet.");
    }

    size_t imgnum = std::max(1, vimgnum.castCoerce<int32_t>());
    
    asset_index_t index;
    AssetSprite* sprite = frame.m_assets.add_asset<AssetSprite>(
        "<dynamic sprite>", &index
    );
    
    sprite->m_offset = {
        xorig.castCoerce<coord_t>(),
        yorig.castCoerce<coord_t>()
    };
    sprite->m_shape = AssetSprite::rectangle;
    sprite->m_subimage_count = imgnum;
    
    // load image (slight hack: store in the subimage vector, but it is actually
    // the whole image.)
    asset::Image& image = sprite->m_subimages.emplace_back(
        frame.m_fs.resolve_file_path(fname.castCoerce<std::string>())
    );
    image.realize_data();
    
    TexturePage* tpage = frame.m_display->m_textures.create_tpage_from_callback(
        [sprite]() -> asset::Image* {
            return &sprite->m_subimages.front();
        }
    );
    
    // split image
    sprite->m_dimensions = geometry::Vector<int32_t>{
        static_cast<int32_t>(image.m_dimensions.x / imgnum),
        image.m_dimensions.y
    };
    
    for (size_t i = 0; i < imgnum; ++i)
    {
        int32_t start = i * sprite->m_dimensions.x;
        coord_t invxrange = 1.0 / static_cast<coord_t>(image.m_dimensions.x);
        frame.m_display->m_textures.bind_asset_to_tpage_location(
            { index, i },
            tpage,
            {
                invxrange * start, 0,
                invxrange * (start + sprite->m_dimensions.x), 1
            }
        );
    }
    
    out = static_cast<real_t>(index);
}

void ogm::interpreter::fn::sprite_add_from_surface(VO out, V index, V surface, V vx, V vy, V vw, V vh, V removebackground, V smooth)
{
    if (removebackground.cond() || smooth.cond())
    {
      throw MiscError("smooth or removebg not supported yet.");
    }

    asset_index_t asset_index = index.castCoerce<asset_index_t>();
    AssetSprite* sprite = frame.m_assets.get_asset<AssetSprite*>(asset_index);
    geometry::Vector<coord_t> dim = {
        vw.castCoerce<coord_t>(),
        vh.castCoerce<coord_t>()
    };

    sprite->m_dimensions.x = std::max(sprite->m_dimensions.x, dim.x);
    sprite->m_dimensions.y = std::max(sprite->m_dimensions.y, dim.y);

    geometry::Vector<coord_t> src_coord = { vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>()};

    surface_id_t si = surface.castCoerce<uint32_t>();
    TextureView* view = frame.m_display->m_textures.get_surface_texture_view(si);
    frame.m_display->m_textures.bind_asset_copy_texture(
        { asset_index, sprite->m_subimage_count++ },
        view,
        geometry::AABB<coord_t>{ src_coord, dim + src_coord }
    );
}
