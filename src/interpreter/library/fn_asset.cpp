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

void ogm::interpreter::fn::asset_get_type(VO out, V asset)
{
    asset_index_t index;
    Asset* a;
    if (asset.is_string())
    {
        a = frame.m_assets.get_asset(asset.castCoerce<std::string>().c_str(), index);
    }
    else
    {
        index = asset.castCoerce<asset_index_t>();
        a = frame.m_assets.get_asset<Asset*>(index);
     }

    // these correspond with the asset_* constants.
    if (dynamic_cast<AssetObject*>(a))
    {
        out = 1.0;
    }
    else if (dynamic_cast<AssetSprite*>(a))
    {
        out = 0.0;
    }
    else if (dynamic_cast<AssetBackground*>(a))
    {
        out = 2.0;
    }
    else if (dynamic_cast<AssetRoom*>(a))
    {
        out = 4.0;
    }
    else if (dynamic_cast<AssetFont*>(a))
    {
        out = 7.0;
    }
    else if (dynamic_cast<AssetScript*>(a))
    {
        out = 10.0;
    }
    else
    {
        out = -1.0;
    }
}

void ogm::interpreter::fn::asset_get_index(VO out, V a)
{
    std::string asset_name = a.castCoerce<std::string>();
    asset_index_t index;
    frame.m_assets.get_asset(asset_name.c_str(), index);

    if (index == k_no_asset)
    {
        out = -1.0;
    }
    else
    {
        out = static_cast<real_t>(index);
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
            ogm_assert(16 >= bytecode.m_retc);
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

namespace
{
    void add_font_from_sprite(VO out, V vsprite, V vprop, V vsep, std::vector<size_t>& indices)
    {
        asset_index_t asset_index;
        AssetFont* font = frame.m_assets.add_asset<AssetFont>("<dynamic font>", &asset_index);
        font->m_ttf = false;
        asset_index_t sprite_index;
        AssetSprite* sprite = frame.get_asset_from_variable<AssetSprite>(vsprite, sprite_index);

        // TODO: support proportional
        const bool monospace = true; //!vprop.cond();
        const int32_t sep = vsep.castCoerce<int32_t>();

        std::vector<TextureView*> sources;
        {
            for (size_t i = 0; i < indices.size(); ++i)
            {
                // TODO: push a cropped version of the texture view if not
                // monospaced.
                sources.push_back(
                    frame.m_display->m_textures.get_texture({ sprite_index, i })
                );
            }
        }

        std::vector<geometry::AABB<real_t>> uv;

        TexturePage* tpage = frame.m_display->m_textures.arrange_tpage(sources, uv, false);

        if (!tpage)
        {
            // TODO: either silently permit this, or report partial failures
            // (i.e. when not all glyphs were added.)
            throw MiscError("Could not create tpage for font.");
        }

        // bind font to tpage
        TextureView* view = frame.m_display->m_textures.bind_asset_to_tpage_location({ asset_index }, tpage);

        // set glyph information in font.
        for (size_t i = 0; i < indices.size(); ++i)
        {
            // Don't set information for glyphs which failed to add.
            if (uv.at(i).width() == 0) continue;
            Glyph& glyph = font->m_glyphs[indices.at(i)];
            glyph.m_position = view->uv_global_i(
                uv.at(i).m_start
            );
            glyph.m_dimensions = view->uv_global_i(
                uv.at(i).diagonal()
            );
            font->m_vshift = std::max(font->m_vshift, glyph.m_dimensions.y);
            glyph.m_shift = sep + glyph.m_dimensions.x;
            glyph.m_offset = 0;
        }

        out = asset_index;
    }
}

void ogm::interpreter::fn::font_add_sprite(VO out, V vsprite, V first, V prop, V sep)
{
    std::vector<size_t> indices;
    AssetSprite* sprite = frame.get_asset_from_variable<AssetSprite>(vsprite);
    size_t base = first.castCoerce<size_t>();
    for (size_t i = 0; i < sprite->m_subimage_count; ++i)
    {
        indices.push_back(base + i);
    }
    add_font_from_sprite(out, vsprite, prop, sep, indices);
}

void ogm::interpreter::fn::font_add_sprite_ext(VO out, V vsprite, V map, V prop, V sep)
{
    std::vector<size_t> indices;
    std::string s = map.castCoerce<std::string>();
    for (size_t i = 0; i < s.length(); ++i)
    {
        indices.push_back(s.at(i));
    }
    add_font_from_sprite(out, vsprite, prop, sep, indices);
}
