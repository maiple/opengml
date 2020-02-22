#include "libpre.h"
    #include "fn_draw.h"
    #include "fn_string.h"
#include "libpost.h"

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
#define display frame.m_display

void ogm::interpreter::fn::draw_rectangle(VO out, V x1, V y1, V x2, V y2, V outline)
{
    display->set_matrix_pre_model();
    if (outline.cond())
    {
        display->draw_outline_rectangle(
            x1.castCoerce<real_t>(),
            y1.castCoerce<real_t>(),
            x2.castCoerce<real_t>(),
            y2.castCoerce<real_t>()
        );
    }
    else
    {
        display->draw_filled_rectangle(
            x1.castCoerce<real_t>(),
            y1.castCoerce<real_t>(),
            x2.castCoerce<real_t>(),
            y2.castCoerce<real_t>()
        );
    }
}

void ogm::interpreter::fn::draw_set_circle_precision(VO out, V p)
{
    display->set_circle_precision(
        p.castCoerce<int32_t>()
    );
}

void ogm::interpreter::fn::draw_circle(VO out, V x, V y, V r, V outline)
{
    display->set_matrix_pre_model();
    if (outline.cond())
    {
        display->draw_outline_circle(
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            r.castCoerce<real_t>()
        );
    }
    else
    {
        display->draw_filled_circle(
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            r.castCoerce<real_t>()
        );
    }
}

void ogm::interpreter::fn::draw_circle_colour(VO out, V x, V y, V r, V c1, V c2, V outline)
{
    display->set_matrix_pre_model();
    // set colours
    uint32_t previous_colours[4];
    display->get_colours4(previous_colours);
    uint32_t ualpha = display->get_alpha() * 255;
    uint32_t cols[4] = { (c2.castCoerce<int32_t>() << 8) | ualpha, (c1.castCoerce<int32_t>() << 8) | ualpha, 0, 0 };
    display->set_colours4(cols);

    // draw circle
    if (outline.cond())
    {
        display->draw_outline_circle(
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            r.castCoerce<real_t>()
        );
    }
    else
    {
        // draw circle
        display->draw_filled_circle(
            x.castCoerce<real_t>(),
            y.castCoerce<real_t>(),
            r.castCoerce<real_t>()
        );
    }
    display->set_colours4(previous_colours);
}

void ogm::interpreter::fn::draw_sprite(VO out, V sprite, V image, V x, V y)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1);
        display->set_colour(0xffffff);
        display->draw_image(
            frame.m_display->m_textures.get_texture({asset_index, image.castCoerce<size_t>() % asset->image_count()}),
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x-asset->m_offset.x,
            asset->m_dimensions.y-asset->m_offset.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_part(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            frame.m_display->m_textures.get_texture({asset_index, image.castCoerce<size_t>() % asset->image_count()}),
            0,
            0,
            c_width,
            c_height,
            c_left / texw,
            c_top / texh,
            (c_left + c_width) / texw,
            (c_top + c_height) / texh
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_part_ext(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            0,
            0,
            c_width,
            c_height,
            c_left / texw,
            c_top / texh,
            (c_left + c_width) / texw,
            (c_top + c_height) / texh
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_general(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V rot, V c1, V c2, V c3, V c4, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), rot.castCoerce<real_t>() * TAU / 360);
        uint32_t previous_colours[4];
        display->get_colours4(previous_colours);
        uint32_t ualpha = alpha.castCoerce<real_t>() * 255;
        uint32_t cols[4] = { (c1.castCoerce<int32_t>() << 8) | ualpha, (c2.castCoerce<int32_t>() << 8) | ualpha, (c3.castCoerce<int32_t>() << 8) | ualpha, (c4.castCoerce<int32_t>()<<8) | ualpha };
        display->set_colours4(cols);

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            0,
            0,
            c_width,
            c_height,
            c_left / texw,
            c_top / texh,
            (c_left + c_width) / texw,
            (c_top + c_height) / texh
        );
        display->set_colours4(previous_colours);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_ext(VO out, V sprite, V image, V x, V y, V xscale, V yscale, V angle, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), angle.castCoerce<real_t>() * TAU / 360);
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x - asset->m_offset.x,
            asset->m_dimensions.y - asset->m_offset.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_stretched_ext(VO out, V sprite, V image, V x, V y, V w, V h, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_stretched(VO out, V sprite, V image, V x, V y, V w, V h)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_sprite_pos(VO out, V sprite, V image, V x1, V y1, V x2, V y2, V x3, V y3, V x4, V y4, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_pre_model();
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(0xffffff);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, image.castCoerce<size_t>() % asset->image_count()}
            ),
            x1.castCoerce<real_t>(),
            y1.castCoerce<real_t>(),
            x2.castCoerce<real_t>(),
            y2.castCoerce<real_t>(),
            x3.castCoerce<real_t>(),
            y3.castCoerce<real_t>(),
            x4.castCoerce<real_t>(),
            y4.castCoerce<real_t>(),
            0, 0,
            0, 1,
            1, 1,
            1, 0
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_self(VO out)
{
    const Instance* self = staticExecutor.m_self;
    asset_index_t asset_index = self->m_data.m_sprite_index;
    AssetSprite* asset = frame.m_assets.get_asset<AssetSprite*>(asset_index);
    if (asset)
    {
        display->set_matrix_pre_model(std::floor(self->m_data.m_position.x), std::floor(self->m_data.m_position.y), self->m_data.m_scale.x, self->m_data.m_scale.y, self->m_data.m_angle * TAU / 360);
        uint32_t previous_colours[4];
        display->get_colours4(previous_colours);

        display->set_colour(self->m_data.m_image_blend);
        display->set_alpha(self->m_data.m_image_alpha);
        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index, static_cast<size_t>(self->m_data.m_image_index) % asset->image_count()}
            ),
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x-asset->m_offset.x,
            asset->m_dimensions.y-asset->m_offset.y
        );

        display->set_colours4(previous_colours);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_set_colour(VO out, V colour)
{
    display->set_colour(colour.castCoerce<uint32_t>());
}

void ogm::interpreter::fn::draw_set_alpha(VO out, V a)
{
    display->set_alpha(a.castCoerce<real_t>());
}

void ogm::interpreter::fn::draw_get_colour(VO out)
{
    out = static_cast<real_t>(display->get_colour());
}

void ogm::interpreter::fn::draw_get_alpha(VO out)
{
    out = static_cast<real_t>(display->get_alpha());
}

void ogm::interpreter::fn::draw_clear(VO out, V c)
{
    int32_t col = c.castCoerce<int32_t>();
    col <<= 8;
    col |= 0xff;
    display->draw_fill_colour(col);
}

void ogm::interpreter::fn::draw_clear_alpha(VO out, V c, V a)
{
    int32_t col = c.castCoerce<int32_t>();
    col <<= 8;
    col |= (static_cast<uint32_t>(a.castCoerce<real_t>() * 255) & 0xff);
    display->draw_fill_colour(col);
}

void ogm::interpreter::fn::draw_enable_alphablend(VO out, V a)
{
    // TODO
}

namespace
{
    // 0: left. 0.5: center. 1: right.
    int32_t g_halign = 0;
    int32_t g_valign = 0;
}

void ogm::interpreter::fn::draw_set_halign(VO out, V a)
{
    g_halign = a.castCoerce<int32_t>();
    if (g_halign < 0 || g_halign > 2)
    {
        throw MiscError("invalid alignment.");
    }
}

void ogm::interpreter::fn::draw_set_valign(VO out, V a)
{
    g_valign = a.castCoerce<int32_t>();
    if (g_valign < 0 || g_valign > 2)
    {
        throw MiscError("invalid alignment.");
    }
}

void ogm::interpreter::fn::draw_set_font(VO out, V font)
{
    if (font.castCoerce<int32_t>() == -1)
    {
        display->set_font(nullptr);
        return;
    }

    asset_index_t af_index;
    AssetFont* af = frame.get_asset_from_variable<AssetFont>(font, af_index);
    TextureView* tv = nullptr;

    // get image for font, if font is a raster font.
    if (!af->m_ttf)
    {
        tv = display->m_textures.get_texture({ af_index });
    }

    display->set_font(af, tv);
}

void ogm::interpreter::fn::draw_text(VO out, V x, V y, V vtext)
{
    display->set_matrix_pre_model();
    Variable _vtext;
    string(_vtext, vtext);
    std::string text = _vtext.castCoerce<std::string>().c_str();
    text = replace_all(text, "#", "\n");
    display->draw_text(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), text.c_str(), g_halign / 2.0, g_valign / 2.0);
}

void ogm::interpreter::fn::draw_text_ext(VO out, V x, V y, V vtext, V sep, V w)
{
    display->set_matrix_pre_model();
    Variable _vtext;
    string(_vtext, vtext);
    std::string text = _vtext.castCoerce<std::string>().c_str();
    text = replace_all(text, "#", "\n");
    display->draw_text(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), text.c_str(), g_halign / 2.0, g_valign / 2.0, true, w.castCoerce<coord_t>(), true, sep.castCoerce<coord_t>());
}

void ogm::interpreter::fn::draw_text_colour(VO out, V x, V y, V vtext, V c1, V c2, V c3, V c4, V alpha)
{
    display->set_matrix_pre_model();
    Variable _vtext;
    string(_vtext, vtext);
    std::string text = _vtext.castCoerce<std::string>().c_str();
    text = replace_all(text, "#", "\n");

    uint32_t previous_colours[4];
    display->get_colours4(previous_colours);
    uint32_t ualpha = alpha.castCoerce<real_t>() * 255;
    uint32_t cols[4] = { (c1.castCoerce<int32_t>() << 8) | ualpha, (c2.castCoerce<int32_t>() << 8) | ualpha, (c3.castCoerce<int32_t>() << 8) | ualpha, (c4.castCoerce<int32_t>()<<8) | ualpha };
    display->set_colours4(cols);

    display->draw_text(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), text.c_str(), g_halign / 2.0, g_valign / 2.0);

    display->set_colours4(previous_colours);
}

void ogm::interpreter::fn::draw_text_ext_colour(VO out, V x, V y, V vtext, V sep, V w, V c1, V c2, V c3, V c4, V alpha)
{
    display->set_matrix_pre_model();
    Variable _vtext;
    string(_vtext, vtext);
    std::string text = _vtext.castCoerce<std::string>().c_str();
    text = replace_all(text, "#", "\n");

    uint32_t previous_colours[4];
    display->get_colours4(previous_colours);
    uint32_t ualpha = alpha.castCoerce<real_t>() * 255;
    uint32_t cols[4] = { (c1.castCoerce<int32_t>() << 8) | ualpha, (c2.castCoerce<int32_t>() << 8) | ualpha, (c3.castCoerce<int32_t>() << 8) | ualpha, (c4.castCoerce<int32_t>()<<8) | ualpha };
    display->set_colours4(cols);

    display->draw_text(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), text.c_str(), g_halign / 2.0, g_valign / 2.0, true, w.castCoerce<coord_t>(), true, sep.castCoerce<coord_t>());

    display->set_colours4(previous_colours);
}

void ogm::interpreter::fn::draw_set_colour_write_enable(VO out, V r, V g, V b, V a)
{
    display->set_colour_mask(r.cond(), g.cond(), b.cond(), a.cond());
}

void ogm::interpreter::fn::draw_set_blend_mode(VO out, V bm)
{
    switch (bm.castCoerce<int32_t>())
    {
    // see fn_draw.h constants
    case 0:
    default:
        display->set_blendmode(8, 9);
        break;
    case 1:
        display->set_blendmode(8, 5);
        break;
    case 2:
        display->set_blendmode(4, 7);
        break;
    case 3:
        display->set_blendmode(8, 7);
        break;
    }
}

void ogm::interpreter::fn::draw_set_blend_mode_ext(VO out, V src, V dst)
{
    display->set_blendmode(src.castCoerce<int32_t>(), dst.castCoerce<int32_t>());
}

void ogm::interpreter::fn::draw_set_alpha_test(VO out, V enabled)
{
    display->shader_set_alpha_test_enabled(enabled.cond());
}

void ogm::interpreter::fn::draw_set_alpha_test_ref_value(VO out, V value)
{
    display->shader_set_alpha_test_threshold(value.castCoerce<real_t>());
}

void ogm::interpreter::fn::texture_set_blending(VO out, V c)
{
    display->set_blending_enabled(c.cond());
}

void ogm::interpreter::fn::ogm_gpu_disable_scissor(VO out)
{
    display->disable_scissor();
}

void ogm::interpreter::fn::ogm_gpu_enable_scissor(VO out, V x1, V y1, V x2, V y2)
{
    display->enable_scissor(
        x1.castCoerce<real_t>(),
        y1.castCoerce<real_t>(),
        x2.castCoerce<real_t>(),
        y2.castCoerce<real_t>()
    );
}
