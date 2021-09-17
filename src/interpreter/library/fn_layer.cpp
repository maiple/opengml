#include "libpre.h"
    #include "fn_layer.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/common/error.hpp"
#include "ogm/geometry/Vector.hpp"

#include <cmath>
#include <algorithm>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

#ifdef OGM_LAYERS

void ogm::interpreter::fn::layer_set_target_room(VO out, V vroom)
{

    asset_index_t room_index = vroom.castCoerce<asset_index_t>();
    AssetRoom* room = frame.m_assets.get_asset<AssetRoom*>(object_index);
    if (room)
    {
        frame.m_data.m_layer_room_target = room_index;
    }
    else
    {
        frame.m_data.m_layer_room_target = k_no_asset;
    }
}

void ogm::interpreter::fn::layer_reset_target(VO out, V vroom)
{
    frame.m_data.m_layer_room_target = k_no_asset;
}

void ogm::interpreter::fn::layer_get_target_room(VO out)
{
    if (frame.m_data.m_layer_room_target == k_no_asset)
    {
        return frame.m_data.m_room_index;
    }
    else
    {
        return frame.m_data.m_layer_room_target;
    }
}

#endif