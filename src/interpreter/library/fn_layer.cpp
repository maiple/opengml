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
#define layers (frame.m_layers)

#ifdef OGM_LAYERS
namespace
{
    bool get_layer_id_from_variable(V v, layer_id& out)
    {
        if (v.is_string())
        {
            std::string layer_name = v.castCoerce<std::string>();
            auto iter = layers.m_layer_ids_by_name.find(layer_name);
            if (iter != layers.m_layer_ids.end())
            {
                out = layers.m_layer_ids_by_name.at(layer_name);
                return true;
            }
            out = k_no_layer;
            return false;
        }
        else
        {
            int64_t layer_id = v.castCoerce<int64_t>();
            if (layers.m_layers.find(layer_id) == layers.m_layers.end())
            {
                out = k_no_layer;
                return false;
            }
            out = static_cast<layer_id_t>(layer_id);
            return true;
        }
    }
    
    layer_id get_layer_id_from_variable_or_err(V v)
    {
        layer_id out;
        if (!get_layer_id_from_variable(out))
        {
            throw RuntimeError("Tried to access layer which does not exist");
        }
        return out;
    }
}

void ogm::interpreter::fn::instance_create_layer(VO out, V x, V y, V vlayer, V vobject_index)
{
    asset_index_t object_index = vobject_index.castCoerce<asset_index_t>();
    const AssetObject* object = frame.m_assets.get_asset<AssetObject*>(object_index);
    if (!object)
    {
        throw RuntimeError("Attempted to create non-existent object");
    }
    
    layer_id_t layer_id = get_layer_id_from_variable_or_err(vlayer);
    
    InstanceCreateArgs args = {
        object_index,
        {x.castCoerce<coord_t>(), y.castCoerce<coord_t>()}
    }
    args.m_type = InstanceCreateArgs::use_provided_layer;
    args.m_layer = layer_id;
    
    out = frame.create_instance(args);
}

void ogm::interpreter::fn::layer_set_target_room(VO out, V vroom)
{
    asset_index_t room_index = vroom.castCoerce<asset_index_t>();
    AssetRoom* room = frame.m_assets.get_asset<AssetRoom*>(object_index);
    if (room && vroom.castCoerce<real_t> != -1.0_r)
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
        out = -1_r;
    }
    else
    {
        out = frame.m_data.m_layer_room_target;
    }
}

void ogm::interpreter::fn::layer_exists(VO out, V v)
{
    if (v.is_string())
    {
        std::string layer_name = v.castCoerce<std::string>();
        auto iter = layers.m_layer_ids_by_name.find(layer_name);
        out = (iter != layers.m_layer_ids.end())
    }
    else
    {
        int64_t layer_id = v.castCoerce<int64_t>();
        out = layers.m_layers.find(layer_id) != frame.m_layers.end();
    }
}

void ogm::interpreter::fn::layer_get_id(VO out, V v)
{
    layer_id_t layer_id;
    if (!get_layer_id_from_variable(v))
    {
        out = -1_r;
    }
    out = layer_id;
}

void ogm::interpreter::fn::layer_get_depth(VO out, V v)
{
    layer_id_t layer_id = get_layer_id_from_variable_or_err(v);
    Layer& layer = layers.m_layers.at(layer_id);
    out = layer.m_depth;
}

#endif