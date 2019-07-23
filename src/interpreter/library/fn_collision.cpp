#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/geometry/Vector.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

namespace
{
    typedef Frame::CollisionEntity CollisionEntity;
    typedef ogm::collision::entity_id_t entity_id_t;
    typedef ogm::collision::Shape Shape;
}

#define frame (staticExecutor.m_frame)
#define collision (frame.m_collision)

void ogmi::fn::place_empty(VO out, V vx, V vy)
{
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();

    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;

    out = true;

    if (entity.m_shape == Shape::count)
    {
        return;
    }

    collision.iterate_entity(entity, [&out, &self_id](entity_id_t, const CollisionEntity& entity) -> bool
    {
        // if instance isn't self
        if (entity.m_payload != self_id)
        {
            out = false;
            return false;
        }

        // continue
        return true;
    });
}

void ogmi::fn::place_empty(VO out, V vx, V vy, V vex)
{
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();
    ex_instance_id_t ex = vex.castCoerce<ex_instance_id_t>();
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;
    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});

    // default return
    out = true;

    if (entity.m_shape == Shape::count)
    {
        return;
    }

    collision.iterate_entity(entity, [&out, &ex, &self_id](entity_id_t, const CollisionEntity& entity) -> bool
    {
        // if instance isn't self
        if (entity.m_payload != self_id)
        {
            // if instance matches description
            if (frame.instance_matches_ex_id(entity.m_payload, ex, staticExecutor.m_self, staticExecutor.m_other))
            {
                out = false;
                return false;
            }
        }

        // continue
        return true;
    });
}

void ogmi::fn::place_meeting(VO out, V vx, V vy, V vex)
{
    place_empty(out, vx, vy, vex);
    out = !out.get<bool>();
}

void ogmi::fn::place_free(VO out, V vx, V vy)
{
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();

    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;

    out = true;

    if (entity.m_shape == Shape::count)
    {
        return;
    }

    collision.iterate_entity(entity, [&out, &self_id](entity_id_t, const CollisionEntity& _entity) -> bool
    {
        // if instance isn't self
        if (_entity.m_payload != self_id)
        {
            // if instance is solid
            Instance* instance = frame.get_instance(_entity.m_payload);
            if (instance->m_data.m_solid)
            {
                out = false;
                return false;
            }
        }

        // continue
        return true;
    });
}

void ogmi::fn::position_empty(VO out, V vx, V vy, V vex)
{
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();
    ex_instance_id_t ex = vex.castCoerce<ex_instance_id_t>();
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;
    geometry::Vector<coord_t> position{x, y};

    // default return
    out = true;

    collision.iterate_vector(position, [&out, &ex, &self_id](entity_id_t, const CollisionEntity& entity) -> bool
    {
        // if instance isn't self
        if (entity.m_payload != self_id)
        {
            // if instance matches description
            if (frame.instance_matches_ex_id(entity.m_payload, ex, staticExecutor.m_self, staticExecutor.m_other))
            {
                out = false;
                return false;
            }
        }

        // continue
        return true;
    });
}

void ogmi::fn::position_empty(VO out, V vx, V vy)
{
    Variable all = k_all;
    position_empty(out, vx, vy, all);
    all.cleanup();
}

void ogmi::fn::position_meeting(VO out, V vx, V vy, V vex)
{
    position_empty(out, vx, vy, vex);
    out = !out.get<bool>();
}

// TODO: standardize type usage so that this undef isn't needed.
#undef collision

void ogmi::fn::instance_position(VO out, V vx, V vy, V object)
{
    ex_instance_id_t match = object.castCoerce<ex_instance_id_t>();
    geometry::Vector<coord_t> vector{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    out = k_noone;
    frame.m_collision.iterate_vector(vector,
        [&out, &match](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::instance_place(VO out, V vx, V vy, V object)
{
    ex_instance_id_t match = object.castCoerce<ex_instance_id_t>();
    geometry::Vector<coord_t> position{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    const auto& place = frame.instance_collision_entity(staticExecutor.m_self, position);
    out = k_noone;
    frame.m_collision.iterate_entity(place,
        [&out, &match](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::collision_rectangle(VO out, V vx1, V vy1, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ vx1.castCoerce<real_t>(), vy1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::Shape::rectangle,
        { p1, p2 },
        -1
    };

    out = k_noone;
    frame.m_collision.iterate_entity(collider,
        [&out, &match, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::collision_ellipse(VO out, V vx1, V vy1, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ vx1.castCoerce<real_t>(), vy1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::Shape::ellipse,
        { p1, p2 },
        -1
    };

    out = k_noone;
    frame.m_collision.iterate_entity(collider,
        [&out, &match, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::collision_circle(VO out, V vx, V vy, V vr, V vobj, V vprec, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ vx.castCoerce<real_t>() - vr.castCoerce<real_t>(), vy.castCoerce<real_t>() - vr.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx.castCoerce<real_t>() + vr.castCoerce<real_t>(), vy.castCoerce<real_t>() + vr.castCoerce<real_t>() };
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::Shape::ellipse,
        { p1, p2 },
        -1
    };

    out = k_noone;
    frame.m_collision.iterate_entity(collider,
        [&out, &match, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::collision_line(VO out, V vx, V vy, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ vx.castCoerce<real_t>(), vy.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    out = k_noone;
    frame.m_collision.iterate_line(p1, p2,
        [&out, &match, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}

void ogmi::fn::collision_point(VO out, V vx, V vy, V object, V vprec, V vnotme)
{
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = object.castCoerce<ex_instance_id_t>();
    geometry::Vector<coord_t> vector{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    out = k_noone;
    frame.m_collision.iterate_vector(vector,
        [&out, &match, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (!frame.instance_matches_ex_id(entity.m_payload, match, staticExecutor.m_self, staticExecutor.m_other))
            {
                // not what we're looking for -- continue searching
                return true;
            }

            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            // return instance id
            out = entity.m_payload;
            return false;
        }
    );
}
