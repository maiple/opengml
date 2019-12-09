#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>
#include <cstring>

namespace ogm { namespace project {

ResourceBackground::ResourceBackground(const char* path, const char* name): m_path(path), m_name(name)
{ }

void ResourceBackground::load_file()
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(m_path.c_str(), pugi::parse_default | pugi::parse_escapes);
    const std::string _path = native_path(m_path);

    if (!result)
    {
        throw MiscError("background.gmx file not found: " + _path);
    }

    pugi::xml_node node = doc.child("background");
    bool casechange = false;
    m_path = case_insensitive_native_path(path_directory(_path), node.child("data").text().get(), &casechange);
    if (casechange) std::cout << "WARNING: path \""<< path_directory(_path) << node.child("data").text().get() << "\" required case-insensitive lookup.\n  This is time-consuming and should be corrected.\n";

    const char* node_w = node.child("width").text().get();
    const char* node_h = node.child("height").text().get();

    m_dimensions = { std::atoi(node_w), std::atoi(node_h) };
}

void ResourceBackground::precompile(bytecode::ProjectAccumulator& acc)
{
    auto& assets = *acc.m_assets;
    m_asset = assets.add_asset<asset::AssetBackground>(m_name.c_str());
    m_asset->m_path = m_path;
    m_asset->m_dimensions = m_dimensions;
}

}
}
