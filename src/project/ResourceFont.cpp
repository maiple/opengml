#include "ogm/project/resource/ResourceFont.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "XMLError.hpp"

#include <pugixml.hpp>
#include <string>

namespace ogm { namespace project {

ResourceFont::ResourceFont(const char* path, const char* name)
    : m_path(path)
    , m_name(name)
    , m_doc(new pugi::xml_document())
{
}

void ResourceFont::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    std::string raw_script;

    std::string _path = native_path(m_path);

    // read in script
    raw_script = read_file_contents(_path);

    m_gmx_contents = raw_script;
}

void ResourceFont::parse(const bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PARSED)) return;
    pugi::xml_document& doc = *m_doc;
    pugi::xml_parse_result result = doc.load_string(m_gmx_contents.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);
    if (!result)
    {
        throw MiscError("Failed to parse font.gmx file");
    }
}

void ResourceFont::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    pugi::xml_document& doc = *m_doc;
    asset::AssetFont* af = acc.m_assets->add_asset<asset::AssetFont>(m_name.c_str());
    af->m_path = m_path;

    pugi::xml_node node_font = doc.child("font");
    std::string s_ittf = node_font.child("includeTTF").text().get();
    if (s_ittf != "0")
    // don't know how to do ttf yet.
    {
        af->m_ttf = true;
        return;
    }
    else
    // image-based, not ttf
    {
        af->m_ttf = false;
        std::string image_path = node_font.child("image").text().get();
        af->m_path = path_directory(m_path) + image_path;

        // glyphs
        for (pugi::xml_node xg : node_font.child("glyphs").children("glyph"))
        {
            uint64_t glyph_n = std::atoi(xg.attribute("character").value());
            asset::Glyph& ag = af->m_glyphs[glyph_n];
            ag.m_position = {
                std::atoi(xg.attribute("x").value()),
                std::atoi(xg.attribute("y").value())
            };

            ag.m_dimensions = {
                std::atoi(xg.attribute("w").value()),
                std::atoi(xg.attribute("h").value())
            };

            af->m_vshift = std::max(af->m_vshift, ag.m_dimensions.y);

            ag.m_offset = std::atoi(xg.attribute("offset").value());
            ag.m_shift = std::atoi(xg.attribute("shift").value());
        }
    }
}

void ResourceFont::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
    if (mark_progress(COMPILED)) return;
}

}}
