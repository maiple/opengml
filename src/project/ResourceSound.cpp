#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>
#include <cstring>

namespace ogm { namespace project {

ResourceSound::ResourceSound(const char* path, const char* name): m_path(path), m_name(name)
{ }


void ResourceSound::precompile(bytecode::ProjectAccumulator& acc)
{
    m_asset = acc.m_assets->add_asset<asset::AssetSound>(m_name.c_str());
    const std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes);

    if (!result)
    {
        throw MiscError("sound.gmx file not found: " + _path);
    }

    pugi::xml_node node = doc.child("sound");
    // TODO
    (void)node;
}

}
}
