#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

namespace
{
inline void instance_destroy_(direct_instance_id_t id)
{
    Instance* instance;
    if (frame.instance_valid(id, instance))
    {
        // run destroy event
        Frame::EventContext e = frame.m_data.m_event_context;
        frame.m_data.m_event_context.m_event = DynamicEvent::DESTROY;
        frame.m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
        frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
        bytecode_index_t index = frame.get_static_event_bytecode<ogm::asset::StaticEvent::DESTROY>(instance->m_data.m_object_index);
        staticExecutor.pushSelf(instance);
        execute_bytecode_safe(index);
        staticExecutor.popSelf();
        frame.m_data.m_event_context = e;

        // destroy properly
        frame.invalidate_instance(id);
        assert(!frame.instance_valid(id));
    }
}
}

void ogmi::fn::instance_create(VO out, V x, V y, V vobject_index)
{
    asset_index_t object_index = vobject_index.castCoerce<asset_index_t>();
    const AssetObject* object = frame.m_assets.get_asset<AssetObject*>(object_index);
    if (!object)
    {
        throw MiscError("Attempted to create non-existent object");
    }
    Instance* instance = frame.create_instance(object_index, x.castCoerce<coord_t>(), y.castCoerce<coord_t>());

    // run create event
    Frame::EventContext e = frame.m_data.m_event_context;
    frame.m_data.m_event_context.m_event = DynamicEvent::CREATE;
    frame.m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
    frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
    bytecode_index_t index = frame.get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
    staticExecutor.pushSelf(instance);
    execute_bytecode_safe(index);
    staticExecutor.popSelf();
    frame.m_data.m_event_context = e;
    out = instance->m_data.m_id;
}

void ogmi::fn::instance_destroy(VO out)
{
    instance_destroy_(staticExecutor.m_self->m_data.m_id);
}

void ogmi::fn::instance_destroy(VO out, V id)
{
    instance_destroy_(id.castCoerce<direct_instance_id_t>());
}

void ogmi::fn::instance_exists(VO out, V id)
{
    ex_instance_id_t ex_id = id.castCoerce<direct_instance_id_t>();
    Instance* instance = frame.get_instance_single(ex_id, staticExecutor.m_self, staticExecutor.m_other);
    out = !!instance;
}

void ogmi::fn::instance_number(VO out, V vobject)
{
    ex_instance_id_t object_index = vobject.castCoerce<ex_instance_id_t>();
    if (object_index == k_all || object_index == k_noone)
    {
        throw UnknownIntendedBehaviourError();
    }
    if (!frame.m_assets.get_asset<AssetObject*>(object_index))
    {
        throw UnknownIntendedBehaviourError();
    }
    out = frame.get_object_instance_count(object_index);
}

void ogmi::fn::instance_find(VO out, V vobject, V vindex)
{
    ex_instance_id_t object_index = vobject.castCoerce<ex_instance_id_t>();
    size_t index = vindex.castCoerce<size_t>();
    if (object_index = k_all || object_index == k_noone)
    {
        throw UnknownIntendedBehaviourError();
    }
    if (!frame.m_assets.get_asset<AssetObject*>(object_index))
    {
        throw UnknownIntendedBehaviourError();
    }
    size_t max = frame.get_object_instance_count(object_index);
    if (index >= max)
    {
        throw UnknownIntendedBehaviourError();
    }
    out = frame.m_object_instances.at(index)->at(max);
}

void ogmi::fn::getv::instance_count(VO out)
{
    out = frame.get_instance_count();
}

void ogmi::fn::instance_id_get(VO out, V i)
{
    getv::instance_id(out, 0, i);
}

void ogmi::fn::getv::instance_id(VO out, V i, V j)
{
    if (i == 0)
    {
        size_t n = j.castCoerce<size_t>();
        if (n < frame.get_instance_count())
        {
            out = frame.get_instance_id_nth(n);
            return;
        }
    }
    out = k_noone;
}

// instance activation

void ogmi::fn::instance_activate_all(VO out)
{
    for (std::pair<instance_id_t, Instance*> pair : frame.m_instances)
    // TODO: consider optimizing by not iterating through instances which are not in the room.
    {
        instance_id_t id = std::get<0>(pair);
        Instance* instance = std::get<1>(pair);
        if (frame.instance_valid(id))
        {
            frame.activate_instance(instance);
        }
    }
}

void ogmi::fn::instance_deactivate_all(VO out, V vnotme)
{
    bool notme = vnotme.cond();
    for (std::pair<instance_id_t, Instance*> pair : frame.m_instances)
    // TODO: consider optimizing by not iterating through instances which are not in the room.
    {
        instance_id_t id = std::get<0>(pair);
        Instance* instance = std::get<1>(pair);
        if (frame.instance_valid(id) && (!notme || instance != staticExecutor.m_self))
        {
            // OPTIMIZE: pass id and instance (rather than read id from instance).
            // same goes for the other similar calls.
            frame.deactivate_instance(instance);
        }
    }
}

void ogmi::fn::instance_activate_object(VO out, V object)
{
    // OPTIMIZE: only iterate through objects of the given description.
    // (can't use with iterators because those only iterate through active instance.)
    for (std::pair<instance_id_t, Instance*> pair : frame.m_instances)
    {
        instance_id_t id = std::get<0>(pair);
        Instance* instance = std::get<1>(pair);
        if (frame.instance_valid(id))
        {
            if (frame.instance_matches_ex_id(instance, object.castCoerce<ex_instance_id_t>(), staticExecutor.m_self, staticExecutor.m_other))
            {
                frame.activate_instance(instance);
            }
        }
    }
}

void ogmi::fn::instance_deactivate_object(VO out, V object)
{
    WithIterator wi;
    frame.get_instance_iterator(object.castCoerce<direct_instance_id_t>(), wi, staticExecutor.m_self, staticExecutor.m_other);

    while (!wi.complete())
    {
        frame.deactivate_instance(*wi);
        ++wi;
    }
}

void ogmi::fn::instance_deactivate_region(VO out, V x1, V y1, V w, V h, V vinside, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ x1.castCoerce<real_t>(), y1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> dim{ w.castCoerce<real_t>(), h.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2 = p1 + dim;
    const bool prec = false;
    const bool notme = vnotme.cond();
    const bool inside = vinside.cond();
    if (!inside)
    {
        throw MiscError("Outside-rectangle deactivation not implemented.");
    }
    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::Shape::rectangle,
        { p1, p2 },
        -1
    };

    out = k_noone;
    std::vector<instance_id_t> ids;
    frame.m_collision.iterate_entity(collider,
        [&out, &ids, &notme](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            if (notme && entity_id == staticExecutor.m_self->m_data.m_frame_collision_id)
            {
                // this is ourself -- continue
                return true;
            }

            ids.push_back(entity.m_payload);
            return true;
        }
    );

    for (instance_id_t id : ids)
    {
        frame.deactivate_instance(id);
    }
}

void ogmi::fn::instance_activate_region(VO out, V x1, V y1, V w, V h, V vinside, V vnotme)
{
    ogm::geometry::Vector<coord_t> p1{ x1.castCoerce<real_t>(), y1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> dim{ w.castCoerce<real_t>(), h.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2 = p1 + dim;
    const bool prec = false;
    const bool notme = vnotme.cond();
    const bool inside = vinside.cond();
    if (!inside)
    {
        throw MiscError("Outside-rectangle activation not implemented.");
    }
    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::Shape::rectangle,
        { p1, p2 },
        -1
    };

    // OPTIMIZE
    std::vector<instance_id_t> ids;
    for (auto& pair : frame.m_instances)
    {
        Instance* instance = pair.second;
        if (!instance->m_data.m_frame_active)
        {
            auto entity = frame.instance_collision_entity(instance, instance->m_data.m_position);
            if (collider.collides_entity(entity))
            {
                ids.push_back(pair.first);
            }
        }
    }

    for (instance_id_t id : ids)
    {
        frame.activate_instance(id);
    }
}


void ogmi::fn::instance_nearest(VO out, V x, V y, V obj)
{
    WithIterator wi;
    frame.get_instance_iterator(obj.castCoerce<direct_instance_id_t>(), wi, staticExecutor.m_self, staticExecutor.m_other);
    out = k_noone;
    if (wi.complete())
    {
        return;
    }

    double nearest = std::numeric_limits<double>::max();

    geometry::Vector<coord_t> position{ x.castCoerce<coord_t>(), y.castCoerce<coord_t>() };

    while (!wi.complete())
    {
        double dist = position.distance_to((*wi)->m_data.m_position);

        if (dist < nearest)
        {
            nearest = dist;
            out = (*wi)->m_data.m_id;
        }
        ++wi;
    }
}
