#include "ogm/interpreter/Frame.hpp"

#ifndef OGM_INSTANCE_IMPL_SPECIFIER
    #define OGM_INSTANCE_IMPL_SPECIFIER
#endif

namespace ogm::interpreter
{

#ifdef OGM_LAYERS
OGM_INSTANCE_IMPL_SPECIFIER
layer_id_t Instance::get_layer() const
{
    if (m_data.m_layer_elt == -1)
    {
        return k_no_layer;
    }
    return m_data.m_frame_owner->m_layers.m_layer_elements.at(m_data.m_layer_elt).m_layer;
}
#endif

OGM_INSTANCE_IMPL_SPECIFIER
void Instance::get_value(variable_id_t id, Variable& vOut) const
{
    #ifdef OGM_STRUCT_SUPPORT
    if (m_is_struct)
    {
        // note that we are converting from memspace_builtin_instance
        // to memspace_instance, but this is okay because they should
        // agree due to Library::reflection_add_instance_variables().
        vOut.copy(findVariable(id));
        return;
    }
    #endif
    switch(id)
    {
    // order is given in src/interpreter/library/ivars.h
    case v_id:
        vOut = static_cast<real_t>(m_data.m_id);
        break;
    case v_object_index:
        vOut = static_cast<real_t>(
            static_cast<typename std::make_signed<decltype(m_data.m_object_index)>::type>(
                m_data.m_object_index
            )
        );
        break;
    case v_depth:
        #ifdef OGM_LAYERS
        if (!m_data.m_frame_owner->m_data.m_layers_enabled || layer_is_managed())
        {
            // return instance's depth
            vOut = m_data.m_depth;
        }
        else
        {
            // return depth of instance's layer
            vOut = m_data.m_frame_owner->m_layers.m_layers.at(get_layer()).m_depth;
        }
        #else
        vOut = m_data.m_depth;
        #endif
        break;
    case v_persistent:
        vOut = m_data.m_persistent;
        break;
    case v_visible:
        vOut = m_data.m_visible;
        break;
    case v_solid:
        vOut = m_data.m_solid;
        break;
    case v_x:
        vOut = m_data.m_position.x;
        break;
    case v_y:
        vOut = m_data.m_position.y;
        break;
    case v_alarm:
        throw MiscError("alarm must be accessed as an array.");
    case v_xprevious:
        vOut = m_data.m_position_prev.x;
        break;
    case v_yprevious:
        vOut = m_data.m_position_prev.y;
        break;
    case v_xstart:
        vOut = m_data.m_position_start.x;
        break;
    case v_ystart:
        vOut = m_data.m_position_start.y;
        break;
    case v_sprite_index:
        vOut = static_cast<real_t>(
            static_cast<typename std::make_signed<decltype(m_data.m_sprite_index)>::type>(
                m_data.m_sprite_index
            )
        );
        break;
    case v_image_angle:
        vOut = m_data.m_angle;
        break;
    case v_image_blend:
        vOut = static_cast<real_t>(m_data.m_image_blend);
        break;
    case v_image_index:
        vOut = m_data.m_image_index;
        break;
    case v_image_alpha:
        vOut = m_data.m_image_alpha;
        break;
    case v_image_number:
        {
            asset::AssetSprite* sprite = m_data.m_frame_owner->m_assets.get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
            if (sprite)
            {
                vOut = static_cast<real_t>(sprite->image_count());
            }
            else
            {
                vOut = 0.0;
            }
            break;
        }
        break;
    case v_image_speed:
        vOut = m_data.m_image_speed;
        break;
    case v_image_xscale:
        vOut = m_data.m_scale.x;
        break;
    case v_image_yscale:
        vOut = m_data.m_scale.y;
        break;
    case v_mask_index:
        vOut = static_cast<real_t>(
            static_cast<typename std::make_signed<decltype(m_data.m_mask_index)>::type>(
                m_data.m_mask_index
            )
        );
        break;
    case v_friction:
        vOut = m_data.m_friction;
        break;
    case v_gravity:
        vOut = m_data.m_gravity;
        break;
    case v_gravity_direction:
        vOut = m_data.m_gravity_direction;
        break;
    case v_hspeed:
        vOut = m_data.m_speed.x;
        break;
    case v_vspeed:
        vOut = m_data.m_speed.y;
        break;
    case v_speed: // speed
        vOut = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);
        break;
    case v_direction: // direction
        vOut = m_data.m_direction;
        break;
    #ifdef OGM_LAYERS
    case v_layer:
        if (m_data.m_frame_owner->m_data.m_layers_enabled)
        {
            if (m_data.m_layer_elt == -1)
            {
                vOut = -1_r;
            }
            else
            {
                // lookup layer from layer element
                // OPTIMIZE: store layer directly in m_data as well?
                vOut = m_data.m_frame_owner->m_layers.m_layer_elements.at(m_data.m_layer_elt).m_layer;
            }
        }
        else
        {
            // FIXME: use whatever instance variable is called 'layer' instead.
            throw MiscError("layers are not enabled.");
        }
        break;
    #endif
    case v_bbox_left:
        vOut = get_bbox_left();
        break;
    case v_bbox_top:
        vOut = get_bbox_top();
        break;
    case v_bbox_right:
        vOut = get_bbox_right();
        break;
    case v_bbox_bottom:
        vOut = get_bbox_bottom();
        break;
    case v_sprite_width: // sprite_width
        {
            asset::AssetSprite* sprite =  m_data.m_frame_owner->m_assets.get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
            if (sprite)
            {
                // [sic] not absolute value
                vOut = sprite->m_dimensions.x * m_data.m_scale.x;
            }
            else
            {
                vOut = 0.0;
            }
            break;
        }
        break;
    case v_sprite_height: // sprite_height
        {
            asset::AssetSprite* sprite =  m_data.m_frame_owner->m_assets.get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
            if (sprite)
            {
                // [sic] not absolute value
                vOut = sprite->m_dimensions.y * m_data.m_scale.y;
            }
            else
            {
                vOut = 0.0;
            }
            break;
        }
        break;
    case v_sprite_xoffset: // sprite_xoffset
        {
            asset::AssetSprite* sprite =  m_data.m_frame_owner->m_assets.get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
            if (sprite)
            {
                vOut = sprite->m_offset.x * m_data.m_scale.x;
            }
            else
            {
                vOut = 0.0;
            }
            break;
        }
        break;
    case v_sprite_yoffset: // sprite_yoffset
        {
            asset::AssetSprite* sprite =  m_data.m_frame_owner->m_assets.get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
            if (sprite)
            {
                // [sic] absolute value
                vOut = sprite->m_offset.y * m_data.m_scale.y;
            }
            else
            {
                vOut = 0.0;
            }
            break;
        }
        break;
    case v_ogm_serializable:
        {
            vOut = m_data.m_serializable;
        }
    default:
        throw MiscError("Built-in variable " + std::to_string(id) + " not supported");
    }
}

OGM_INSTANCE_IMPL_SPECIFIER
void Instance::set_value(variable_id_t id, const Variable& v)
{
    #ifdef OGM_STRUCT_SUPPORT
    if (m_is_struct)
    {
        // note that we are converting from memspace_builtin_instance
        // to memspace_instance, but this is okay because they should
        // agree due to Library::reflection_add_instance_variables().
        getVariable(id).copy(v);
        return;
    }
    #endif
    switch(id)
    // order is given in src/interpreter/library/ivars.h
    {
    case v_id:
        throw MiscError("Cannot write to id.");
        break;
    case v_object_index:
        throw MiscError("Cannot write to object_index.");
        break;
    case v_depth:
        #ifdef OGM_LAYERS
        // setting depth directly causes layer to become 'managed'
        if (m_data.m_frame_owner->m_data.m_layers_enabled)
        {
            if (!layer_is_managed())
            {
                // remove from previous layer
                m_data.m_frame_owner->m_layers.element_remove(m_data.m_layer_elt);
            }
            else
            {
                // remove from 'managed instances' layer
                m_data.m_frame_owner->m_layers.managed_remove(m_data.m_depth, m_data.m_id);
            }
            
            // move to managed layer new depth.
            m_data.m_frame_owner->m_layers.managed_add(v.castCoerce<real_t>(), m_data.m_id);
            
            // no longer has associated element.
            m_data.m_layer_elt = -1;
        }
        #endif
        m_data.m_depth = v.castCoerce<real_t>();
        break;
    case v_persistent:
        m_data.m_persistent = v.castCoerce<bool_t>();
        break;
    case v_visible:
        m_data.m_visible = v.castCoerce<bool_t>();
        break;
    case v_solid:
        m_data.m_solid = v.castCoerce<bool_t>();
        break;
    case v_x:
        m_data.m_position.x = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_y:
        m_data.m_position.y = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_alarm:
        throw MiscError("alarm must be accessed as an array.");
        break;
    case v_xprevious:
        m_data.m_position_prev.x = v.castCoerce<real_t>();
        break;
    case v_yprevious:
        m_data.m_position_prev.y = v.castCoerce<real_t>();
        break;
    case v_xstart:
        m_data.m_position_start.x = v.castCoerce<real_t>();
        break;
    case v_ystart:
        m_data.m_position_start.y = v.castCoerce<real_t>();
        break;
    case v_sprite_index:
        m_data.m_sprite_index = v.castCoerce<asset_index_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_image_angle:
        m_data.m_angle = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_image_blend:
        m_data.m_image_blend = v.castCoerce<int32_t>();
        break;
    case v_image_index:
        m_data.m_image_index = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_image_alpha:
        m_data.m_image_alpha = v.castCoerce<real_t>();
        break;
    case v_image_speed:
        m_data.m_image_speed = v.castCoerce<real_t>();
        break;
    case v_image_xscale:
        m_data.m_scale.x = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_image_yscale:
        m_data.m_scale.y = v.castCoerce<real_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_mask_index:
        m_data.m_mask_index = v.castCoerce<asset_index_t>();
        m_data.m_frame_owner->queue_update_collision(this);
        break;
    case v_friction:
        m_data.m_friction = v.castCoerce<real_t>();
        break;
    case v_gravity:
        m_data.m_gravity = v.castCoerce<real_t>();
        break;
    case v_gravity_direction:
        m_data.m_gravity_direction = v.castCoerce<real_t>();
        break;
    case v_hspeed:
        m_data.m_speed.x = v.castCoerce<real_t>();
        m_data.m_direction = std::atan2(-m_data.m_speed.y, m_data.m_speed.x) * 360.0 / TAU;
        break;
    case v_vspeed:
        m_data.m_speed.y = v.castCoerce<real_t>();
        m_data.m_direction = std::atan2(-m_data.m_speed.y, m_data.m_speed.x) * 360.0 / TAU;
        break;
    case v_speed: // speed
        {
            real_t new_speed = v.castCoerce<real_t>();
            m_data.m_speed.x = std::cos(m_data.m_direction * TAU / 360.0) * new_speed;
            m_data.m_speed.y = -std::sin(m_data.m_direction * TAU / 360.0) * new_speed;
        }
        break;
    case v_direction: // direction
        {
            real_t radians_dir = v.castCoerce<real_t>() * TAU / 360.0;
            real_t speed = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);
            m_data.m_speed.x = speed * cos(radians_dir);
            m_data.m_speed.y = -speed * sin(radians_dir);
            m_data.m_direction =  v.castCoerce<real_t>();
        }
        break;
    #ifdef OGM_LAYERS
    case v_layer:
        if (m_data.m_frame_owner->m_data.m_layers_enabled)
        {
            if (!layer_is_managed())
            {
                // remove from layer
                m_data.m_frame_owner->m_layers.element_remove(m_data.m_layer_elt);
            }
            else
            {
                // remove from managed layer
                m_data.m_frame_owner->m_layers.managed_remove(m_data.m_depth, m_data.m_id);
            }
            
            // put on the given layer
            m_data.m_frame_owner->m_layers.add_element(
                LayerElement::et_instance,
                v.castCoerce<layer_id_t>()
            ).instance.m_id = m_data.m_id;
        }
        else
        {
            // FIXME: use whatever instance variable is called 'layer' instead.
            throw MiscError("layers are not enabled.");
        }
        break;
    #endif
    case v_ogm_serializable:
        {
            m_data.m_serializable = v.cond();
        }
        break;
    default:
        throw MiscError("Writing to built-in variable " + std::to_string(id) + " not supported.");
    }
}

OGM_INSTANCE_IMPL_SPECIFIER
const Variable& Instance::findVariable(variable_id_t id) const
{
    if (!hasVariable(id))
    {
        assert(m_data.m_frame_owner);
        
        // check for a static variable of the same name
        static_remap(id);
        
        return m_data.m_frame_owner->find_global_variable(id);
    }
    
    try
    {
        return m_variables.at(id);
    }
    catch (const typename decltype(m_variables)::ItemNotFoundException&)
    {
        std::string s = ".";
        if (m_data.m_frame_owner)
        {
            if (bytecode::ReflectionAccumulator* reflection = m_data.m_frame_owner->m_reflection)
            {
                if (reflection->m_namespace_instance.has_name(id))
                {
                    s = " \"" + std::string(reflection->m_namespace_instance.find_name(id)) + "\".";
                }
            }
        }
        throw MiscError("Read on unset instance variable" + s);
    }
}

}