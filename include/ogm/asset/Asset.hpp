// looks up assets by name.

#pragma once

#include "ogm/common/types.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <map>

namespace ogm::asset {

// TODO: question why assets are all subclasses when
// there is no actual advantage to it, considering
// they don't share any features. There is only
// the penalty of virtualization.
class Asset
{
public:
    virtual ~Asset() { }
};

}
