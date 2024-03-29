#include "ogm/common/types.hpp"
#include "ogm/interpreter/Frame.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "Instance_Impl.inc"

namespace ogm::interpreter
{
coord_t Instance::get_bbox_left() const
{
    Frame::CollisionEntity entity = m_data.m_frame_owner->instance_collision_entity(this, m_data.m_position);
    return floor(entity.m_aabb.m_start.x);
}

coord_t Instance::get_bbox_top() const
{
    Frame::CollisionEntity entity = m_data.m_frame_owner->instance_collision_entity(this, m_data.m_position);
    return floor(entity.m_aabb.m_start.y);
}

coord_t Instance::get_bbox_right() const
{
    Frame::CollisionEntity entity = m_data.m_frame_owner->instance_collision_entity(this, m_data.m_position);
    return ceil(entity.m_aabb.m_end.x) - 1;
}

coord_t Instance::get_bbox_bottom() const
{
    Frame::CollisionEntity entity = m_data.m_frame_owner->instance_collision_entity(this, m_data.m_position);
    return ceil(entity.m_aabb.m_end.y) - 1;
}

void Instance::gc_integrity_check() const
{
    for (auto& [id, v] : m_variables)
    {
        v.gc_integrity_check();
    }
}

#ifdef OGM_STRUCT_SUPPORT
bool Instance::static_remap(variable_id_t& id) const
{
    return m_data.m_frame_owner->m_bytecode.lookup_static(m_struct_type, id);
}

Variable Instance::get_struct() const
{
    ogm_assert(m_is_struct);
    
    Variable v;
    v.set_from_struct(
        #ifdef OGM_GARBAGE_COLLECTOR
        static_cast<VariableStructData*>(
            m_gc_node->get()
        )
        #else
        m_struct_data
        #endif
    );
    return v;
}
#endif

}
