#ifndef OGM_PROJECT_SCRIPT_HPP
#define OGM_PROJECT_SCRIPT_HPP

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

#endif