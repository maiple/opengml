#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>
#include <cstring>

namespace ogm { namespace project {

ResourceBackground::ResourceBackground(const char* path, const char* name): m_path(path), m_name(name)
{ }


void ResourceBackground::precompile(bytecode::ProjectAccumulator& acc)
{
    auto& assets = *acc.m_assets;
    const std::string bg_name = m_name;
    m_asset = assets.add_asset<asset::AssetBackground>(bg_name.c_str());
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    if (!result)
    {
        throw MiscError("background.gmx file not found: " + _path);
    }

    pugi::xml_node node = doc.child("background");
    pugi::xml_node node_frames = node.child("frames");
    m_path = case_insensitive_native_path(path_directory(_path), node.child("data").text().get());
    m_asset->m_path = m_path;

    const char* node_w = node.child("width").text().get();
    const char* node_h = node.child("height").text().get();

    m_asset->m_dimensions = { atoi(node_w), atoi(node_h) };
}

}
}
