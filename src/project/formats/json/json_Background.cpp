#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include "json_parse.hpp"

using nlohmann::json;

namespace ogm::project {

// backgrounds are called 'tilesets' in v2
void ResourceBackground::load_file_json()
{
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw ResourceError(1013, this, "Error opening file \"{}\"", m_path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "TileSet");
    
    m_image = path_join(path_directory(m_path), "output_tileset.png");
    m_is_tileset = true;
    m_v2_id = j.at("id");
    
    int tile_count = j.at("tile_count").get<int>();
    int columns = j.at("out_columns").get<int>();
    int rows = (tile_count + columns - 1) / columns;
    int hborder = j.at("out_tilehborder").get<int>();
    int vborder = j.at("out_tilevborder").get<int>();
    int twidth  = j.at("tilewidth").get<int>();
    int theight  = j.at("tileheight").get<int>();
    int txo  = j.at("tileyoff").get<int>();
    int tyo  = j.at("tilexoff").get<int>();
    int hsep  = j.at("tilehsep").get<int>();
    int vsep  = j.at("tilevsep").get<int>();
    ogm_assert(columns > 0);
    ogm_assert(vborder >= 0);
    ogm_assert(hborder >= 0);
    if (!(hsep == 0 && vsep == 0 && txo == 0 && tyo == 0))
    {
        throw MiscError("cannot currently handle vsep/hsep/xoffset/yoffset");
    }
    
    m_tileset_dimensions = geometry::Vector<int>{twidth + 2*hborder, theight + 2*hborder};
    m_dimensions = m_tileset_dimensions * geometry::Vector<int>{columns, rows};
    m_offset = geometry::Vector<int>{hborder, vborder};
    m_sep = geometry::Vector<int>{2 * hborder, 2 * vborder};
}

}