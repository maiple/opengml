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
#define display frame.m_display

namespace
{
    void get_uvs(VO out, TextureView* texture)
    {
        // initialize
        out.array_get(0, 7) = 0;

        // fill array
        out.array_get(0, 0) = texture->m_uv.left();
        out.array_get(0, 1) = texture->m_uv.top();
        out.array_get(0, 2) = texture->m_uv.right();
        out.array_get(0, 3) = texture->m_uv.bottom();

        // TODO: fix these placeholder values
        out.array_get(0, 4) = 0;
        out.array_get(0, 5) = 0;
        out.array_get(0, 6) = 1;
        out.array_get(0, 7) = 1;
    }
}

void ogm::interpreter::fn::sprite_get_texture(VO out, V asset, V subimage)
{
    out = static_cast<void*>(display->m_textures.get_texture(
        {asset.castCoerce<asset_index_t>(), subimage.castCoerce<size_t>()}
    ));
}

void ogm::interpreter::fn::background_get_texture(VO out, V asset)
{
    out = static_cast<void*>(display->m_textures.get_texture(
        {asset.castCoerce<asset_index_t>()}
    ));
}

void ogm::interpreter::fn::texture_get_width(VO out, V tex)
{
    TextureView* texture = static_cast<TextureView*>(tex.castExact<void*>());
    out = static_cast<real_t>(texture->m_uv.width());
}

void ogm::interpreter::fn::texture_get_height(VO out, V tex)
{
    TextureView* texture = static_cast<TextureView*>(tex.castExact<void*>());
    out = static_cast<real_t>(texture->m_uv.height());
}

void ogm::interpreter::fn::texture_get_texel_width(VO out, V tex)
{
    TextureView* texture = static_cast<TextureView*>(tex.castExact<void*>());
    out = 1.0/(texture->m_tpage->m_dimensions.x);
}

void ogm::interpreter::fn::texture_get_texel_height(VO out, V tex)
{
    TextureView* texture = static_cast<TextureView*>(tex.castExact<void*>());
    out = 1.0/(texture->m_tpage->m_dimensions.y);
}

void ogm::interpreter::fn::texture_get_uvs(VO out, V tex)
{
    TextureView* texture = static_cast<TextureView*>(tex.castExact<void*>());
    get_uvs(out, texture);
}

void ogm::interpreter::fn::sprite_get_uvs(VO out, V asset, V subimage)
{
    get_uvs(out,
        display->m_textures.get_texture(
            {asset.castCoerce<asset_index_t>(), subimage.castCoerce<size_t>()}
        )
    );
}

void ogm::interpreter::fn::background_get_uvs(VO out, V asset)
{
    get_uvs(out,
        display->m_textures.get_texture(
            {asset.castCoerce<asset_index_t>()}
        )
    );
}
