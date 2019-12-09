#include "ogm/project/resource/ResourceShader.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include <string>

namespace ogm { namespace project {

ResourceShader::ResourceShader(const char* path, const char* name)
    : m_path(path)
    , m_name(name)
{
}

void ResourceShader::load_file()
{
    std::string raw_script;

    std::string _path = native_path(m_path);

    // read in script
    raw_script = read_file_contents(_path);

    m_source = raw_script;
}

void ResourceShader::parse()
{ }

void ResourceShader::precompile(bytecode::ProjectAccumulator& acc)
{
    asset::AssetShader* sh = acc.m_assets->add_asset<asset::AssetShader>(m_name.c_str());
    std::vector<std::string> contents;
    split(contents, m_source, "#################");
    if (contents.size() < 2)
    {
        throw MiscError("Error parsing shader -- no vertex/pixel divider.");
    }
    sh->m_vertex_source = contents.front();
    sh->m_pixel_source = contents.back();
}

void ResourceShader::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{ }

}}