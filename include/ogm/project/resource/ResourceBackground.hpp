#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/geometry/Vector.hpp"

#include <string>
#include <map>

namespace ogm { namespace project {

class ResourceBackground : public Resource
{
public:
    ResourceBackground(const char* path, const char* name);

    void load_file() override;
    void precompile(bytecode::ProjectAccumulator&);

    asset::AssetBackground* m_asset;
    std::string m_path;
    std::string m_name;
    geometry::Vector<coord_t> m_dimensions;
    asset::Image m_image;

    // non-asset data
    bool m_is_tileset = false;
    geometry::Vector<coord_t> m_tileset_dimensions;
    geometry::Vector<coord_t> m_offset;
    geometry::Vector<coord_t> m_sep;

private:
    void load_file_xml();

    void load_file_arf();
};

}}
