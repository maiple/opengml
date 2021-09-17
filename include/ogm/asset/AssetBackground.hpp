#pragma once

#include "Asset.hpp"
#include "Image.hpp"

#include "ogm/geometry/aabb.hpp"
#include "ogm/collision/collision.hpp"

#include <vector>

namespace ogm
{
namespace asset
{
using namespace ogm::geometry;

class AssetBackground : public Asset
{
    // although in practice these are all constrained to integral values,
    // type conversion to coord_t is common enough that it's more pragmatic
    // to keep these as coord_t from the getgo.
public:
    Vector<coord_t> m_dimensions;

    Image m_image;
    
    // These are not meaningful (have no effect) prior to v2
    Vector<coord_t> m_tile_dimensions;
    Vector<coord_t> m_tile_sep;
    Vector<coord_t> m_tile_offset;
};

}
}
