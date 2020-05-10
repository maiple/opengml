#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"
#include "XMLError.hpp"

#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <pugixml.hpp>
#include <string>
#include <cstring>

using nlohmann::json;

namespace ogm::project {

ResourceBackground::ResourceBackground(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
{ }

void ResourceBackground::load_file()
{
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
        throw MiscError("Unrecognized file extension for background file " + m_path);
    }
}

namespace
{
    ARFSchema arf_background_schema
    {
        "background",
        ARFSchema::DICT
    };
}

void ResourceBackground::load_file_arf()
{
    if (mark_progress(FILE_LOADED)) return;
    std::string _path = native_path(m_path);
    std::string _dir = path_directory(_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection background_section;

    try
    {
        arf_parse(arf_background_schema, file_contents.c_str(), background_section);
    }
    catch (std::exception& e)
    {
        throw MiscError(
            "Error parsing background file \"" + _path + "\": " + e.what()
        );
    }

    // path to background
    {
        std::string resolved_path = background_section.get_value("image", "");

        if (resolved_path == "")
        {
            throw MiscError("background \"" + m_name + "\" needs an image.");
        }
        else
        {
            resolved_path = case_insensitive_native_path(_dir, resolved_path);
        }

        m_image = std::move(resolved_path);
    }

    std::vector<std::string_view> arr;
    std::string arrs;

    // dimensions
    arrs = background_section.get_value("dimensions", "[-1, -1]");
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"dimensions\" should be a 2-tuple.");
    m_dimensions.x = svtoi(arr[0]);
    m_dimensions.y = svtoi(arr[1]);
    arr.clear();

    if (m_dimensions.x < 0 || m_dimensions.y < 0)
    // load sprite and read its dimensions
    {
        m_image.realize_data();

        if (m_dimensions.x < 0)
        {
            m_dimensions.x = m_image.m_dimensions.x;
        }

        if (m_dimensions.y < 0)
        {
            m_dimensions.y = m_image.m_dimensions.y;
        }
    }
    else
    {
        // tell image about dimensions (to resolve errors later if mismatch)
        m_image.m_dimensions = m_dimensions;
    }

    // dimensions
    arrs = background_section.get_value("tileset", "0");
    if (is_digits(arrs))
    {
        m_is_tileset = false;
    }
    else
    {
        arf_parse_array(arrs.c_str(), arr);
        if (arr.size() != 2) throw MiscError("field \"tileset\" should be a 2-tuple.");
        m_tileset_dimensions.x = svtoi(arr[0]);
        m_tileset_dimensions.y = svtoi(arr[1]);
        arr.clear();
    }

    // dimensions
    arrs = background_section.get_value("offset", "[0, 0]");
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"offset\" should be a 2-tuple.");
    m_offset.x = svtoi(arr[0]);
    m_offset.y = svtoi(arr[1]);
    arr.clear();

    // separation
    arrs = background_section.get_value("sep", "[0, 0]");
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"sep\" should be a 2-tuple.");
    m_sep.x = svtoi(arr[0]);
    m_sep.y = svtoi(arr[1]);
    arr.clear();
}

void ResourceBackground::load_file_xml()
{
    if (mark_progress(FILE_LOADED)) return;
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(m_path.c_str(), pugi::parse_default | pugi::parse_escapes);
    const std::string _path = native_path(m_path);

    check_xml_result(result, _path.c_str(), "background.gmx file not found: " + _path);

    pugi::xml_node node = doc.child("background");
    bool casechange = false;
    m_image = case_insensitive_native_path(path_directory(_path), node.child("data").text().get(), &casechange);
    if (casechange) std::cout << "WARNING: path \""<< path_directory(_path) << node.child("data").text().get() << "\" required case-insensitive lookup.\n  This is time-consuming and should be corrected.\n";

    const char* node_w = node.child("width").text().get();
    const char* node_h = node.child("height").text().get();

    m_dimensions = { std::stod(node_w), std::stod(node_h) };

    m_tileset_dimensions.x = std::atoi(node.child("tilewidth").text().get());
    m_tileset_dimensions.y = std::atoi(node.child("tileheight").text().get());

    m_offset.x = std::atoi(node.child("tilexoff").text().get());
    m_offset.y = std::atoi(node.child("tileyoff").text().get());

    m_sep.x = std::atoi(node.child("tilehsep").text().get());
    m_sep.y = std::atoi(node.child("tilevsep").text().get());
}

void ResourceBackground::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    auto& assets = *acc.m_assets;
    m_asset = assets.add_asset<asset::AssetBackground>(m_name.c_str());
    m_asset->m_image = m_image;
    m_asset->m_dimensions = m_dimensions;
}

void ResourceBackground::load_file_json()
{
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw MiscError("Error parsing file " + m_path);
    
    json j;
    ifs >> j;
    
    m_v2_id = j.at("id");
    
    // TODO
    std::cout << "WARNING: ResourceBackground::load_file_json unfinished.\n";
}

}
