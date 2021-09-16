#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"
#include "XMLError.hpp"

#include <stb_image.h>
#include <pugixml.hpp>

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
        throw ResourceError(1026, this, "Unrecognized file extension for sprite file \"{}\"", m_path);
    }
}

void ResourceSprite::load_file_xml()
{
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    check_xml_result<ResourceError>(1064, result, _path.c_str(), "Sprite file not found: " + _path, this);

    pugi::xml_node node = doc.child("sprite");
    pugi::xml_node node_frames = node.child("frames");
    for (pugi::xml_node frame: node_frames.children("frame"))
    {
        std::string frame_index = frame.attribute("index").value();
        size_t index = stoi(frame_index);

        // path to subimage
        bool casechange = false;
        std::string path = case_insensitive_native_path(path_directory(_path), frame.text().get(), &casechange);
        if (!path_exists(path)) throw ResourceError(1056, this, "Failed to find resource at path \"{}\"", path);
        if (casechange) std::cout << "WARNING: path \""<< path_directory(_path) << frame.text().get() << "\" required case-insensitive lookup:\n  Became \"" << path << "\"\n  This is time-consuming and should be corrected.\n";
        if (m_subimages.size() <= index)
        {
            m_subimages.resize(index + 1);
        }
        m_subimages[index].m_path = path;
    }

    const char* node_w = node.child("width").text().get();
    const char* node_h = node.child("height").text().get();
    const char* node_x = node.child("xorig").text().get();
    const char* node_y = node.child("yorigin").text().get();

    const char* bbox_left = node.child("bbox_left").text().get();
    const char* bbox_right = node.child("bbox_right").text().get();
    const char* bbox_top = node.child("bbox_top").text().get();
    const char* bbox_bottom = node.child("bbox_bottom").text().get();

    m_colkind = atoi(node.child("colkind").text().get());
    m_alpha_tolerance = atoi(node.child("coltolerance").text().get());
    m_separate_collision_masks = node.child("sepmasks").text().get() != std::string("0");
    m_bboxmode = atoi(node.child("bboxmode").text().get());

    m_offset = { static_cast<coord_t>(atoi(node_x)), static_cast<coord_t>(atoi(node_y)) };
    m_dimensions = { static_cast<coord_t>(atoi(node_w)), static_cast<coord_t>(atoi(node_h)) };
    m_aabb = {
        { static_cast<coord_t>(atof(bbox_left)), static_cast<coord_t>(atof(bbox_top)) },
        { static_cast<coord_t>(atof(bbox_right)+ 1.0), static_cast<coord_t>(atof(bbox_bottom) + 1.0) }
    };
}

void ResourceSprite::precompile(bytecode::ProjectAccumulator& acc)
{
    const std::string& sprite_name = m_name;
    m_sprite_asset = acc.m_assets->add_asset<asset::AssetSprite>(sprite_name.c_str());

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
        throw ResourceError(1057, this, "unknown collision shape");
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
