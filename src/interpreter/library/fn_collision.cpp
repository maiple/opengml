#include "libpre.h"
    #include "fn_collision.h"
    #include "fn_math.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/geometry/Vector.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

namespace
{
    typedef Frame::CollisionEntity CollisionEntity;
    typedef ogm::collision::entity_id_t entity_id_t;
    typedef ogm::collision::ShapeType ShapeType;
}

#define frame (staticExecutor.m_frame)
#define collision (frame.m_collision)

void ogm::interpreter::fn::place_empty(VO out, V vx, V vy)
{
    frame.process_collision_updates();
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();

    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;

    out = true;

    if (entity.m_shape == ShapeType::count)
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
    }, true);
}

void ogm::interpreter::fn::place_empty(VO out, V vx, V vy, V vex)
{
    frame.process_collision_updates();
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();
    ex_instance_id_t ex = vex.castCoerce<ex_instance_id_t>();
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;
    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});

    // default return
    out = true;

    if (entity.m_shape == ShapeType::count)
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
    }, true);
}

void ogm::interpreter::fn::place_meeting(VO out, V vx, V vy, V vex)
{
    place_empty(out, vx, vy, vex);
    out = !out.get<bool>();
}

void ogm::interpreter::fn::place_free(VO out, V vx, V vy)
{
    frame.process_collision_updates();
    coord_t x = vx.castCoerce<coord_t>();
    coord_t y = vy.castCoerce<coord_t>();

    CollisionEntity entity = frame.instance_collision_entity(staticExecutor.m_self, {x, y});
    entity_id_t self_id = staticExecutor.m_self->m_data.m_id;

    out = true;

    if (entity.m_shape == ShapeType::count)
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
    }, true);
}

void ogm::interpreter::fn::position_empty(VO out, V vx, V vy, V vex)
{
    frame.process_collision_updates();
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
    }, true);
}

void ogm::interpreter::fn::position_empty(VO out, V vx, V vy)
{
    Variable all = k_all;
    position_empty(out, vx, vy, all);
    all.cleanup();
}

void ogm::interpreter::fn::position_meeting(VO out, V vx, V vy, V vex)
{
    position_empty(out, vx, vy, vex);
    out = !out.get<bool>();
}

// TODO: standardize type usage so that this undef isn't needed.
#undef collision

void ogm::interpreter::fn::instance_position(VO out, V vx, V vy, V object)
{
    frame.process_collision_updates();
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        true
    );
}

void ogm::interpreter::fn::instance_place(VO out, V vx, V vy, V object)
{
    frame.process_collision_updates();
    ex_instance_id_t match = object.castCoerce<ex_instance_id_t>();
    geometry::Vector<coord_t> position{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    const auto& place = frame.instance_collision_entity(staticExecutor.m_self, position);
    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        true
    );
}

void ogm::interpreter::fn::collision_rectangle(VO out, V vx1, V vy1, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    frame.process_collision_updates();
    ogm::geometry::Vector<coord_t> p1{ vx1.castCoerce<real_t>(), vy1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    ogm::geometry::AABB<coord_t> aabb{ p1, p2 };
    aabb.correct_sign();
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::ShapeType::rectangle,
        aabb,
        -1
    };

    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        prec
    );
}

void ogm::interpreter::fn::collision_ellipse(VO out, V vx1, V vy1, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    frame.process_collision_updates();
    ogm::geometry::Vector<coord_t> p1{ vx1.castCoerce<real_t>(), vy1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    ogm::geometry::AABB<coord_t> aabb{ p1, p2 };
    aabb.correct_sign();
    ogm::geometry::Ellipse<coord_t> ellipse(aabb);
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::ShapeType::ellipse,
        aabb,
        std::move(ellipse),
        -1
    };

    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        prec
    );
}

void ogm::interpreter::fn::collision_circle(VO out, V vx, V vy, V vr, V vobj, V vprec, V vnotme)
{
    frame.process_collision_updates();
    ogm::geometry::Vector<coord_t> p1{ vx.castCoerce<real_t>() - vr.castCoerce<real_t>(), vy.castCoerce<real_t>() - vr.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx.castCoerce<real_t>() + vr.castCoerce<real_t>(), vy.castCoerce<real_t>() + vr.castCoerce<real_t>() };
    ogm::geometry::AABB<coord_t> aabb{ p1, p2 };
    aabb.correct_sign();
    ogm::geometry::Ellipse<coord_t> ellipse(aabb);
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::ShapeType::ellipse,
        aabb,
        std::move(ellipse),
        -1
    };

    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        prec
    );
}

void ogm::interpreter::fn::collision_line(VO out, V vx, V vy, V vx2, V vy2, V vobj, V vprec, V vnotme)
{
    frame.process_collision_updates();
    ogm::geometry::Vector<coord_t> p1{ vx.castCoerce<real_t>(), vy.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2{ vx2.castCoerce<real_t>(), vy2.castCoerce<real_t>() };
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = vobj.castCoerce<ex_instance_id_t>();

    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        prec
    );
}

void ogm::interpreter::fn::collision_point(VO out, V vx, V vy, V object, V vprec, V vnotme)
{
    frame.process_collision_updates();
    bool prec = vprec.cond();
    bool notme = vnotme.cond();
    ex_instance_id_t match = object.castCoerce<ex_instance_id_t>();
    geometry::Vector<coord_t> vector{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>(entity.m_payload);
            return false;
        },
        prec
    );
}

void ogm::interpreter::fn::distance_to_point(VO out, V vx, V vy)
{
    frame.process_collision_updates();
    geometry::Vector<coord_t> p{ vx.castCoerce<coord_t>(), vy.castCoerce<coord_t>() };
    collision::Entity<coord_t, direct_instance_id_t> entity = frame.instance_collision_entity(staticExecutor.m_self);

    if (entity.m_shape == collision::ShapeType::count)
    // no collision for this entity
    {
        // return distance from x, y instead.
        point_distance(out, staticExecutor.m_self->m_data.m_position.x, staticExecutor.m_self->m_data.m_position.y, vx, vy);
        return;
    }
    else
    {
        geometry::AABB<coord_t>& aabb = entity.m_aabb;
        geometry::Vector<coord_t> c = aabb.get_center();
        if (aabb.contains(p))
        {
            out = static_cast<real_t>(0);
            return;
        }

        // exists horizontal line to aabb.
        if (aabb.contains({c.x, p.y}))
        {
            if (p.y >= aabb.m_end.y)
            {
                out = static_cast<real_t>(p.y - aabb.m_end.y);
                return;
            }
            else
            {
                out = static_cast<real_t>(aabb.m_start.y - p.y);
                return;
            }
        }

        // exists vertical line to aabb.
        if (aabb.contains({p.x, c.y}))
        {
            if (p.x >= aabb.m_end.x)
            {
                out = static_cast<real_t>(p.x - aabb.m_end.x);
                return;
            }
            else
            {
                out = static_cast<real_t>(aabb.m_start.x - p.x);
                return;
            }
        }

        // find nearest of aabb corners.
        if (p.x <= aabb.m_start.x && p.y <= aabb.m_start.y)
        {
            out = static_cast<real_t>(p.distance_to(aabb.top_left()));
            return;
        }

        if (p.x <= aabb.m_start.x && p.y >= aabb.m_end.y)
        {
            out = static_cast<real_t>(p.distance_to(aabb.bottom_left()));
            return;
        }

        if (p.x >= aabb.m_end.x && p.y <= aabb.m_start.y)
        {
            out = static_cast<real_t>(p.distance_to(aabb.top_right()));
            return;
        }

        if (p.x >= aabb.m_end.x && p.y >= aabb.m_end.y)
        {
            out = static_cast<real_t>(p.distance_to(aabb.bottom_right()));
            return;
        }
    }
}

void ogm::interpreter::fn::distance_to_object(VO out, V vobject)
{
    Variable dummy;

    std::vector<real_t> distances;
    WithIterator wi;
    frame.get_instance_iterator(vobject.castCoerce<ex_instance_id_t>(), wi, staticExecutor.m_self, staticExecutor.m_other);

    while (!wi.complete())
    {
        Instance* in = *wi;
        if (frame.instance_active(in))
        {
            // TODO: properly compare distances between BOTH bboxes.
            // (this is incorrect behaviour currently!)
            distance_to_point(dummy, in->m_data.m_position.x, in->m_data.m_position.y);
            distances.emplace_back(dummy.castCoerce<real_t>());
        }
        ++wi;
    }

    if (distances.empty())
    {
        // TODO: verify
        out = -1.0;
    }
    else
    {
        auto iter = std::min_element(distances.begin(), distances.end());
        out = *iter;
    }

    // not actually necessary since it's real or undefined.
    dummy.cleanup();
}
