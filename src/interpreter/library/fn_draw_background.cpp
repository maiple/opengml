#include "libpre.h"
    #include "fn_draw.h"
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
#define display frame.m_display

void ogm::interpreter::fn::draw_background(VO out, V background, V x, V y)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1);
        display->set_colour(0xffffff);
        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
            ),
            0,
            0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_background_part(VO out, V background, V left, V top, V width, V height, V x, V y)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

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
            frame.m_display->m_textures.get_texture(
                {asset_index}
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

void ogm::interpreter::fn::draw_background_part_ext(VO out, V background, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

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
                {asset_index}
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

void ogm::interpreter::fn::draw_background_general(VO out, V background, V left, V top, V width, V height, V x, V y, V xscale, V yscale, V c1, V c2, V c3, V c4, V alpha)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

        coord_t texw = asset->m_dimensions.x;
        coord_t texh = asset->m_dimensions.y;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
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
                {asset_index}
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

void ogm::interpreter::fn::draw_background_ext(VO out, V background, V x, V y, V xscale, V yscale, V angle, V c, V alpha)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>(), angle.castCoerce<real_t>() * TAU / 360);
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
            ),
            0, 0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_background_ext(VO out, V background, V x, V y, V xscale, V yscale, V c, V alpha)
{
    {
        asset_index_t asset_index = background.castCoerce<asset_index_t>();
        AssetBackground* asset = frame.m_assets.get_asset<AssetBackground*>(asset_index);
        if (!asset) return;

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>(), xscale.castCoerce<real_t>(), yscale.castCoerce<real_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
            ),
            0, 0,
            asset->m_dimensions.x,
            asset->m_dimensions.y
        );
        display->set_alpha(prev_alpha);
        display->set_colour(prev_colour);
        display->set_matrix_pre_model();
    }
}

void ogm::interpreter::fn::draw_background_stretched_ext(VO out, V background, V x, V y, V w, V h, V c, V alpha)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(c.castCoerce<int32_t>());

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
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

void ogm::interpreter::fn::draw_background_stretched(VO out, V background, V x, V y, V w, V h)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_pre_model(x.castCoerce<coord_t>(), y.castCoerce<coord_t>());
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(1.0);
        display->set_colour(0xffffff);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
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

void ogm::interpreter::fn::draw_background_pos(VO out, V background, V x1, V y1, V x2, V y2, V x3, V y3, V x4, V y4, V alpha)
{
    {
        asset_index_t asset_index;
        frame.get_asset_from_variable<AssetBackground>(background, asset_index);

        display->set_matrix_pre_model();
        float prev_alpha = display->get_alpha();
        float prev_colour = display->get_colour();
        display->set_alpha(alpha.castCoerce<real_t>());
        display->set_colour(0xffffff);

        display->draw_image(
            frame.m_display->m_textures.get_texture(
                {asset_index}
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
