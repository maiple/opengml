#include "ogm/common/types.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "ogm/interpreter/Frame.hpp"

namespace ogmi
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
}
