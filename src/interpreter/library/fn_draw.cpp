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
#define display frame.m_display

void ogm::interpreter::fn::draw_rectangle(VO out, V x1, V y1, V x2, V y2, V outline)
{
    display->set_matrix_model();
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
    display->set_matrix_model();
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
    display->set_matrix_model();
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

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1);
        display->set_colour(0xffffff);
        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x-asset->m_offset.x,
            asset->m_dimensions.y-asset->m_offset.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_sprite_part(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
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
    }
}

void ogm::interpreter::fn::draw_sprite_part_ext(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
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
    }
}

void ogm::interpreter::fn::draw_sprite_general(VO out, V sprite, V image, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V rot, V c1, V c2, V c3, V c4, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), rot.castCoerce<real_t>() * TAU / 360);
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
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
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
    }
}

void ogm::interpreter::fn::draw_sprite_ext(VO out, V sprite, V image, V x, V y, V xscale, V yscale, V angle, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), angle.castCoerce<real_t>() * TAU / 360);
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x - asset->m_offset.x,
            asset->m_dimensions.y - asset->m_offset.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_sprite_stretched_ext(VO out, V sprite, V image, V x, V y, V w, V h, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_sprite_stretched(VO out, V sprite, V image, V x, V y, V w, V h)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_sprite_pos(VO out, V sprite, V image, V x1, V y1, V x2, V y2, V x3, V y3, V x4, V y4, V alpha)
{
    {
        asset_index_t asset_index;
        AssetSprite* asset = frame.get_asset_from_variable<AssetSprite>(sprite, asset_index);

        display->set_matrix_model();
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(0xffffff);

        display->draw_image(
            {asset_index, image.castCoerce<size_t>() % asset->image_count()},
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
        display->set_matrix_model(std::floor(self->m_data.m_position.x), std::floor(self->m_data.m_position.y), self->m_data.m_scale.x, self->m_data.m_scale.y, self->m_data.m_angle * TAU / 360);
        uint32_t previous_colours[4];
        display->get_colours4(previous_colours);

        display->set_colour(self->m_data.m_image_blend);
        display->set_alpha(self->m_data.m_image_alpha);
        display->draw_image(
            {asset_index, static_cast<size_t>(self->m_data.m_image_index) % asset->image_count()},
            -asset->m_offset.x,
            -asset->m_offset.y,
            asset->m_dimensions.x-asset->m_offset.x,
            asset->m_dimensions.y-asset->m_offset.y
        );

        display->set_colours4(previous_colours);
    }
}

void ogm::interpreter::fn::draw_background(VO out, V background, V x, V y)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1);
        display->set_colour(0xffffff);
        display->draw_image(
            {asset_index},
            0,
            0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_background_part(VO out, V background, V left, V top, V width, V height, V x, V y)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            {asset_index},
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
    }
}

void ogm::interpreter::fn::draw_background_part_ext(VO out, V background, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        coord_t c_left = std::max(left.castCoerce<coord_t>(), 0.0);
        coord_t c_top = std::max(top.castCoerce<coord_t>(), 0.0);
        coord_t c_width = std::min(width.castCoerce<coord_t>(), asset->m_dimensions.x - c_left);
        coord_t c_height = std::min(height.castCoerce<coord_t>(), asset->m_dimensions.y - c_top);

        display->draw_image(
            {asset_index},
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
    }
}

void ogm::interpreter::fn::draw_background_general(VO out, V background, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c1, V c2, V c3, V c4, V alpha)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);
        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
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
            {asset_index},
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
    }
}

void ogm::interpreter::fn::draw_background_ext(VO out, V background, V x, V y, V xscale, V yscale, V angle, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), angle.castCoerce<real_t>() * TAU / 360);
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            {asset_index},
            0, 0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_background_ext(VO out, V background, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index;
        AssetBackground* asset = frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            {asset_index},
            0, 0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_background_stretched_ext(VO out, V background, V x, V y, V w, V h, V c, V alpha)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            {asset_index},
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_background_stretched(VO out, V background, V x, V y, V w, V h)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        display->draw_image(
            {asset_index},
            0,
            0,
            w.castCoerce<real_t>(),
            h.castCoerce<real_t>()
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
    }
}

void ogm::interpreter::fn::draw_background_pos(VO out, V background, V x1, V y1, V x2, V y2, V x3, V y3, V x4, V y4, V alpha)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_model();
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(0xffffff);

        display->draw_image(
            {asset_index},
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
void ogm::interpreter::fn::draw_set_colour(VO out, V colour)
{
    display->set_colour(colour.castCoerce<uint32_t>());
}

void ogm::interpreter::fn::draw_set_alpha(VO out, V a)
{
    display->set_colour(a.castCoerce<real_t>());
}

void ogm::interpreter::fn::draw_get_colour(VO out)
{
    out = display->get_colour();
}

void ogm::interpreter::fn::draw_get_alpha(VO out)
{
    out = display->get_alpha();
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
    col |= (a.castCoerce<int32_t>() & 0xff);
    display->draw_fill_colour(col);
}

void ogm::interpreter::fn::draw_enable_alphablend(VO out, V a)
{
    // TODO
}

void ogm::interpreter::fn::draw_set_halign(VO out, V a)
{
    // TODO
}

void ogm::interpreter::fn::draw_set_valign(VO out, V a)
{
    // TODO
}

void ogm::interpreter::fn::draw_text(VO out, V x, V y, V text)
{
    // TODO
}
