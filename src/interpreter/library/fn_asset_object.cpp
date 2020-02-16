#include "libpre.h"
    #include "fn_asset.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

// TODO: some of these functions can take instances as well as objects.

void ogm::interpreter::fn::object_exists(VO out, V obj)
{
    asset_index_t index = obj.castCoerce<asset_index_t>();
    out = !!frame.m_assets.get_asset<AssetObject*>(index);
}

void ogm::interpreter::fn::object_get_parent(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = object->m_parent;
}

void ogm::interpreter::fn::object_get_name(VO out, V obj)
{
    asset_index_t index = obj.castCoerce<asset_index_t>();
    if (frame.m_assets.get_asset<AssetObject*>(index))
    {
        out = frame.m_assets.get_asset_name(index);
    }
    else
    {
        out = "<undefined>";
    }
}

void ogm::interpreter::fn::object_get_sprite(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_sprite_index);
}

void ogm::interpreter::fn::object_get_mask(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_mask_index);
}

void ogm::interpreter::fn::object_get_depth(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_depth);
}

void ogm::interpreter::fn::object_get_solid(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_solid);
}

void ogm::interpreter::fn::object_get_persistent(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_persistent);
}

void ogm::interpreter::fn::object_get_visible(VO out, V obj)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    out = static_cast<real_t>(object->m_init_visible);
}

void ogm::interpreter::fn::object_is_ancestor(VO out, V obj, V ancestor)
{
    out = false;
    id_t obji = obj.castCoerce<asset_index_t>();
    id_t ancestori = ancestor.castCoerce<asset_index_t>();

    // convert instance->object
    if (frame.instance_valid(obji))
    {
        obji = frame.get_instance(obji)->m_data.m_object_index;
    }
    if (frame.instance_valid(ancestori))
    {
        ancestori = frame.get_instance(ancestori)->m_data.m_object_index;
    }

    while (obji != k_no_asset && obji != ancestori)
    {
        AssetObject* object = frame.m_assets.get_asset<AssetObject*>(obji);
        if (object->m_parent == ancestori)
        {
            out = true;
            break;
        }
        else
        {
            obji = object->m_parent;
        }
    }
}

void ogm::interpreter::fn::object_set_sprite(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_sprite_index = v.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::object_set_mask(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_mask_index = v.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::object_set_depth(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_depth = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_persistent(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_persistent = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_visible(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_visible = v.castCoerce<real_t>();
}

void ogm::interpreter::fn::object_set_solid(VO out, V obj, V v)
{
    AssetObject* object = frame.get_asset_from_variable<AssetObject>(obj);
    object->m_init_solid = v.castCoerce<real_t>();
}
