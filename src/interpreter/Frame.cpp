#include "ogm/interpreter/Frame.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <algorithm>

namespace ogmi
{
using namespace ogm;
Instance* Frame::create_instance_as(instance_id_t id, asset_index_t object_index, real_t x, real_t y)
{
    #ifdef LOG_INSTANCE_ACTIVITY
    std::cout << "creating instance " << id << std::endl;
    #endif

    assert(!instance_valid(id));
    assert(!instance_pending_deletion(id));

    Instance* i =  new Instance();
    i->m_data.m_id = id;

    VALGRIND_CHECK_INITIALIZED(object_index);
    i->m_data.m_object_index = object_index;

    AssetObject* object = static_cast<AssetObject*>(m_assets.get_asset(object_index));

    // initialize from object resource
    i->m_data.m_depth = object->m_init_depth;
    i->m_data.m_visible = object->m_init_visible;
    i->m_data.m_persistent = object->m_init_persistent;
    i->m_data.m_solid = object->m_init_solid;
    i->m_data.m_sprite_index = object->m_init_sprite_index;
    i->m_data.m_mask_index = object->m_init_mask_index;
    i->m_data.m_position = { x, y };
    i->m_data.m_position_prev = { x, y };
    i->m_data.m_position_start = { x, y };

    // add to data structures
    m_instances[id] = i;
    m_valid[id] = true;
    m_active[id] = true;

    // use this member to cache lookups for 'invalid' given the instance pointer.
    i->m_data.m_frame_owner = this;
    i->m_data.m_frame_active = true;

    add_collision(i);

    // append
    m_resource_sorted_instances.push_back(i);
    asset_index_t object_list_index = object_index;
    while (object_list_index != k_no_asset)
    {
        get_object_instances(object_list_index).push_back(i);
        object_list_index = get_object_parent_index(object_list_index);
    }

    // add in depth order
    i->m_data.m_frame_depth_previous = i->m_data.m_depth;
    auto iter = m_depth_sorted_instances.begin();
    for (; iter != m_depth_sorted_instances.end(); iter++)
    {
        if ((*iter)->m_data.m_frame_depth_previous > i->m_data.m_frame_depth_previous)
        {
            break;
        }
    }
    m_depth_sorted_instances.insert(iter, i);
    return i;
}

void Frame::reset_hard()
{
    m_ds_list.clear();
    m_assets.clear();
    m_bytecode.clear();
    m_reflection = nullptr;
    m_collision.clear();

    // todo: determine ownership of m_display pointer.
    if (m_display) delete m_display;
    m_display = nullptr;
}

Instance* Frame::create_instance(asset_index_t object_index, real_t x, real_t y)
{
    return create_instance_as(m_config.m_next_instance_id++, object_index, x, y);
}

void Frame::change_instance(direct_instance_id_t id, asset_index_t object_index)
{
    assert(instance_valid(id));
    assert(m_assets.get_asset<AssetObject*>(object_index));

    Instance* instance = get_instance(id);

    // remove from object list
    asset_index_t remove_list_index = instance->m_data.m_object_index;
    while (remove_list_index != k_no_asset)
    {
        auto& vec = get_object_instances(remove_list_index);
        vec.erase(std::remove(vec.begin(), vec.end(), instance), vec.end());
        remove_list_index = get_object_parent_index(remove_list_index);
    }

    // change object_index
    instance->m_data.m_object_index = object_index;

    // add to (new) object list
    asset_index_t add_list_index = instance->m_data.m_object_index;
    while (add_list_index != k_no_asset)
    {
        get_object_instances(add_list_index).push_back(instance);
        add_list_index = get_object_parent_index(add_list_index);
    }
}

void Frame::invalidate_instance(direct_instance_id_t id)
{
    assert(instance_valid(id));
    Instance* instance = m_instances[id];

    // remove collision
    remove_collision(m_instances[id]);

    // mark as invalid
    instance->m_data.m_frame_active = false;
    #ifndef NDEBUG
    instance->m_data.m_frame_valid = false;
    #endif
    m_valid[id] = false;
    m_active[id] = false;

    // memory-delete later.
    m_instances_delete.push_back(id);

    #ifdef LOG_INSTANCE_ACTIVITY
    std::cout << "slating for deletion instance " << id << std::endl;
    #endif
}

void Frame::process_instance_deletion()
{
    for (direct_instance_id_t id : m_instances_delete)
    {
        #ifdef LOG_INSTANCE_ACTIVITY
        std::cout << "deleting instance " << id << std::endl;
        #endif

        #ifndef NDEBUG
        assert(!m_instances[id]->m_data.m_frame_valid);
        #endif
        assert(!instance_valid(id));
        assert(!m_instances[id]->m_data.m_frame_active);
        assert(!instance_active(id));

        // delete pointer
        delete m_instances[id];

        // remove from instance map
        m_instances.erase(m_instances.find(id));
    }
    m_instances_delete.clear();
}

void Frame::remove_inactive_instances_from_vector(std::vector<Instance*>& vec)
{
    auto new_end = std::remove_if(
        vec.begin(),
        vec.end(),
        [](Instance* instance) -> bool
        {
            bool active = instance->m_data.m_frame_active;
            return !active;
        }
    );
    vec.erase(new_end, vec.end());
}

void Frame::remove_inactive_instances()
{
    // resource-sorted instances
    remove_inactive_instances_from_vector(m_resource_sorted_instances);

    // depth-sorted instances
    remove_inactive_instances_from_vector(m_depth_sorted_instances);

    // object instance lists
    for (auto& pair : m_object_instances)
    {
        auto& object_instances = *std::get<1>(pair);
        remove_inactive_instances_from_vector(object_instances);
    }
}

void Frame::sort_instances()
{
    // clear inactive instances from these vectors.
    remove_inactive_instances();

    // garbage-collect deleted instances.
    process_instance_deletion();

    // sort resource-sorted-instances lexicographically by object index then id.
    std::sort(m_resource_sorted_instances.begin(), m_resource_sorted_instances.end(),
        [](const Instance* a, const Instance* b) -> bool
        {
            if (a->m_data.m_object_index == b->m_data.m_object_index)
            {
                return a->m_data.m_id < b->m_data.m_id;
            }
            return a->m_data.m_object_index < b->m_data.m_object_index;
        }
    );

    // sort depth-sorted-instances by depth decreasing.
    for (Instance* instance : m_depth_sorted_instances)
    {
        instance->m_data.m_frame_depth_previous = instance->m_data.m_depth;
    }
    std::stable_sort(m_depth_sorted_instances.begin(), m_depth_sorted_instances.end(),
        [](const Instance* a, const Instance* b) -> bool
        {
            return a->m_data.m_frame_depth_previous > b->m_data.m_frame_depth_previous;
        }
    );
}

void Frame::get_multi_instance_iterator(ex_instance_id_t id, WithIterator& outIterator)
{
    std::vector<Instance*>& vec = (id == k_all)
        ? m_resource_sorted_instances
        : get_object_instances(id);

    auto id_iter = vec.rbegin();
    auto id_end = vec.rend();

    // create the iterator's array.
    // (it becomes responsible for deleting it).
    outIterator.m_single = false;
    outIterator.m_at = 0;
    outIterator.m_count = vec.size();
    Instance** instances = new Instance*[outIterator.m_count];

    size_t invalid_encountered = 0;
    size_t i = 0;
    for (; id_iter != id_end; ++id_iter)
    {
        Instance* instance = *id_iter;
        // only add valid instances.
        if (instance->m_data.m_frame_active)
        {
            instances[i++] = instance;
        }
        else
        {
            outIterator.m_count--;
        }
    }
    outIterator.m_instance = instances;

    // for speed,
    // check if number of invalid is greater than some arbitrary threshold,
    // and if so, refresh so that the next with-iterator has an easier time.
    if (invalid_encountered >= 0x40
        && invalid_encountered > (outIterator.m_count >> 1))
    {
        remove_inactive_instances_from_vector(vec);
    }
}

void Frame::change_room(asset_index_t room_index)
{
    // destroy all non-persistent instances
    for (auto& pair : m_instances)
    {
        Instance* instance = std::get<1>(pair);
        assert(instance);
        if (!instance->m_data.m_persistent)
        {
            invalidate_instance(std::get<0>(pair));
        }
    }

    // destroy all tiles
    m_tiles.clear();

    // destroy all backgrounds
    m_background_layers.clear();

    // collect garbage
    remove_inactive_instances();
    process_instance_deletion();

    // enter new room
    m_data.m_room_index = room_index;

    AssetRoom* room = m_assets.get_asset<AssetRoom*>(room_index);
    if (!room)
    {
        throw MiscError("Invalid room id " + std::to_string(room_index));
    }

    // room properties
    m_data.m_show_background_colour = room->m_show_colour;
    m_data.m_background_colour = room->m_colour;
    m_data.m_room_dimension = room->m_dimensions;
    m_data.m_desired_fps = room->m_speed;

    // add backgrounds
    for (AssetRoom::BackgroundLayerDefinition& def : room->m_bg_layers)
    {
        m_background_layers.emplace_back();
        BackgroundLayer& layer = m_background_layers.back();
        layer.m_background_index = def.m_background_index;
        layer.m_position = def.m_position;
        layer.m_velocity = def.m_velocity;
        layer.m_tiled_x = def.m_tiled_x;
        layer.m_tiled_y = def.m_tiled_y;
        layer.m_visible = def.m_visible;
        layer.m_foreground = def.m_foreground;

        // TODO: bg speed, stretch.
    }

    // default backgrounds.
    while (m_background_layers.size() < 8)
    {
        m_background_layers.emplace_back();
    }

    // add tiles
    #ifndef NDEBUG
    real_t prev_depth = std::numeric_limits<real_t>::min();
    #endif
    for (AssetRoom::TileLayerDefinition& def : room->m_tile_layers)
    {
        #ifndef NDEBUG
        assert(prev_depth < def.m_depth);
        prev_depth = def.m_depth;
        #endif

        m_tiles.m_tile_layers.emplace_back();
        TileLayer& layer = m_tiles.m_tile_layers.back();
        layer.m_depth = def.m_depth;
        for (AssetRoom::TileDefinition& tdef : def.m_contents)
        {
            Tile& tile = m_tiles.m_tiles[tdef.m_id];
            tile.m_background_index = tdef.m_background_index;
            tile.m_position = tdef.m_position;
            tile.m_bg_position = tdef.m_bg_position;
            tile.m_dimension = tdef.m_dimension;
            tile.m_scale = tdef.m_scale;
            tile.m_blend = tdef.m_blend;
            tile.m_depth = def.m_depth;
            tile.m_visible = true;
            layer.m_contents.push_back(tdef.m_id);
            ogm::collision::Entity<coord_t, tile_id_t> entity{ ogm::collision::Shape::rectangle,
                tile.get_aabb(), tdef.m_id};
            tile.m_collision_id = layer.m_collision.emplace_entity(entity);
        }
    }


    // add instances
    std::vector<Instance*> instances;
    instances.reserve(room->m_instances.size());
    for (AssetRoom::InstanceDefinition& def : room->m_instances)
    {
        if (!instance_valid(def.m_id))
        {
            const AssetObject* object = m_assets.get_asset<AssetObject*>(def.m_object_index);
            assert(object);
            Instance* instance = create_instance_as(def.m_id, def.m_object_index, def.m_position.x, def.m_position.y);
            instances.push_back(instance);

            // set instance properties
            instance->m_data.m_scale = def.m_scale;

            // run create event

            bytecode_index_t index = get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
            staticExecutor.pushSelf(instance);
            EventContext e = m_data.m_event_context;
            m_data.m_event_context.m_event = DynamicEvent::CREATE;
            m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
            m_data.m_event_context.m_object = def.m_object_index;
            execute_bytecode_safe(index);
            m_data.m_event_context = e;
            staticExecutor.popSelf();
        }
        else
        {
            instances.push_back(nullptr);
        }
    }

    // set views up
    {
        m_data.m_views_enabled = room->m_enable_views;
        size_t i = 0;
        for (const AssetRoom::ViewDefinition& def : room->m_views)
        {
            m_data.m_view_visible[i] = def.m_visible;
            m_data.m_view_position[i] = def.m_position;
            m_data.m_view_dimension[i] = def.m_dimension;
            m_data.m_view_angle[i] = 0;

            ++i;
        }
    }

    // execute instance creation code
    size_t i = 0;
    for (AssetRoom::InstanceDefinition& def : room->m_instances)
    {
        // run creation code event
        Instance* instance = instances[i++];

        if (instance)
        {
            staticExecutor.pushSelf(instance);
            execute_bytecode_safe(def.m_cc);
            staticExecutor.popSelf();
        }
    }

    // execute room creation code
    // context is a dummy instance.
    Instance* instance = new Instance();
    staticExecutor.pushSelf(instance);
    execute_bytecode_safe(room->m_cc);
    staticExecutor.popSelf();
    delete instance;
}
}
