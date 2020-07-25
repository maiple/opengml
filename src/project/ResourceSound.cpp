#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"
#include "XMLError.hpp"

#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <string>
#include <cstring>

using nlohmann::json;

namespace ogm::project {

ARFSchema arf_sound_schema
{
    "sound",
    ARFSchema::DICT,
    {
        "data",
        ARFSchema::LIST
    }
};

ResourceSound::ResourceSound(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
{ }

void ResourceSound::load_file()
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
        throw ResourceError(1025, this, "Unrecognized file extension for sound file \"{}\"", m_path);
    }
}

void ResourceSound::load_file_arf()
{
    const std::string _path = native_path(m_path);

    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection object_section;

    arf_parse(arf_sound_schema, file_contents.c_str(), object_section);

    // data
    for (const ARFSection* event_data : object_section.m_sections)
    {
        if (event_data->m_name == "data")
        {
            for (const std::string& path : event_data->m_list)
            {
                m_data_path = path_directory(native_path(m_path));
                m_data_path = path_join(m_data_path, path);
            }
        }
    }
}

void ResourceSound::load_file_xml()
{
    using namespace std::string_literals;
    
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    check_xml_result<ResourceError>(1063, result, _path.c_str(), "sound.gmx file not found: " + _path, this);

    pugi::xml_node node = doc.child("sound");
    
    std::string path = node.child("data").text().get();
    m_data_path = path_join(path_directory(_path), "audio", path);
    
    m_volume = std::stod(node.child("volume").child("volume").text().get());
    m_effects = std::stoi(node.child("effects").text().get());
    m_pan = std::stod(node.child("pan").text().get());
    m_bit_rate = std::stod(node.child("bitRates").child("bitRate").text().get());
    m_type = std::stoi(node.child("types").child("type").text().get());
    m_bit_depth = std::stoi(node.child("bitDepths").child("bitDepth").text().get());
    m_preload = node.child("preload").text().get() != "0"s;
    m_streamed = node.child("streamed").text().get() != "0"s;
    m_uncompress_on_load = node.child("uncompressOnLoad").text().get() != "0"s;
    
}

void ResourceSound::load_file_json()
{
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw ResourceError(1045, this, "Error opening file \"{}\"", m_path);
    
    json j;
    ifs >> j;
    
    m_v2_id = j.at("id");
    
    m_data_path = remove_extension(m_path);
}

void ResourceSound::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    m_asset = acc.m_assets->add_asset<asset::AssetSound>(m_name.c_str());
    m_asset->m_path = m_data_path;
    m_asset->m_volume = m_volume;
    m_asset->m_effects = m_effects;
    m_asset->m_pan = m_pan;
    m_asset->m_bit_rate = m_bit_rate;
    m_asset->m_type = m_type;
    m_asset->m_bit_depth = m_bit_depth;
    m_asset->m_preload = m_preload;
    m_asset->m_uncompress_on_load = m_uncompress_on_load;
}

}