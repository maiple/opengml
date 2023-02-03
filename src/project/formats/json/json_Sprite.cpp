#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

#include <nlohmann/json.hpp>

#include "json_parse.hpp"

using nlohmann::json;

namespace ogm::project {

void ResourceSprite::load_file_json()
{
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw ResourceError(ErrorCode::F::file, this, "Error parsing file \"{}\"", m_path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Sprite");
    
    m_v2_id = j.at("id");
    
    for (const json& frame : j.at("frames"))
    {
        asset::AssetSprite::SubImage& subimage = m_subimages.emplace_back();
        
        std::string id = frame.at("id");
        std::string path = path_join(path_directory(m_path), id + ".png");
        if (!path_exists(path))
        {
            throw ResourceError(ErrorCode::F::imgfile, this, "Failed to find sprite subframe image at path \"{}\"", path);
        }
        subimage.m_path = path;
    }
    
    m_dimensions = {
        j.at("width").get<real_t>(),
        j.at("height").get<real_t>()
    };
    m_offset = {
        j.at("xorig").get<real_t>(),
        j.at("yorig").get<real_t>()
    };
    m_aabb = {
        { j.at("bbox_left").get<real_t>(), j.at("bbox_right").get<real_t>() },
        { j.at("bbox_right").get<real_t>(), j.at("bbox_bottom").get<real_t>() }
    };
    
    m_separate_collision_masks = j.at("sepmasks").get<bool>();
    m_bboxmode = j.at("bboxmode").get<int32_t>();
    m_colkind = j.at("colkind").get<int32_t>();
    m_alpha_tolerance = j.at("coltolerance").get<int32_t>();
    m_speed = j.at("playbackSpeed").get<real_t>();
    m_speed_real_time = j.at("playbackSpeedType").get<real_t>() == 0;
}
}