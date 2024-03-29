#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "project/XMLError.hpp"

#include <stb_image.h>

#include <string>
#include <cstring>

namespace ogm::project {

ResourceSprite::ResourceSprite(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
{ }

void ResourceSprite::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    if (ends_with(m_path, ".gmx"))
    {
        load_file_xml();
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        load_file_arf();
    }
    else if (ends_with(m_path, ".yy"))
    {
        load_file_json();
    }
    else
    {
        throw ResourceError(ErrorCode::F::unkresext, this, "Unrecognized file extension for sprite file \"{}\"", m_path);
    }
}

void ResourceSprite::assign_id(bytecode::ProjectAccumulator& acc)
{
    Resource::assign_id(acc);
    m_sprite_asset = acc.m_assets->add_asset<asset::AssetSprite>(m_name.c_str());
}

void ResourceSprite::precompile(bytecode::ProjectAccumulator& acc)
{
    const std::string& sprite_name = m_name;

    m_sprite_asset->m_offset = m_offset;
    m_sprite_asset->m_dimensions = m_dimensions;
    m_sprite_asset->m_aabb = m_aabb;
    m_sprite_asset->m_speed = m_speed;
    m_sprite_asset->m_speed_real_time = m_speed_real_time;

    // we will assign subimage data during compilation proper.

    #ifdef ONLY_RECTANGULAR_COLLISION
    m_sprite_asset->m_shape = asset::AssetSprite::rectangle;
    #else
    switch (m_colkind)
    {
    case 0:
        m_sprite_asset->m_shape = asset::AssetSprite::raster;
        break;
    case 1:
        m_sprite_asset->m_shape = asset::AssetSprite::rectangle;
        break;
    case 2:
        m_sprite_asset->m_shape = asset::AssetSprite::ellipse;
        break;
    case 3:
        m_sprite_asset->m_shape = asset::AssetSprite::diamond;
        break;
    default:
        throw ResourceError(ErrorCode::F::colshape, this, "unknown collision shape");
    }
    #endif
}

// adds a raster image to the image data, filled with zeros.
void ResourceSprite::addRaster()
{
    m_sprite_asset->m_raster.emplace_back();
    m_sprite_asset->m_raster.back().m_width = m_sprite_asset->m_dimensions.x;
    const size_t length = m_sprite_asset->m_dimensions.x * m_sprite_asset->m_dimensions.y;
    m_sprite_asset->m_raster.back().m_length = length;
    m_sprite_asset->m_raster.back().m_data = new bool[length];
    for (size_t i = 0; i < length; ++i)
    {
        m_sprite_asset->m_raster.back().m_data[i] = 0;
    }
}

void ResourceSprite::compile(bytecode::ProjectAccumulator&)
{
    if (mark_progress(COMPILED)) return;
    if (m_sprite_asset->m_shape == asset::AssetSprite::raster)
    {
        // precise collision data requires realizing the images.
        if (!m_separate_collision_masks)
        // just add one raster, containing the union of
        // collision data over all frames.
        {
            addRaster();
        }

        for (asset::AssetSprite::SubImage& image : m_subimages)
        {
            if (m_separate_collision_masks)
            // one raster for each frame.
            {
                addRaster();
            }
            collision::CollisionRaster& raster = m_sprite_asset->m_raster.back();
            int channel_count = 4;

            image.realize_data();

            size_t i = 0;
            for (
                uint8_t* c = image.m_data + 3;
                c < image.m_data + image.get_data_len();
                c += channel_count, ++i
            )
            {
                if (*c > m_alpha_tolerance)
                {
                    raster.set(i, true);
                }
            }
        }
    }

    // copy subimage data to sprite.
    m_sprite_asset->m_subimages = m_subimages;
    m_sprite_asset->m_subimage_count = m_subimages.size();
}

}
