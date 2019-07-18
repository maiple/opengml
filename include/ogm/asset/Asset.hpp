// looks up assets by name.

#ifndef OGM_ASSET_INDEX_HPP
#define OGM_ASSET_INDEX_HPP

#include "ogm/common/types.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <map>

namespace ogm { namespace asset {

// TODO: question why assets are all subclasses when
// there is no actual advantage to it, considering
// they don't share any features. There is only
// the penalty of virtualization.
class Asset
{
public:
    virtual ~Asset() { }
};

}}

#endif
