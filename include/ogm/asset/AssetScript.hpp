#pragma once

#include "Asset.hpp"

namespace ogm
{
namespace asset
{

class AssetScript : public Asset
{
public:
    bytecode_index_t m_bytecode_index;
};

}
}
