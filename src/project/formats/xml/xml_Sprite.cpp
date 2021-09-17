#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "project/XMLError.hpp"

#include <pugixml.hpp>
#include <string>

namespace ogm::project
{

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

}