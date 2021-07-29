#include "ogm/project/resource/ResourceShader.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "XMLError.hpp"

#include <string>

namespace ogm { namespace project {

ResourceShader::ResourceShader(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
{
}

void ResourceShader::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    std::string raw_script;

    std::string _path = native_path(m_path);

    // read in script
    raw_script = read_file_contents(_path);

    m_source = raw_script;
}

void ResourceShader::parse(const bytecode::ProjectAccumulator& acc)
{ }

void ResourceShader::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    asset::AssetShader* sh = acc.m_assets->add_asset<asset::AssetShader>(m_name.c_str());
    std::vector<std::string> contents;
    split(contents, m_source, "#################");
    if (contents.size() < 2)
    {
        throw ResourceError(1044, this, "No vertex/pixel divider found.");
    }
    sh->m_vertex_source = contents.front();
    trim(sh->m_vertex_source);
    sh->m_pixel_source = contents.back();
    trim(sh->m_pixel_source);
}

void ResourceShader::compile(bytecode::ProjectAccumulator& acc)
{ }

}}
