#pragma once

#include "ogm/common/types.hpp"

namespace ogm::interpreter
{

// instance variables IDs enum
// variabe IDs are v_id, vobject_index, etc.
// listed in ivars.h

#define DEF(x) v_##x,
#define DEFREADONLY(x) DEF(x)
enum
{
    #include "ivars.h"
    INSTANCE_VARIABLE_MAX
};
#undef DEFREADONLY
#undef DEF

}
