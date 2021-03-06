#pragma once

#include "SparseContiguousMap.hpp"
#include "InstanceVariables.hpp"

#include "ogm/interpreter/Variable.hpp"

#include "ogm/collision/collision.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/common/types.hpp"

namespace ogm::interpreter
{
    using namespace ogm;

    // forward declarations
    class GCNode;
    class Frame;
    class Instance;
    namespace FrameImpl
    {
        void queue_update_collision(Frame* f, Instance*);
        asset::AssetTable* get_assets(Frame* f);
        bytecode::ReflectionAccumulator* get_reflection(Frame* f);
        const Variable& find_global(Frame* f, variable_id_t id);
    }
    
    // built-in instance data
    struct InstanceData
    {
        ogm::id_t m_id = -1;
        asset_index_t m_object_index = -1, m_sprite_index = -1, m_mask_index = -1;
        real_t m_depth = 0;
        bool m_visible = true, m_persistent = false, m_solid = false;
        geometry::Vector<coord_t> m_position{ 0, 0 };
        geometry::Vector<coord_t> m_position_prev{ 0, 0 };
        geometry::Vector<coord_t> m_position_start{ 0, 0 };
        real_t m_image_index = 0;
        real_t m_image_speed = 1;
        real_t m_image_alpha = 1;
        uint32_t m_image_blend = 0xffffff;
        geometry::Vector<coord_t> m_scale{ 1, 1 };

        // rotation of this instance in left-handed degrees.
        real_t m_angle = 0;

        geometry::Vector<coord_t> m_speed{ 0, 0 };
        real_t m_direction = 0; // direction of m_speed, unless m_speed is 0. In left-handed degrees
        real_t m_gravity = 0; // pixels per frame per frame
        real_t m_gravity_direction = 0; // left-handed degrees
        real_t m_friction = 0;

        int32_t m_alarm[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        // used only by friend class Frame
        // this is done for efficiency only.
        Frame* m_frame_owner = nullptr;
        real_t m_frame_depth_previous;
        bool_t m_frame_active;
        bool_t m_frame_in_instance_vectors;

        // TODO: delete this.
        bool_t m_frame_valid = true;

        collision::entity_id_t m_frame_collision_id = -1;
        collision::entity_id_t m_frame_inactive_collision_id = -1;

        #ifdef QUEUE_COLLISION_UPDATES
        bool m_collision_queued = false;
        #endif

        bool_t m_input_listener;
        bool_t m_async_listener;

        bool_t m_serializable = true;
    };

    class Instance
    {
        friend class Frame;
        public:
            Instance()
                : m_variables()
            { }

            Instance(const Instance& other)=delete;
            Instance& operator=(const Instance& other)=delete;

            void storeVariable(variable_id_t id , Variable&& v)
            {
                #ifdef OGM_GARBAGE_COLLECTOR
                #ifdef OGM_STRUCT_SUPPORT
                if (m_is_struct)
                    m_gc_node->add_reference(v.get_gc_node());
                else
                #endif
                // this will be decremented when cleanup'd.
                v.make_root();
                #endif

                m_variables[id] = std::move(v);
            }

            // retrieves variable, adding it if necessary.
            Variable& getVariable(variable_id_t id)
            {
                return m_variables[id];
            }

            // retrieves variable, throwing an error on failure.
            const Variable& findVariable(variable_id_t id) const
            {
                if (!hasVariable(id))
                {
                    assert(m_data.m_frame_owner);
                    
                    // check for a static variable of the same name
                    static_remap(id);
                    
                    return FrameImpl::find_global(m_data.m_frame_owner, id);
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
                        if (bytecode::ReflectionAccumulator* reflection = FrameImpl::get_reflection(m_data.m_frame_owner))
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

            bool hasVariable(variable_id_t id) const
            {
                return m_variables.contains(id);
            }

            // copies other instance's variables into this.
            void copyVariables(const Instance* other)
            {
                for (const auto& pair : other->m_variables)
                {
                    Variable v;
                    v.copy(pair.second);
                    storeVariable(pair.first, std::move(v));
                }
            }

            // returns the container for variables.
            const SparseContiguousMap<variable_id_t, Variable>& getVariableStore() const
            {
                return m_variables;
            }

            coord_t get_bbox_left() const;
            coord_t get_bbox_top() const;
            coord_t get_bbox_right() const;
            coord_t get_bbox_bottom() const;
            
            #ifdef OGM_STRUCT_SUPPORT
            bool static_remap(variable_id_t& id) const;
            #endif

            // these functions use the same variable order as is in library/ivars.h
            inline void get_value(variable_id_t id, Variable& vOut) const
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
                        static_cast<std::make_signed<decltype(m_data.m_object_index)>::type>(
                            m_data.m_object_index
                        )
                    );
                    break;
                case v_depth:
                    vOut = m_data.m_depth;
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
                        static_cast<std::make_signed<decltype(m_data.m_sprite_index)>::type>(
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
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
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
                        static_cast<std::make_signed<decltype(m_data.m_mask_index)>::type>(
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
                    vOut = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);;
                    break;
                case v_direction: // direction
                    vOut = m_data.m_direction;
                    break;
                case 41:
                    vOut = get_bbox_left();
                    break;
                case 42:
                    vOut = get_bbox_top();
                    break;
                case 43:
                    vOut = get_bbox_right();
                    break;
                case 44:
                    vOut = get_bbox_bottom();
                    break;
                case 45: // sprite_width
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
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
                case 46: // sprite_height
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
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
                case 47: // sprite_xoffset
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
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
                case 48: // sprite_yoffset
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
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

            inline void set_value(variable_id_t id, const Variable& v)
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
                case 1:
                    throw MiscError("Cannot write to object_index.");
                    break;
                case 2:
                    m_data.m_depth = v.castCoerce<real_t>();
                    break;
                case 3:
                    m_data.m_persistent = v.castCoerce<bool_t>();
                    break;
                case 4:
                    m_data.m_visible = v.castCoerce<bool_t>();
                    break;
                case 5:
                    m_data.m_solid = v.castCoerce<bool_t>();
                    break;
                case 6:
                    m_data.m_position.x = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case 7:
                    m_data.m_position.y = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case 8:
                    throw MiscError("alarm must be accessed as an array.");
                    break;
                case 9:
                    m_data.m_position_prev.x = v.castCoerce<real_t>();
                    break;
                case 10:
                    m_data.m_position_prev.y = v.castCoerce<real_t>();
                    break;
                case 11:
                    m_data.m_position_start.x = v.castCoerce<real_t>();
                    break;
                case 12:
                    m_data.m_position_start.y = v.castCoerce<real_t>();
                    break;
                case v_sprite_index:
                    m_data.m_sprite_index = v.castCoerce<asset_index_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case v_image_angle:
                    m_data.m_angle = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case v_image_blend:
                    m_data.m_image_blend = v.castCoerce<int32_t>();
                    break;
                case v_image_index:
                    m_data.m_image_index = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case v_image_alpha:
                    m_data.m_image_alpha = v.castCoerce<real_t>();
                    break;
                case v_image_speed:
                    m_data.m_image_speed = v.castCoerce<real_t>();
                    break;
                case v_image_xscale:
                    m_data.m_scale.x = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case v_image_yscale:
                    m_data.m_scale.y = v.castCoerce<real_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
                    break;
                case v_mask_index:
                    m_data.m_mask_index = v.castCoerce<asset_index_t>();
                    FrameImpl::queue_update_collision(m_data.m_frame_owner, this);
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
                case v_ogm_serializable:
                    {
                        m_data.m_serializable = v.cond();
                    }
                    break;
                default:
                    throw MiscError("Writing to built-in variable " + std::to_string(id) + " not supported.");
                }
            }

            // as above, but for array values.
            inline void get_value_array(variable_id_t id
                #ifdef OGM_2DARRAY
                , uint32_t i
                #endif
                , uint32_t j, Variable& vOut) const
            {
                switch (id)
                {
                    case 8:
                        vOut = static_cast<real_t>(m_data.m_alarm[j]);
                        break;
                    default:
                        throw MiscError("Cannot access built-in variable as an array.");
                }
            }

            // as above, but for array values.
            inline void set_value_array(variable_id_t id,
                #ifdef OGM_2DARRAY
                uint32_t i,
                #endif
                uint32_t j, const Variable& v)
            {
                switch (id)
                {
                    case 8:
                        m_data.m_alarm[j] = v.castCoerce<int32_t>();
                        break;
                    default:
                        throw MiscError("Cannot access built-in variable as an array.");
                }
            }
            
            #ifdef OGM_STRUCT_SUPPORT
            // retrieves this instance as a struct variable
            // (requires m_is_struct)
            Variable get_struct() const;
            #endif

            // it's up to the caller to respect m_data.m_serializable
            template<bool write>
            void serialize(typename state_stream<write>::state_stream_t& s)
            {
                // OPTIMIZE: id information is technically redundent
                // in the site where this is used...
                _serialize<write>(s, m_data.m_id);
                _serialize<write>(s, m_data.m_object_index);
                _serialize<write>(s, m_data.m_sprite_index);
                _serialize<write>(s, m_data.m_mask_index);
                _serialize<write>(s, m_data.m_depth);
                _serialize<write>(s, m_data.m_visible);
                _serialize<write>(s, m_data.m_solid);
                _serialize<write>(s, m_data.m_persistent);
                _serialize<write>(s, m_data.m_position);
                _serialize<write>(s, m_data.m_position_prev);
                _serialize<write>(s, m_data.m_position_start);
                _serialize<write>(s, m_data.m_image_index);
                _serialize<write>(s, m_data.m_image_speed);
                _serialize<write>(s, m_data.m_image_alpha);
                _serialize<write>(s, m_data.m_image_blend);
                _serialize<write>(s, m_data.m_scale);
                _serialize<write>(s, m_data.m_angle);
                _serialize<write>(s, m_data.m_speed);
                _serialize<write>(s, m_data.m_direction);
                _serialize<write>(s, m_data.m_gravity);
                _serialize<write>(s, m_data.m_gravity_direction);
                _serialize<write>(s, m_data.m_friction);
                _serialize<write>(s, m_data.m_alarm); // TODO: optimize this...

                // (don't serialize m_frame_owner, it won't change.)

                _serialize<write>(s, m_data.m_frame_depth_previous);
                _serialize<write>(s, m_data.m_frame_active);
                _serialize<write>(s, m_data.m_frame_in_instance_vectors);
                _serialize<write>(s, m_data.m_frame_valid);
                _serialize<write>(s, m_data.m_frame_collision_id);
                _serialize<write>(s, m_data.m_frame_inactive_collision_id);

                #ifdef QUEUE_COLLISION_UPDATES
                // (it's expected that m_collision_queued will be false,
                //  since we can process collision updates before serializing.)
                if (!write)
                {
                    m_data.m_collision_queued = false;
                }
                #endif

                _serialize<write>(s, m_data.m_input_listener);
                _serialize<write>(s, m_data.m_async_listener);

                m_variables.template serialize<write>(s);
            }
            
            void gc_integrity_check() const;
            
            // clears all instance variables.
            void clear_variables()
            {
                for (auto& [id, variable] : m_variables)
                {
                    #if defined(OGM_GARBAGE_COLLECTOR) && defined(OGM_STRUCT_SUPPORT)
                    // not needed because gc nodes will handle this kind of deletion themselves.
                    /*
                    if (m_gc_node)
                    {
                        m_gc_node->remove_reference(variable.get_gc_node());
                    }
                    */
                    #endif
                    // not needed due to cleanup:
                    // variable.make_not_root();
                    variable.cleanup();
                }
                m_variables.clear();
            }

            ~Instance()
            {
                clear_variables();
            }

        public:
            // this could be a template type to support
            // different OGM implementations.
            InstanceData m_data;
            
            #ifdef OGM_STRUCT_SUPPORT
            bool m_is_struct = false;
            bytecode_index_t m_struct_type = bytecode::k_no_bytecode;
            #ifdef OGM_GARBAGE_COLLECTOR
            GCNode* m_gc_node = nullptr;
            #else
            VariableStructData* m_struct_data = nullptr;
            #endif
            #endif

        private:
            // instance variables
            SparseContiguousMap<variable_id_t, Variable> m_variables;
    };
}
