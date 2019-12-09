#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"

#include <stb_image.h>
#include <pugixml.hpp>

#include <string>
#include <cstring>

namespace ogm { namespace project {

ResourceSprite::ResourceSprite(const char* path, const char* name): m_path(path), m_name(name)
{ }

void ResourceSprite::load_file()
{
    if (ends_with(m_path, ".gmx"))
    {
        load_file_xml();
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        load_file_arf();
    }
    else
    {
        throw MiscError("Unrecognized file extension for sprite file " + m_path);
    }
}

namespace
{
    ARFSchema arf_sprite_schema
    {
        "sprite",
        ARFSchema::DICT,
        {
            "subimages",
            ARFSchema::LIST
        }
    };
}

void ResourceSprite::load_file_arf()
{
    std::string _path = native_path(m_path);
    std::string _dir = path_directory(_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection sprite_section;

    try
    {
        arf_parse(arf_sprite_schema, file_contents.c_str(), sprite_section);
    }
    catch (std::exception& e)
    {
        throw MiscError(
            "Error parsing sprite file \"" + _path + "\": " + e.what()
        );
    }

    // subimages
    for (ARFSection* subimages : sprite_section.m_sections)
    {
        for (const std::string& subimage : subimages->m_list)
        {
            m_subimage_paths.push_back(_dir + subimage);
        }
    }

    if (m_subimage_paths.empty())
    {
        throw MiscError("sprite \"" + m_name + "\" needs at least one subimage.");
    }

    std::vector<std::string_view> arr;
    std::string arrs;

    // dimensions
    arrs = sprite_section.get_value("dimensions", "[-1, -1]");
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"dimensions\" should be a 2-tuple.");
    m_dimensions.x = svtoi(arr[0]);
    m_dimensions.y = svtoi(arr[1]);
    arr.clear();

    if (m_dimensions.x == -1 || m_dimensions.y == -1)
    // load sprite and read its dimensions
    {
        throw MiscError("Inferred sprite dimensions not yet implemented.");
        // TODO
    }

    // origin
    arrs = sprite_section.get_value("origin", "[0, 0]");
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"origin\" should be a 2-tuple.");
    m_offset.x = svtoi(arr[0]);
    m_offset.y = svtoi(arr[1]);
    arr.clear();

    // bbox
    if (sprite_section.has_value("bbox"))
    {
        arrs = sprite_section.get_value("bbox", "[0, 0], [1, 1]");
        std::vector<std::string> subarrs;
        split(subarrs, arrs, "x");
        if (subarrs.size() != 2) goto bbox_error;
        for (std::string& s : subarrs)
        {
            trim(s);
        }

        // parse sub-arrays

        // x bounds
        arf_parse_array(std::string{ subarrs.at(0) }.c_str(), arr);
        if (arr.size() != 2) goto bbox_error;
        m_aabb.m_start.x = svtoi(arr[0]);
        m_aabb.m_end.x = svtoi(arr[1]);
        arr.clear();

        // y bounds
        arf_parse_array(std::string{ subarrs.at(1) }.c_str(), arr);
        if (arr.size() != 2) goto bbox_error;
        m_aabb.m_start.y = svtoi(arr[0]);
        m_aabb.m_end.y = svtoi(arr[1]);
        arr.clear();

        if (false)
        {
        bbox_error:
            throw MiscError("field \"bbox\" should be 2-tuple x 2-tuple.");
        }
    }
    else
    {
        m_aabb = { {0, 0}, m_dimensions };
    }

    m_separate_collision_masks = svtoi(sprite_section.get_value("sepmasks", "0"));
    m_alpha_tolerance = svtoi(
        sprite_section.get_value("tolerance", "0")
    );

    std::string colkind = sprite_section.get_value(
        "colkind", sprite_section.get_value("collision", "1")
    );
    if (is_digits(colkind))
    {
        m_colkind = std::stoi(colkind);
    }
    else
    {
        if (colkind == "precise" || colkind == "raster")
        {
            m_colkind = 0;
        }
        else if (colkind == "separate")
        {
            m_colkind = 0;
            m_separate_collision_masks = true;
        }
        else if (colkind == "rect" || colkind == "aabb" || colkind == "rectangle" || colkind == "bbox")
        {
            m_colkind = 1;
        }
        else if (colkind == "ellipse" || colkind == "circle")
        {
            m_colkind = 2;
        }
        else if (colkind == "diamond")
        {
            m_colkind = 3;
        }
        else
        {
            throw MiscError("Unrecognized collision type \'" + colkind + "\"");
        }
    }
}

void ResourceSprite::load_file_xml()
{
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
        bool casechange = false;
        std::string path = case_insensitive_native_path(path_directory(_path), frame.text().get(), &casechange);
        if (!path_exists(path)) throw MiscError("Failed to find resource path " + path);
        if (casechange) std::cout << "WARNING: path \""<< path_directory(_path) << frame.text().get() << "\" required case-insensitive lookup:\n  Became \"" << path << "\"\n  This is time-consuming and should be corrected.\n";
        if (m_subimage_paths.size() <= index)
        {
            m_subimage_paths.resize(index + 1);
        }
        m_subimage_paths[index] = path;
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

    m_sprite_asset->m_subimage_paths = m_subimage_paths;
    m_sprite_asset->m_subimage_count = m_subimage_paths.size();

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
        throw MiscError("unknown collision shape for sprite " + m_name);
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

void ResourceSprite::compile(bytecode::ProjectAccumulator&, const bytecode::Library* library)
{
    if (m_sprite_asset->m_shape != asset::AssetSprite::raster)
    {
        // no need for image data.
        return;
    }

    if (!m_separate_collision_masks)
    {
        // just add one raster.
        addRaster();
    }

    for (const std::string& image_path : m_sprite_asset->m_subimage_paths)
    {
        if (m_separate_collision_masks)
        // add one for each frame.
        {
            addRaster();
        }
        bool* data = m_sprite_asset->m_raster.back().m_data;
        int width, height, channel_count;

        // TODO: confirm sprite image dimensions match the actual iamge.
        unsigned char* img_data = stbi_load(image_path.c_str(), &width, &height, &channel_count, 4);

        if (!img_data)
        {
            throw MiscError("Failed to load sprite " + image_path);
        }

        size_t i = 0;
        for (unsigned char* c = img_data + 3; c < img_data + width * height * channel_count; c += channel_count, ++i)
        {
            if (*c >= m_alpha_tolerance)
            {
                data[i] = 1;
            }
        }

        stbi_image_free(img_data);
    }
}
}}
