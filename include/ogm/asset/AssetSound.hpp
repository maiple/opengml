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
};

}
}
