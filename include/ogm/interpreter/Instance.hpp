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
    
    // built-in instance data
    struct InstanceData
    {
        ogm::id_t m_id = -1;
        asset_index_t m_object_index = -1, m_sprite_index = -1, m_mask_index = -1;
        real_t m_depth = 0;
        #ifdef OGM_LAYERS
        layer_elt_id_t m_layer_elt = -1;
        #endif
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
            const Variable& findVariable(variable_id_t id) const;

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
            void get_value(variable_id_t id, Variable& vOut) const;
            void set_value(variable_id_t id, const Variable& v);

            // as above, but for array values.
            inline void get_value_array(variable_id_t id
                #ifdef OGM_2DARRAY
                , uint32_t i
                #endif
                , uint32_t j, Variable& vOut) const
            {
                switch (id)
                {
                    case v_alarm:
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
                    case v_alarm:
                        m_data.m_alarm[j] = v.castCoerce<int32_t>();
                        break;
                    default:
                        throw MiscError("Cannot access built-in variable as an array.");
                }
            }
            
            #ifdef OGM_LAYERS
            bool layer_is_managed() const
            {
                return m_data.m_layer_elt == -1;
            }
            
            // returns k_no_layer if not on a layer.
            layer_id_t get_layer() const;
            #endif
            
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
                #ifdef OGM_LAYERS
                _serialize<write>(s, m_data.m_layer_elt);
                #endif
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
