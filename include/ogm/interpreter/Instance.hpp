#pragma once

#include "SparseContiguousMap.hpp"

#include "ogm/collision/collision.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/common/types.hpp"

namespace ogm { namespace interpreter
{
    using namespace ogm;

    // forward declarations
    class Frame;
    struct Instance;
    namespace FrameImpl
    {
        void update_collision(Frame* f, Instance*);
        inline asset::AssetTable* get_assets(Frame* f);
        bytecode::ReflectionAccumulator* get_reflection(Frame* f);
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
        real_t m_hspeed = 0, m_vspeed = 0;
        real_t m_image_index = 0;
        real_t m_image_speed = 1;
        real_t m_image_alpha = 1;
        uint32_t m_image_blend = 0xffffff;
        geometry::Vector<coord_t> m_scale{ 1, 1 };
        real_t m_angle = 0;

        geometry::Vector<coord_t> m_speed{ 0, 0 };
        real_t m_gravity = 0;
        real_t m_gravity_direction = 0;
        real_t m_friction = 0;

        int32_t m_alarm[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        // used only by friend class Frame
        // this is done for efficiency only.
        Frame* m_frame_owner = nullptr;
        real_t m_frame_depth_previous;
        bool_t m_frame_active;
        #ifndef NDEBUG
        // slated for deletion.
        bool_t m_frame_valid = true;
        #endif
        collision::entity_id_t m_frame_collision_id = -1;
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
                m_variables[id] = std::move(v);
            }

            // retrieves variable, adding it if necessary.
            Variable& getVariable(variable_id_t id)
            {
                return m_variables[id];
            }

            // retrieves variable, throwing ItemNotFoundException on failure.
            const Variable& findVariable(variable_id_t id) const
            {
                try
                {
                    return m_variables.at(id);
                }
                catch (std::out_of_range e)
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
                    throw MiscError("Read on unset variable" + s);
                }
            }

            bool hasVariable(variable_id_t id) const
            {
                return m_variables.contains(id);
            }

            const SparseContiguousMap<variable_id_t, Variable>& getVariableStore() const
            {
                return m_variables;
            }

            coord_t get_bbox_left() const;
            coord_t get_bbox_top() const;
            coord_t get_bbox_right() const;
            coord_t get_bbox_bottom() const;

            // these functions use the same variable order as is in lbrary/ivars.h
            inline void get_value(variable_id_t id, Variable& vOut) const
            {
                switch(id)
                {
                // order is given in src/interpreter/library/ivars.h
                case 0:
                    vOut = m_data.m_id;
                    break;
                case 1:
                    vOut = m_data.m_object_index;
                    break;
                case 2:
                    vOut = m_data.m_depth;
                    break;
                case 3:
                    vOut = m_data.m_persistent;
                    break;
                case 4:
                    vOut = m_data.m_visible;
                    break;
                case 5:
                    vOut = m_data.m_solid;
                    break;
                case 6:
                    vOut = m_data.m_position.x;
                    break;
                case 7:
                    vOut = m_data.m_position.y;
                    break;
                case 8:
                    throw MiscError("alarm must be accessed as an array.");
                case 9:
                    vOut = m_data.m_position_prev.x;
                    break;
                case 10:
                    vOut = m_data.m_position_prev.y;
                    break;
                case 11:
                    vOut = m_data.m_position_start.x;
                    break;
                case 12:
                    vOut = m_data.m_position_start.y;
                    break;
                case 13:
                    vOut = m_data.m_sprite_index;
                    break;
                case 14:
                    vOut = m_data.m_angle;
                    break;
                case 15:
                    vOut = m_data.m_image_blend;
                    break;
                case 16:
                    vOut = m_data.m_image_index;
                    break;
                case 17:
                    vOut = m_data.m_image_alpha;
                    break;
                case 18:
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
                        if (sprite)
                        {
                            vOut = sprite->image_count();
                        }
                        else
                        {
                            vOut = 0;
                        }
                        break;
                    }
                    break;
                case 19:
                    vOut = m_data.m_image_speed;
                    break;
                case 20:
                    vOut = m_data.m_scale.x;
                    break;
                case 21:
                    vOut = m_data.m_scale.y;
                    break;
                case 22:
                    vOut = m_data.m_mask_index;
                    break;
                case 23:
                    vOut = m_data.m_friction;
                    break;
                case 24:
                    vOut = m_data.m_gravity;
                    break;
                case 25:
                    vOut = m_data.m_gravity_direction;
                    break;
                case 26:
                    vOut = m_data.m_speed.x;
                    break;
                case 27:
                    vOut = m_data.m_speed.y;
                    break;
                case 28: // speed
                    vOut = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);;
                    break;
                case 29: // direction
                    if (m_data.m_speed.x == 0 && m_data.m_speed.y == 0)
                    {
                        vOut = 0;
                    }
                    else
                    {
                        vOut = std::atan2(-m_data.m_speed.y, m_data.m_speed.x) * 360 / TAU;
                    }
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
                            // TODO: confirm this should be absolute value
                            vOut = sprite->m_dimensions.x * std::abs(m_data.m_scale.x);
                        }
                        else
                        {
                            vOut = 0;
                        }
                        break;
                    }
                    break;
                case 46: // sprite_height
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
                        if (sprite)
                        {
                            // TODO: confirm this should be absolute value
                            vOut = sprite->m_dimensions.y * std::abs(m_data.m_scale.y);
                        }
                        else
                        {
                            vOut = 0;
                        }
                        break;
                    }
                    break;
                case 47: // sprite_xoffset
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
                        if (sprite)
                        {
                            // TODO: confirm this should be absolute value
                            vOut = sprite->m_offset.x * m_data.m_scale.x;
                        }
                        else
                        {
                            vOut = 0;
                        }
                        break;
                    }
                    break;
                case 48: // sprite_xoffset
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
                        if (sprite)
                        {
                            // TODO: confirm this should be absolute value
                            vOut = sprite->m_offset.y * m_data.m_scale.y;
                        }
                        else
                        {
                            vOut = 0;
                        }
                        break;
                    }
                    break;
                default:
                    throw MiscError("Built-in variable " + std::to_string(id) + " not supported");
                }
            }

            inline void set_value(variable_id_t id, const Variable& v)
            {
                switch(id)
                // order is given in src/interpreter/library/ivars.h
                {
                case 0:
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
                    FrameImpl::update_collision(m_data.m_frame_owner, this);
                    break;
                case 7:
                    m_data.m_position.y = v.castCoerce<real_t>();
                    FrameImpl::update_collision(m_data.m_frame_owner, this);
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
                case 13:
                    m_data.m_sprite_index = v.castCoerce<asset_index_t>();
                    FrameImpl::update_collision(m_data.m_frame_owner, this);
                    break;
                case 14:
                    m_data.m_angle = v.castCoerce<real_t>();
                    FrameImpl::update_collision(m_data.m_frame_owner, this);
                    break;
                case 15:
                    m_data.m_image_blend = v.castCoerce<int32_t>();
                    break;
                case 16:
                    {
                        asset::AssetSprite* sprite = FrameImpl::get_assets(m_data.m_frame_owner)->get_asset<asset::AssetSprite*>(m_data.m_sprite_index);
                        if (sprite)
                        {
                            m_data.m_image_index = positive_modulo<real_t>(v.castCoerce<real_t>(), sprite->image_count());
                        }
                        else
                        {
                            m_data.m_image_index = 0;
                        }
                    }
                    break;
                case 17:
                    m_data.m_image_alpha = v.castCoerce<real_t>();
                    break;
                case 19:
                    m_data.m_image_speed = v.castCoerce<real_t>();
                    break;
                case 20:
                    m_data.m_scale.x = v.castCoerce<real_t>();
                    break;
                case 21:
                    m_data.m_scale.y = v.castCoerce<real_t>();
                    break;
                case 22:
                    m_data.m_mask_index = v.castCoerce<asset_index_t>();
                    FrameImpl::update_collision(m_data.m_frame_owner, this);
                    break;
                case 23:
                    m_data.m_friction = v.castCoerce<real_t>();
                    break;
                case 24:
                    m_data.m_gravity = v.castCoerce<real_t>();
                    break;
                case 25:
                    m_data.m_gravity_direction = v.castCoerce<real_t>();
                    break;
                case 26:
                    m_data.m_speed.x = v.castCoerce<real_t>();
                    break;
                case 27:
                    m_data.m_speed.y = v.castCoerce<real_t>();
                    break;
                case 28: // speed
                    {
                        real_t prev_speed = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);
                        real_t new_speed = v.castCoerce<real_t>();
                        if (prev_speed == 0)
                        {
                            m_data.m_speed.x = new_speed;
                        }
                        else
                        {
                            real_t scalar = new_speed / prev_speed;
                            m_data.m_speed.x *= scalar;
                            m_data.m_speed.y *= scalar;
                        }
                    }
                    break;
                case 29: // direction
                    {
                        real_t radians_dir = v.castCoerce<real_t>() * TAU / 360.0;
                        real_t speed = std::sqrt(m_data.m_speed.x * m_data.m_speed.x + m_data.m_speed.y * m_data.m_speed.y);
                        m_data.m_speed.x = speed * cos(radians_dir);
                        m_data.m_speed.y = -speed * sin(radians_dir);
                    }
                    break;
                default:
                    throw MiscError("Writing to built-in variable " + std::to_string(id) + " not supported.");
                }
            }

            // as above, but for array values.
            inline void get_value_array(variable_id_t id, uint32_t i, uint32_t j, Variable& vOut) const
            {
                switch (id)
                {
                    case 8:
                        vOut = m_data.m_alarm[j];
                        break;
                    default:
                        throw MiscError("Cannot access built-in variable as an array.");
                }
            }

            // as above, but for array values.
            inline void set_value_array(variable_id_t id, uint32_t i, uint32_t j, const Variable& v)
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

        public:
            // this could be a template type to support
            // different OGM implementations.
            InstanceData m_data;

        private:
            // instance variables
            SparseContiguousMap<variable_id_t, Variable> m_variables;
    };
}}
