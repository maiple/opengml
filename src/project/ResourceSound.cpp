#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"
#include "XMLError.hpp"

#include <pugixml.hpp>
#include <string>
#include <cstring>

namespace ogm { namespace project {

ARFSchema arf_sound_schema
{
    "sound",
    ARFSchema::DICT,
    {
        "data",
        ARFSchema::LIST
    }
};

ResourceSound::ResourceSound(const char* path, const char* name): m_path(path), m_name(name)
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
    else
    {
        throw MiscError("Unrecognized file extension for object file " + m_path);
    }
}

void ResourceSound::load_file_arf()
{
    const std::string _path = native_path(m_path);

    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection object_section;

    try
    {
        arf_parse(arf_sound_schema, file_contents.c_str(), object_section);
    }
    catch (std::exception& e)
    {
        throw MiscError(
            "Error parsing object file \"" + _path + "\": " + e.what()
        );
    }

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
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    check_xml_result(result, _path.c_str(), "sound.gmx file not found: " + _path);

    pugi::xml_node node = doc.child("sound");
    // TODO
    (void)node;
}

void ResourceSound::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    m_asset = acc.m_assets->add_asset<asset::AssetSound>(m_name.c_str());
    m_asset->m_path = m_data_path;
}

}
}
