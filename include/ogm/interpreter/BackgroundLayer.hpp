#pragma once

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"

namespace ogmi
{
using namespace ogm;
using namespace ogm::geometry;

// a background of a room.
struct BackgroundLayer
{
    asset_index_t m_background_index = k_no_asset;
    Vector<coord_t> m_position{ 0, 0 };
    Vector<coord_t> m_velocity{ 0, 0 };
    bool m_tiled_x = false;
    bool m_tiled_y = false;
    bool m_visible = false;
    bool m_foreground = false;
};
}
