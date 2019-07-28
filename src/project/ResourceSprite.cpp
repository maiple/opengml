#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>
#include <cstring>

namespace ogm { namespace project {

ResourceSprite::ResourceSprite(const char* path, const char* name): m_path(path), m_name(name)
{ }


void ResourceSprite::precompile(bytecode::ProjectAccumulator& acc)
{
    const std::string sprite_name = m_name;
    m_sprite_asset = acc.m_assets->add_asset<asset::AssetSprite>(sprite_name.c_str());
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    if (!result)
    {
        throw MiscError("Sprite file not found: " + _path);
    }

    pugi::xml_node node = doc.child("sprite");
    pugi::xml_node node_frames = node.child("frames");
    for (pugi::xml_node frame: node_frames.children("frame"))
    {
        std::string frame_index = frame.attribute("index").value();
        size_t index = stoi(frame_index);
        // path to subimage
        std::string path = case_insensitive_native_path(path_directory(_path), frame.text().get());
        if (m_sprite_asset->m_subimage_paths.size() <= index)
        {
            m_sprite_asset->m_subimage_paths.resize(index + 1);
        }
        m_sprite_asset->m_subimage_paths[index] = path;
    }

    const char* node_w = node.child("width").text().get();
    const char* node_h = node.child("height").text().get();
    const char* node_x = node.child("xorig").text().get();
    const char* node_y = node.child("yorigin").text().get();

    const char* bbox_left = node.child("bbox_left").text().get();
    const char* bbox_right = node.child("bbox_right").text().get();
    const char* bbox_top = node.child("bbox_top").text().get();
    const char* bbox_bottom = node.child("bbox_bottom").text().get();

    m_sprite_asset->m_offset = { static_cast<coord_t>(atoi(node_x)), static_cast<coord_t>(atoi(node_y)) };
    m_sprite_asset->m_dimensions = { static_cast<coord_t>(atoi(node_w)), static_cast<coord_t>(atoi(node_h)) };
    m_sprite_asset->m_aabb = { { static_cast<coord_t>(atof(bbox_left)), static_cast<coord_t>(atof(bbox_top)) },
        { static_cast<coord_t>(atof(bbox_right)+ 1.0), static_cast<coord_t>(atof(bbox_bottom) + 1.0) } };
    m_sprite_asset->m_shape = collision::Shape::rectangle;
}
}}
