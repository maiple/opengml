#pragma once

#include "Asset.hpp"

namespace ogm
{
namespace asset
{

class AssetShader : public Asset
{
public:
    std::string m_vertex_source;
    std::string m_pixel_source;
};

}
}
