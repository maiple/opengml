#pragma once

#include "Asset.hpp"

#include "ogm/geometry/aabb.hpp"

#include <vector>

namespace ogm
{
namespace asset
{
using namespace ogm::geometry;

class AssetSprite : public Asset
{
    // although in practice these are all constrained to integral values,
    // type conversion to coord_t is common enough that it's more pragmatic
    // to keep these as coord_t from the getgo.
public:
    Vector<coord_t> m_offset;
    Vector<coord_t> m_dimensions;

    // bounding box (relative to (0, 0); not relative to m_offset)
    AABB<coord_t> m_aabb;
    bool m_precise;
    enum ShapeType {
        rectangle,
        diamond,
        ellipse,
        raster
    } m_shape;

    std::vector<std::string> m_subimage_paths;

    size_t image_count() const
    {
        return m_subimage_paths.size();
    }
};

}
}
