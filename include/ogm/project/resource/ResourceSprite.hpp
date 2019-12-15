#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/geometry/aabb.hpp"

#include <string>
#include <map>

namespace ogm { namespace project {

class ResourceSprite : public Resource
{
public:
    ResourceSprite(const char* path, const char* name);

    void load_file() override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);
    const char* get_name() { return m_name.c_str(); }

    asset::AssetSprite* m_sprite_asset;
    std::string m_path;
    std::string m_name;

    int32_t m_colkind;
    int32_t m_bboxmode; // what is this for..?
    bool m_separate_collision_masks;
    uint32_t m_alpha_tolerance;
    geometry::AABB<coord_t> m_aabb;
    geometry::Vector<coord_t> m_dimensions;
    geometry::Vector<coord_t> m_offset;

    std::vector<asset::AssetSprite::SubImage> m_subimages;

private:
    void addRaster();

    void load_file_xml();

    void load_file_arf();
};

}}
