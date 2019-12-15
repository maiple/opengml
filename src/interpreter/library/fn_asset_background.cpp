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

void ogm::interpreter::fn::background_exists(VO out, V bg)
{
    asset_index_t index = bg.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetBackground*>(index);
}

void ogm::interpreter::fn::background_get_width(VO out, V vb)
{
    AssetBackground* bg = frame.m_assets.get_asset<AssetBackground*>(vb.castCoerce<asset_index_t>());
    if (bg)
    {
        out = static_cast<real_t>(bg->m_dimensions.x);
    }
    else
    {
        out = 0.0;
    }
}

void ogm::interpreter::fn::background_get_height(VO out, V vb)
{
    AssetBackground* bg = frame.m_assets.get_asset<AssetBackground*>(vb.castCoerce<asset_index_t>());
    if (bg)
    {
        out = static_cast<real_t>(bg->m_dimensions.y);
    }
    else
    {
        out = 0.0;
    }
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
        out = "<undefined>";
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
    out = static_cast<real_t>(asset_index);
    frame.m_display->m_textures.bind_asset_to_callback(
        asset_index,
        [bg]() { return &bg->m_image; }
    );
}
