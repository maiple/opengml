#include "ogm/asset/Image.hpp"
#include "ogm/common/error.hpp"
#include "stb_image.h"

#include <xbr.hpp>
#include <cassert>

namespace ogm::asset {

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

Image Image::rotated(real_t angle, geometry::Vector<real_t>& io_centre)
{
    realize_data();
    
    angle = positive_modulo(angle, TAU);
    if (angle == 0) return *this;
    
    // determine new bounds
    geometry::AABB<real_t> aabb_p{ -io_centre, m_dimensions - io_centre};
    geometry::AABB<int32_t> aabb_n = aabb_p.rotated(angle);
    
    geometry::Vector<real_t> o_centre = -aabb_n.top_left();
    
    Image c;
    c.m_dimensions = aabb_n.diagonal();
    c.m_data = alloc<uint8_t>(c.m_dimensions.area() * 4);
    memset(c.m_data, 0, c.m_dimensions.area() * 4);
    
    // copy pixels in.
    for (size_t y = 0; y < c.m_dimensions.y; ++y)
    {
        for (size_t x = 0; x < c.m_dimensions.x; ++x)
        {
            geometry::Vector<real_t> rdst{x + 0.5f, y + 0.5f };
            geometry::Vector<real_t> delta = rdst - o_centre;
            geometry::Vector<int32_t> src = delta.add_angle_copy(angle) + io_centre;
            
            // check bounds
            if (src.x < 0 || src.y < 0 || src.x >= m_dimensions.x || src.y >= m_dimensions.y) continue;
            if (x < 0 || y < 0 || x >= c.m_dimensions.x || y >= c.m_dimensions.y) continue;
            
            // copy data
            size_t src_offset = src.x + src.y * m_dimensions.x;
            size_t dst_offset = x + y * c.m_dimensions.x;
            
            memcpy(c.m_data + 4 * dst_offset, m_data + 4 * src_offset, 4);
        }
    }
    
    io_centre = o_centre;
    return c;
}

Image Image::scaled(double sx, double sy)
{
    realize_data();
    
    Image c;
    c.m_dimensions = {
        std::abs(static_cast<int32_t>(m_dimensions.x * sx)),
        std::abs(static_cast<int32_t>(m_dimensions.y * sy))
    };
    
    c.m_data = (uint8_t*) malloc(c.m_dimensions.area() * 4);
    
    // copy data
    for (size_t y = 0; y < c.m_dimensions.y; ++y)
    {
        int32_t src_y = y / sy;
        while (src_y < 0) src_y += m_dimensions.y;
        if (src_y >= m_dimensions.y) src_y = m_dimensions.y - 1;
        for (size_t x = 0; x < c.m_dimensions.x; ++x)
        {
            int32_t src_x = x / sx;
            while (src_x < 0) src_x += m_dimensions.x;
            if (src_x >= m_dimensions.x) src_x = m_dimensions.x - 1;
            
            assert(src_x >= 0 && src_y >= 0 && src_x < m_dimensions.x && src_y < m_dimensions.y);
            
            size_t src_offset = static_cast<size_t>(src_x + src_y * m_dimensions.x);
            size_t offset = static_cast<size_t>(x + y * c.m_dimensions.x);
            
            // copy this pixel.
            memcpy(c.m_data + offset * 4, m_data + src_offset * 4, 4);
        }
    }
    
    return c;
}

Image Image::xbr(uint8_t scale, bool blend_colours, bool scale_alpha)
{
    realize_data();
    
    if (scale == 1) return *this;
    if (scale == 0 || scale > 4)
    {
        throw MiscError("xbr can only scale 1-4 times.");
    }
    
    auto xbr = (scale == 2)
        ? xbr2x
        : (scale == 3)
            ? xbr3x
            : xbr4x;
    
    Image c;
    c.m_dimensions = m_dimensions * scale;
    c.m_data = alloc<uint8_t>(c.m_dimensions.area() * 4);
    
    xbr(
        reinterpret_cast<image>(m_data),
        reinterpret_cast<image>(c.m_data),
        m_dimensions.x,
        m_dimensions.y,
        blend_colours,
        scale_alpha
    );
    
    return c;
}

}
