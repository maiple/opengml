#pragma once

#include "Asset.hpp"

#include "ogm/geometry/aabb.hpp"

#include "ogm/collision/collision.hpp"

#include <vector>

namespace ogm::asset
{

// Represents the single source-of-truth for an image.
// It can be any of:
// - a path to an image
// - a buffer of image data
// - a path *and* buffer (in which case the buffer takes precedence)
//
// at any time, the image data can be loaded from the path by invoking
// the realize_data() function.
//
// additionally, there is a dimensions field which is either (-1, -1), meaning
// dimensions TBD, or it can be set even in lieu of the data being set.
// (in which case the dimensions will be verified when the data is loaded.)
class Image
{
public:
    // AT LEAST ONE: a path to an image, and/or a buffer of image data.
    // if the buffer is supplied, it should be treated as the more reliable
    // source of image data.
    std::string m_path;

    // if dimensions are negative, then they are T.B.D.
    geometry::Vector<int32_t> m_dimensions = {-1, -1};

    // m_data is either nullptr or owned by this subimage.
    // malloc/free.
    // length is 4 * m_dimensions.area()
    uint8_t* m_data=nullptr;

    // loads image data from path.
    void realize_data();

    size_t get_data_len() const
    {
        if (!m_data) return 0;
        ogm_assert(m_dimensions.x >= 0 && m_dimensions.y >= 0);
        return m_dimensions.area() * 4;
    }

    Image()=default;
    Image(const Image& other)
        : m_path(other.m_path)
    {
        m_dimensions = other.m_dimensions;
        if (other.m_data)
        {
            m_data = (uint8_t*)malloc(other.get_data_len());
            memcpy(m_data, other.m_data, other.get_data_len());
        }
    }

    Image(std::string&& path)
        : m_path(std::move(path))
    { }

    ~Image()
    {
        free(m_data);
    }
};

}
