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

    enum ShapeType {
        rectangle,
        diamond,
        ellipse,
        raster
    } m_shape;

    size_t m_subimage_count = 0;

    // optional
    typedef asset::Image SubImage;
    std::vector<SubImage> m_subimages;

    // optional
    std::vector<collision::CollisionRaster> m_raster;

public:
    size_t image_count() const
    {
        return m_subimage_count;
    }

    ~AssetSprite()
    {
        // clean up raster data
        for (auto& raster : m_raster)
        {
            if (raster.m_data)
            {
                delete(raster.m_data);
            }
        }

        m_raster.clear();
    }
};

}
}
