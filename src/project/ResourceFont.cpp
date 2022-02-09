#include "ogm/project/resource/ResourceFont.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "XMLError.hpp"

#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <string>

using nlohmann::json;

namespace ogm::project {

ResourceFont::ResourceFont(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
{
}

ResourceFont::~ResourceFont()
{
    if (m_doc) delete m_doc;
    if (m_json) delete (json*)m_json;
}

void ResourceFont::load_file()
{   
    if (mark_progress(FILE_LOADED)) return;
    std::string raw_script;

    std::string _path = native_path(m_path);

    // read in font
    raw_script = read_file_contents(_path);
    m_file_contents = raw_script;
    
    if (ends_with(m_path, ".font.gmx"))
    {
        m_file_type = FONT_GMX;
    }
    else if (ends_with(m_path, ".yy"))
    {
        m_file_type = FONT_JSON;
    }
    else
    {
        m_file_type = UNKNOWN;
        throw ResourceError(1016, this, "Unrecognized font file extension");
    }
}

void ResourceFont::parse(const bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PARSED)) return;
    switch (m_file_type)
    {
    case FONT_GMX: {
            m_doc = new pugi::xml_document();
            pugi::xml_document& doc = *m_doc;
            pugi::xml_parse_result result = doc.load_string(m_file_contents.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);
            if (!result)
            {
                throw ResourceError(1014, this, "Failed to parse font.gmx file");
            }
            break;
        }
    case FONT_JSON: {
            m_json = new json(json::parse(m_file_contents));
        }
        break;
    default:
        return;
    }
}

void ResourceFont::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    switch (m_file_type)
    {
    case FONT_GMX: {
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
        break;
    case FONT_JSON: {
            json& j = *(json*)m_json;
            m_v2_id = j.at("id");
            asset::AssetFont* af = acc.m_assets->add_asset<asset::AssetFont>(m_name.c_str());
            af->m_path = m_path;
            
            // TODO: how to load TTFs.
            
            // glyphs
            for (const json& glyph : j.at("glyphs"))
            {
                const json& xg = glyph.at("Value");
                uint64_t glyph_n = glyph.at("character").get<uint64_t>();
                asset::Glyph& ag = af->m_glyphs[glyph_n];
                ag.m_position = {
                    xg.at("x").get<int32_t>(),
                    xg.at("y").get<int32_t>()
                };

                ag.m_dimensions = {
                    xg.at("w").get<int32_t>(),
                    xg.at("h").get<int32_t>()
                };

                af->m_vshift = std::max(af->m_vshift, ag.m_dimensions.y);

                ag.m_offset = xg.at("offset").get<int32_t>();
                ag.m_shift = xg.at("shift").get<int32_t>();
            }
        }
        break;
    default:
        return;
    }
}

void ResourceFont::compile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(COMPILED)) return;
    
    // We did everything in precompile.
}

}
