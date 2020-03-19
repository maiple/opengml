#include "libpre.h"
    #include "fn_instance.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

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
        staticExecutor.pushSelfDouble(instance);
        execute_bytecode_safe(index);
        staticExecutor.popSelfDouble();
        frame.m_data.m_event_context = e;

        // destroy properly
        frame.invalidate_instance(id);
        ogm_assert(!frame.instance_valid(id));
    }
}
}

void ogm::interpreter::fn::instance_create(VO out, V vobject_index)
{
    instance_create(out, 0, 0, v_object_index);
}

void ogm::interpreter::fn::instance_create(VO out, V x, V y, V vobject_index)
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
    staticExecutor.pushSelfDouble(instance);
    execute_bytecode_safe(index);
    staticExecutor.popSelfDouble();
    frame.m_data.m_event_context = e;
    out = static_cast<real_t>(instance->m_data.m_id);
}

void ogm::interpreter::fn::instance_change(VO out, V vobject_index, V events)
{
    asset_index_t object_index = vobject_index.castCoerce<asset_index_t>();
    const AssetObject* object = frame.m_assets.get_asset<AssetObject*>(object_index);
    if (!object)
    {
        throw MiscError("Attempted to change instance to non-existent object");
    }

    Instance* instance = staticExecutor.m_self;

    const AssetObject* prevobject = frame.m_assets.get_asset<AssetObject*>(instance->m_data.m_object_index);
    ogm_assert(prevobject);

    // run destroy event
    if (events.cond())
    {
        Frame::EventContext e = frame.m_data.m_event_context;
        // set event context
        frame.m_data.m_event_context.m_event = DynamicEvent::DESTROY;
        frame.m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
        frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
        bytecode_index_t index = frame.get_static_event_bytecode<ogm::asset::StaticEvent::DESTROY>(prevobject);
        execute_bytecode_safe(index);
        // restore event context
        frame.m_data.m_event_context = e;
    }

    // change instance
    frame.change_instance(instance->m_data.m_id, object_index);

    // run create event
    if (events.cond())
    {
        Frame::EventContext e = frame.m_data.m_event_context;
        // set event context
        frame.m_data.m_event_context.m_event = DynamicEvent::CREATE;
        frame.m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
        frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
        bytecode_index_t index = frame.get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
        execute_bytecode_safe(index);
        // restore event context
        frame.m_data.m_event_context = e;
    }
}

void ogm::interpreter::fn::instance_copy(VO out, V events)
{
    Instance* self = staticExecutor.m_self;
    const AssetObject* object = frame.m_assets.get_asset<AssetObject*>(self->m_data.m_object_index);
    ogm_assert(object);
    Instance* newinstance = frame.create_instance(
        self->m_data.m_object_index,
        self->m_data.m_position.x,
        self->m_data.m_position.y
    );

    if (!frame.instance_active(self) || !frame.instance_valid(self))
    {
        throw MiscError("Can't copy inactive instances.");
    }

    // transfer variables over
    newinstance->copyVariables(self);

    // transfer builtin variables over
    for (size_t i = 0; i < INSTANCE_VARIABLE_MAX; ++i)
    {
        Variable v;
        // skip the alarm variable, which requires special treatment to copy.
        // skip image_number, which transfers with sprite_index
        // also skip id (not copied) and object_index (already set)
        if (i != v_alarm && i != v_image_number && i != v_id && i != v_object_index)
        {
            self->get_value(i, v);
            newinstance->set_value(i, std::move(v));
        }
    }

    // copy alarms over (special, because it's an array)
    std::copy(
        std::begin(self->m_data.m_alarm),
        std::end(self->m_data.m_alarm),
        std::begin(newinstance->m_data.m_alarm)
    );

    // run create event
    if (events.cond())
    {
        Frame::EventContext e = frame.m_data.m_event_context;
        frame.m_data.m_event_context.m_event = DynamicEvent::CREATE;
        frame.m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
        frame.m_data.m_event_context.m_object = newinstance->m_data.m_object_index;
        bytecode_index_t index = frame.get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
        staticExecutor.pushSelfDouble(newinstance);
        execute_bytecode_safe(index);
        staticExecutor.popSelfDouble();
        frame.m_data.m_event_context = e;
    }

    out = static_cast<real_t>(newinstance->m_data.m_id);
}

void ogm::interpreter::fn::instance_destroy(VO out)
{
    instance_destroy_(staticExecutor.m_self->m_data.m_id);
}

void ogm::interpreter::fn::instance_destroy(VO out, V id)
{
    instance_destroy_(id.castCoerce<direct_instance_id_t>());
}

void ogm::interpreter::fn::instance_exists(VO out, V id)
{
    ex_instance_id_t ex_id = id.castCoerce<direct_instance_id_t>();
    Instance* instance = frame.get_instance_single(ex_id, staticExecutor.m_self, staticExecutor.m_other);
    if (instance)
    {
        out = frame.instance_valid(instance);
    }
    else
    {
        out = false;
    }
}

void ogm::interpreter::fn::instance_number(VO out, V vobject)
{
    ex_instance_id_t object_index = vobject.castCoerce<ex_instance_id_t>();
    if (object_index == k_noone)
    {
        out = 0.0;
        return;
    }
    if (object_index == k_all)
    {
        out = static_cast<real_t>(frame.get_instance_count());
        return;
    }
    if (!frame.m_assets.get_asset<AssetObject*>(object_index))
    {
        throw UnknownIntendedBehaviourError();
    }
    out = static_cast<real_t>(frame.get_object_instance_count(object_index));
}

void ogm::interpreter::fn::instance_find(VO out, V vobject, V vindex)
{
    ex_instance_id_t object_index = vobject.castCoerce<ex_instance_id_t>();
    size_t index = vindex.castCoerce<size_t>();
    if (object_index == k_all || object_index == k_noone)
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
    out = static_cast<real_t>(frame.m_object_instances.at(object_index)->at(index)->m_data.m_id);
}

void ogm::interpreter::fn::getv::instance_count(VO out)
{
    out = static_cast<real_t>(frame.get_instance_count());
}

void ogm::interpreter::fn::instance_id_get(VO out, V i)
{
    getv::instance_id(out, 0, i);
}

void ogm::interpreter::fn::getv::instance_id(VO out, V i, V j)
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
    out = static_cast<real_t>(k_noone);
}

// instance activation

void ogm::interpreter::fn::instance_activate_all(VO out)
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

void ogm::interpreter::fn::instance_deactivate_all(VO out, V vnotme)
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

void ogm::interpreter::fn::instance_activate_object(VO out, V object)
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

void ogm::interpreter::fn::instance_deactivate_object(VO out, V object)
{
    WithIterator wi;
    frame.get_instance_iterator(object.castCoerce<direct_instance_id_t>(), wi, staticExecutor.m_self, staticExecutor.m_other);

    while (!wi.complete())
    {
        if (frame.instance_active(*wi))
        {
            frame.deactivate_instance(*wi);
        }
        ++wi;
    }
}

void ogm::interpreter::fn::instance_deactivate_region(VO out, V x1, V y1, V w, V h, V vinside, V vnotme)
{
    frame.process_collision_updates();
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
        ogm::collision::ShapeType::rectangle,
        { p1, p2 },
        -1
    };

    out = static_cast<real_t>(k_noone);
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
        },
        prec
    );

    for (instance_id_t id : ids)
    {
        frame.deactivate_instance(id);
    }
}

void ogm::interpreter::fn::instance_activate_region(VO out, V x1, V y1, V w, V h, V vinside)
{
    ogm::geometry::Vector<coord_t> p1{ x1.castCoerce<real_t>(), y1.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> dim{ w.castCoerce<real_t>(), h.castCoerce<real_t>() };
    ogm::geometry::Vector<coord_t> p2 = p1 + dim;
    const bool inside = vinside.cond();
    if (!inside)
    {
        throw MiscError("Outside-rectangle activation not implemented.");
    }
    const ogm::collision::Entity<coord_t, direct_instance_id_t> collider
    {
        ogm::collision::ShapeType::rectangle,
        { p1, p2 },
        -1
    };

    out = static_cast<real_t>(k_noone);
    std::vector<instance_id_t> ids;
    frame.m_inactive_collision.iterate_entity(collider,
        [&out, &ids](collision::entity_id_t entity_id, const collision::Entity<coord_t, direct_instance_id_t>& entity) -> bool
        {
            ids.push_back(entity.m_payload);
            return true;
        },
        false
    );

    for (instance_id_t id : ids)
    {
        frame.activate_instance(id);
    }
}


void ogm::interpreter::fn::instance_nearest(VO out, V x, V y, V obj)
{
    frame.process_collision_updates();
    WithIterator wi;
    frame.get_instance_iterator(obj.castCoerce<direct_instance_id_t>(), wi, staticExecutor.m_self, staticExecutor.m_other);
    out = static_cast<real_t>(k_noone);
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
            out = static_cast<real_t>((*wi)->m_data.m_id);
        }
        ++wi;
    }
}
