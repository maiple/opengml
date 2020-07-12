#include "ogm/asset/Image.hpp"
#include "ogm/common/error.hpp"
#include "stb_image.h"

namespace ogm { namespace asset {

void Image::realize_data()
{
    if (m_data == nullptr)
    {
        int32_t width, height, channel_count;
        m_data = stbi_load(m_path.c_str(), &width, &height, &channel_count, 4);

        if (!m_data)
        {
            throw MiscError("Failed to load image \"" + m_path + "\": " + stbi_failure_reason());
        }

        // confirm dimensions match.
        if (m_dimensions.x >= 0 && m_dimensions.x != width)
        {
            throw MiscError(
                "Image dimensions mismatch \"" + m_path
                + "\": expected width of " + std::to_string(m_dimensions.x)
                + ", actual image has width of :" + std::to_string(width)
            );
        }
        if (m_dimensions.y >= 0 && m_dimensions.y != height)
        {
            throw MiscError(
                "Image dimensions mismatch \"" + m_path
                + "\": expected height of " + std::to_string(m_dimensions.y)
                + ", actual image has width of :" + std::to_string(height)
            );
        }
        m_dimensions = {width, height};
    }
}

void Image::load_from_memory(const unsigned char* data, size_t len)
{
    if (m_data == nullptr)
    {
        int32_t width, height, channel_count;
        m_data = stbi_load_from_memory(data, len, &width, &height, &channel_count, 4);

        if (!m_data)
        {
            throw MiscError("Failed to load embedded image:" + std::string(stbi_failure_reason()));
        }

        // confirm dimensions match.
        if (m_dimensions.x >= 0 && m_dimensions.x != width)
        {
            throw MiscError(
                "Image dimensions mismatch \"" + m_path
                + "\": expected width of " + std::to_string(m_dimensions.x)
                + ", actual image has width of :" + std::to_string(width)
            );
        }
        if (m_dimensions.y >= 0 && m_dimensions.y != height)
        {
            throw MiscError(
                "Image dimensions mismatch \"" + m_path
                + "\": expected height of " + std::to_string(m_dimensions.y)
                + ", actual image has width of :" + std::to_string(height)
            );
        }
        m_dimensions = {width, height};
    }
}

Image Image::cropped(const geometry::AABB<int32_t>& region)
{
    realize_data();
    
    // make sure given region is within this image.
    const geometry::AABB<int32_t>& full{ {0, 0}, m_dimensions };
    if (full.intersection(region) != region)
    {
        throw MiscError("cropped region exceeds boundaries of source image.");
    }
    
    Image c;
    c.m_dimensions = region.diagonal();
    c.m_data = (uint8_t*) malloc(c.m_dimensions.area() * 4);
    
    // copy data
    for (size_t y = region.top(); y < region.bottom(); ++y)
    {
        uint8_t* src_start = m_data + 4 * (
            region.left() + m_dimensions.x * y
        );
        
        uint8_t* dst_start = c.m_data + 4 * (
            region.width() * (y - region.top())
        );
        
        size_t length = 4 * region.width();
        
        memcpy(dst_start, src_start, length);
    }
    
    return c;
}

}}
