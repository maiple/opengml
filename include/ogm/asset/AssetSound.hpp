#pragma once

#include "Asset.hpp"

#include "ogm/geometry/aabb.hpp"
#include "ogm/collision/collision.hpp"

#include <vector>

namespace ogm
{
namespace asset
{
using namespace ogm::geometry;

class AssetSound : public Asset
{
public:
    std::string m_path;
    real_t m_volume;
    real_t m_pan;
    uint32_t m_bit_rate;
    uint32_t m_sample_rate;
    uint8_t m_bit_depth;
    bool m_preload;
    int32_t m_effects;
    int32_t m_type;
    bool m_compressed;
    bool m_streamed;
    bool m_uncompress_on_load;
};

}
}
