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
            throw MiscError("Failed to load image " + m_path);
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

}}
