#include "ogm/interpreter/Frame.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <algorithm>
#include <unordered_set>

namespace ogm::interpreter
{
    
// declared in Garbage.hpp
#ifdef OGM_GARBAGE_COLLECTOR
GarbageCollector g_gc{};
#endif

using namespace ogm;
Instance* Frame::create_instance_as(instance_id_t id, const InstanceCreateArgs& args)
{
    #ifdef LOG_INSTANCE_ACTIVITY
    std::cout << "creating instance " << id << std::endl;
    #endif

    ogm_assert(!instance_valid(id));
    ogm_assert(!instance_pending_deletion(id));

    Instance* i =  new Instance();
    i->m_data.m_id = id;

    VALGRIND_CHECK_INITIALIZED(args.m_object_index);
    i->m_data.m_object_index = args.m_object_index;

    AssetObject* object = static_cast<AssetObject*>(m_assets.get_asset(args.m_object_index));

    // initialize from object resource
    i->m_data.m_visible = object->m_init_visible;
    i->m_data.m_persistent = object->m_init_persistent;
    i->m_data.m_solid = object->m_init_solid;
    i->m_data.m_sprite_index = object->m_init_sprite_index;
    i->m_data.m_mask_index = object->m_init_mask_index;
    i->m_data.m_position = args.m_position;
    i->m_data.m_position_prev = args.m_position;
    i->m_data.m_position_start = args.m_position;
    i->m_data.m_input_listener = object->m_input_listener;
    i->m_data.m_async_listener = object->m_async_listener;
    
    // set depth or layer
    switch(args.m_type)
    {
    case InstanceCreateArgs::use_object_depth:
        i->m_data.m_depth = object->m_init_depth;
        #ifdef OGM_LAYERS
        if (m_data.m_layers_enabled)
        {
            m_layers.managed_add(object->m_init_depth, id);
        }
        #endif
        break;
    case InstanceCreateArgs::use_provided_depth:
        i->m_data.m_depth = args.m_depth;
        #ifdef OGM_LAYERS
        if (m_data.m_layers_enabled)
        {
            m_layers.managed_add(object->m_init_depth, id);
        }
        #endif
        break;
    #ifdef OGM_LAYERS
    case InstanceCreateArgs::use_provided_layer:
        ogm_assert(m_data.m_layers_enabled);
        ogm_assert(m_layers.layer_exists(args.m_layer));
        m_layers.add_element(
            LayerElement::et_instance,
            args.m_layer,
            i->m_data.m_layer_elt
        ).instance.m_id = id;
        break;
    case InstanceCreateArgs::use_provided_layer_and_elt:
        ogm_assert(m_data.m_layers_enabled);
        ogm_assert(m_layers.layer_exists(args.m_layer));
        ogm_assert(!m_layers.element_exists(args.m_layer_elt));
        m_layers.add_element_at(
            args.m_layer_elt,
            LayerElement::et_instance,
            args.m_layer
        ).instance.m_id = id;
        i->m_data.m_layer_elt = args.m_layer_elt;
        break;
    #endif
    }

    // add to data structures
    m_instances[id] = i;
    m_valid[id] = true;
    m_active[id] = true;

    // use this member to cache lookups for 'invalid' given the instance pointer.
    i->m_data.m_frame_owner = this;
    i->m_data.m_frame_active = true;
    i->m_data.m_frame_in_instance_vectors = false;

    add_collision(i);

    add_to_instance_vectors(i);
    
    if (args.m_run_create_event)
    {
        // run create event
        Frame::EventContext e = m_data.m_event_context;
        m_data.m_event_context.m_event = DynamicEvent::CREATE;
        m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
        m_data.m_event_context.m_object = i->m_data.m_object_index;
        bytecode_index_t index = get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
        staticExecutor.pushSelfDouble(i);
        execute_bytecode_safe(index);
        staticExecutor.popSelfDouble();
        m_data.m_event_context = e;
    }

    return i;
}

#ifdef DEBUG_INSTANCE_VECTORS
void Frame::assert_instance_vector_does_not_contain(Instance* i)
{
    ogm_assert(std::find(m_resource_sorted_instances.begin(), m_resource_sorted_instances.end(), i) == m_resource_sorted_instances.end());
    ogm_assert(std::find(m_depth_sorted_instances.begin(), m_depth_sorted_instances.end(), i) == m_depth_sorted_instances.end());
    ogm_assert(std::find(m_input_listener_instances.begin(), m_input_listener_instances.end(), i) == m_input_listener_instances.end());
    ogm_assert(std::find(m_async_listener_instances.begin(), m_async_listener_instances.end(), i) == m_async_listener_instances.end());
    ogm_assert(!i->m_data.m_frame_in_instance_vectors);
    // TODO: search object lists too
}
#endif

void Frame::add_to_instance_vectors(Instance* i)
{
    if (i->m_data.m_frame_in_instance_vectors) return;

    #ifdef DEBUG_INSTANCE_VECTORS
    assert_instance_vector_does_not_contain(i);
    #endif

    // append
    m_resource_sorted_instances.push_back(i);
    asset_index_t object_list_index = i->m_data.m_object_index;
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
    i->m_data.m_frame_in_instance_vectors = true;

    // input listener
    if (i->m_data.m_input_listener)
    {
        m_input_listener_instances.push_back(i);
    }

    // async listener
    if (i->m_data.m_input_listener)
    {
        m_async_listener_instances.push_back(i);
    }
}

void Frame::change_instance(direct_instance_id_t id, asset_index_t object_index)
{
    ogm_assert(instance_valid(id));
    AssetObject* obj = m_assets.get_asset<AssetObject*>(object_index);

    Instance* instance = get_instance(id);

    if (instance->m_data.m_frame_in_instance_vectors)
    {
        // remove from object list
        asset_index_t remove_list_index = instance->m_data.m_object_index;
        while (remove_list_index != k_no_asset)
        {
            auto& vec = get_object_instances(remove_list_index);
            vec.erase(std::remove(vec.begin(), vec.end(), instance), vec.end());
            remove_list_index = get_object_parent_index(remove_list_index);
        }

        if (instance->m_data.m_input_listener)
        {
            auto& vec = m_input_listener_instances;

            // input listener
            vec.erase(std::remove(vec.begin(), vec.end(), instance), vec.end());
        }

        if (instance->m_data.m_async_listener)
        {
            auto& vec = m_async_listener_instances;

            // input listener
            vec.erase(std::remove(vec.begin(), vec.end(), instance), vec.end());
        }
    }

    // change object_index
    instance->m_data.m_object_index = object_index;
    instance->m_data.m_input_listener = obj->m_input_listener;
    instance->m_data.m_async_listener = obj->m_async_listener;

    // add to (new) object list
    if (instance->m_data.m_frame_in_instance_vectors)
    {
        asset_index_t add_list_index = instance->m_data.m_object_index;
        while (add_list_index != k_no_asset)
        {
            get_object_instances(add_list_index).push_back(instance);
            add_list_index = get_object_parent_index(add_list_index);
        }

        // input listener
        if (instance->m_data.m_input_listener)
        {
            auto& vec = m_input_listener_instances;
            vec.push_back(instance);
        }

        // input listener
        if (instance->m_data.m_async_listener)
        {
            auto& vec = m_async_listener_instances;
            vec.push_back(instance);
        }
    }
}

void Frame::invalidate_instance(direct_instance_id_t id)
{
    ogm_assert(instance_valid(id));
    Instance* instance = m_instances[id];

    // remove inactive collision if needed
    if (!instance_active(id))
    {
        remove_inactive_collision(instance);
    }

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
        ogm_assert(!m_instances[id]->m_data.m_frame_valid);
        #endif
        ogm_assert(!instance_valid(id));
        ogm_assert(!m_instances[id]->m_data.m_frame_active);
        ogm_assert(!instance_active(id));

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
            if (!active)
            {
                instance->m_data.m_frame_in_instance_vectors = false;
            }
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

    remove_inactive_instances_from_vector(m_input_listener_instances);

    remove_inactive_instances_from_vector(m_async_listener_instances);

    // object instance lists
    for (auto& pair : m_object_instances)
    {
        auto& object_instances = *std::get<1>(pair);
        remove_inactive_instances_from_vector(object_instances);
    }
}

void Frame::sort_instances()
{
    // need to clear the collision update queue because it has instance pointers.
    process_collision_updates();

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

    // TODO: how to sort listener instances?
}

void Frame::get_multi_instance_iterator(ex_instance_id_t id, WithIterator& outIterator)
{
    std::vector<Instance*>* ptr_vec;
    try
    {
        ptr_vec = (id == k_all)
            ? &m_resource_sorted_instances
            : &get_object_instances(id);
    }
    catch (std::exception& e)
    {
        // we need to do this to ensure outIterator is initialized.
        outIterator.m_single = false;
        outIterator.m_instance = nullptr;
        outIterator.m_count = 0;
        throw e;
    }

    std::vector<Instance*>& vec = *ptr_vec;
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
        // only add valid, active instances.
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
        && invalid_encountered > (outIterator.m_count / 2))
    {
        remove_inactive_instances_from_vector(vec);
    }
}

void Frame::change_room(asset_index_t room_index)
{
    AssetRoom* room = m_assets.get_asset<AssetRoom*>(room_index);
    if (!room)
    {
        throw MiscError("Invalid room id " + std::to_string(room_index));
    }
    
    #ifdef OGM_LAYERS
    // take note of preferred name and layer depth for all persistent instances.
    struct persistent_instance_layer_preference_t
    {
        Instance* instance;
        bool m_managed;
        std::string m_name; // name, if set, outprioritizes depth
        real_t m_depth;
    };
    std::vector<persistent_instance_layer_preference_t> persistent_instance_layer_prefs;
    #endif
    
    // destroy all non-persistent instances,
    // and, if layers are enabled, record the layer preferences
    // of persistent instances.
    for (auto& pair : m_instances)
    {
        Instance* instance = std::get<1>(pair);
        ogm_assert(instance);
        if (!instance_valid(instance)) continue;
        
        if (!instance->m_data.m_persistent)
        {
            invalidate_instance(std::get<0>(pair));
        }
        #ifdef OGM_LAYERS
        else if (m_data.m_layers_enabled || room->m_layers_enabled)
        {
            // record layer preferences for persistent instance
            // in order to find a matching layer in the next room.
            if (!m_data.m_layers_enabled || instance->layer_is_managed())
            {
                persistent_instance_layer_prefs.push_back(
                    persistent_instance_layer_preference_t{
                        instance,
                        true,
                        "",
                        instance->m_data.m_depth
                    }
                );
            }
            else
            {
                Layer& layer = m_layers.m_layers.at(instance->get_layer());
                
                persistent_instance_layer_prefs.push_back(
                    persistent_instance_layer_preference_t{
                        instance,
                        false,
                        layer.m_name,
                        layer.m_depth
                    }
                );
            }
        }
        #endif
    }
    
    #ifdef OGM_LAYERS    
    // remove all layers
    m_layers.clear();
    #endif

    // destroy all tiles
    m_tiles.clear();

    // destroy all backgrounds
    m_background_layers.clear();

    // collect garbage
    remove_inactive_instances();
    process_instance_deletion();

    // enter new room
    m_data.m_room_index = room_index;

    // room properties
    m_data.m_show_background_colour = room->m_show_colour;
    m_data.m_background_colour = room->m_colour;
    m_data.m_room_dimension = room->m_dimensions;
    m_data.m_desired_fps = room->m_speed;
    #ifdef OGM_LAYERS
    m_data.m_layers_enabled = room->m_layers_enabled;
    
    // add layers, but not their contents (yet).
    if (m_data.m_layers_enabled)
    {
        for (const auto& [layer_id, def] : room->m_layers)
        {
            Layer& layer = m_layers.layer_create_at(layer_id, def.m_depth, def.m_name.c_str());
            layer.m_position = def.m_position;
            layer.m_velocity = def.m_velocity;
            layer.m_visible = def.m_visible;
            layer.m_primary_element = def.m_primary_element;
        }
    }
    
    // add/merge all pre-existing persistent instances' layers.
    for (const persistent_instance_layer_preference_t& prefs : persistent_instance_layer_prefs)
    {
        assert(m_data.m_layers_enabled);
        
        Instance* instance = prefs.instance;
        if (!prefs.m_managed)
        {
            // assign this instance to a 'managed' layer.
            prefs.instance->m_data.m_layer_elt = -1;
            m_layers.managed_add(instance->m_data.m_depth, instance->m_data.m_id);
        }
        else
        {
            // create layer if no matching layer exists.
            layer_id_t layer_id;
            auto iter = m_layers.m_layer_ids_by_name.find(prefs.m_name);
            if (iter == m_layers.m_layer_ids_by_name.end())
            {
                // no layer with matching name; create new layer
                ogm_assert(prefs.m_name != ""); // layers cannot have empty names.
                m_layers.layer_create(prefs.m_depth, prefs.m_name.c_str(), layer_id);
            }
            else
            {
                // found matching layer
                layer_id = iter->second.front();
            }
            
            // create element for instance.
            m_layers.add_element(
                LayerElement::et_instance,
                layer_id,
                instance->m_data.m_layer_elt
            ).instance.m_id = instance->m_data.m_id;
        }
    }
    
    // add layer elements from room to layers
    for (const auto& [elt_id, elt] : room->m_layer_elements)
    {
        // skip adding element if it's an instance element; instances add their own layer elements.
        if (elt.m_type == elt.et_instance)
        {
            continue;
        }
        else
        {
            m_layers.add_element_at(elt_id, elt.m_type, elt.m_layer) = elt;
        }
    }
    #endif

    // add backgrounds (unless using gmv2 layer model)
    if (!m_data.m_layers_enabled)
    {
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
    }

    // add tiles (unless using gmv2 layer model)
    if (!m_data.m_layers_enabled)
    {
        #ifndef NDEBUG
        real_t prev_depth = std::numeric_limits<real_t>::lowest();
        #endif
        for (AssetRoom::TileLayerDefinition& def : room->m_tile_layers)
        {
            #ifndef NDEBUG
            ogm_assert(prev_depth < def.m_depth);
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
                tile.m_alpha = tdef.m_alpha;
                tile.m_depth = def.m_depth;
                tile.m_visible = true;
                layer.m_contents.push_back(tdef.m_id);
                ogm::collision::Entity<coord_t, tile_id_t> entity{ ogm::collision::ShapeType::rectangle,
                    tile.get_aabb(), tdef.m_id};
                tile.m_collision_id = layer.m_collision.emplace_entity(entity);
            }
        }
    }

    // add room's instances (except pre-existing persistent instances).
    std::vector<Instance*> instances;
    instances.reserve(room->m_instances.size());
    for (AssetRoom::InstanceDefinition& def : room->m_instances)
    {
        if (!instance_valid(def.m_id))
        {
            const AssetObject* object = m_assets.get_asset<AssetObject*>(def.m_object_index);
            ogm_assert(object);
            InstanceCreateArgs args{ def.m_object_index, {def.m_position.x, def.m_position.y}, false}; 
            #ifdef OGM_LAYERS
            if (m_data.m_layers_enabled)
            {
                // use layer and element provided by room
                args.m_type = InstanceCreateArgs::use_provided_layer_and_elt;
                args.m_layer_elt = def.m_layer_elt_id;
                args.m_layer = room->m_layer_elements.at(def.m_layer_elt_id).m_layer;
            }
            #endif
            Instance* instance = create_instance_as(def.m_id, args);
            ogm_assert(m_instances.find(instance->m_data.m_id) != m_instances.end());
            ogm_assert(get_ex_instance_from_ex_id(instance->m_data.m_id) == instance);
            instances.push_back(instance);

            // set instance properties
            instance->m_data.m_scale = def.m_scale;
            instance->m_data.m_angle = def.m_angle;
            instance->m_data.m_image_blend = def.m_blend;
            
            // queue collision update
            queue_update_collision(instance);
        }
        else
        {
            // instance already exists.
            instances.push_back(nullptr);
        }
    }
    
    // instances have been added, so we add them to the collision tracker.
    process_collision_updates();

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

    // execute create events for all room instances,
    // (unless they already exist due to being persistent)
    {
        size_t i = 0;
        for (AssetRoom::InstanceDefinition& def : room->m_instances)
        {
            Instance* instance = instances[i++];
            if (!instance) continue; // persistent instances aren't listed here, so we skip.
            
            ogm_assert(m_instances.find(instance->m_data.m_id) != m_instances.end());
            ogm_assert(get_ex_instance_from_ex_id(instance->m_data.m_id) == instance);

            const AssetObject* object = m_assets.get_asset<AssetObject*>(def.m_object_index);
            ogm_assert(object);

            // get create event
            bytecode_index_t index = get_static_event_bytecode<ogm::asset::StaticEvent::CREATE>(object);
            staticExecutor.pushSelfDouble(instance);
            EventContext e = m_data.m_event_context;
            m_data.m_event_context.m_event = DynamicEvent::CREATE;
            m_data.m_event_context.m_sub_event = DynamicSubEvent::NO_SUB;
            m_data.m_event_context.m_object = def.m_object_index;

            // execute create event
            execute_bytecode_safe(index);
            m_data.m_event_context = e;
            staticExecutor.popSelfDouble();
        }
    }

    // execute instance creation code
    {
        size_t i = 0;
        for (AssetRoom::InstanceDefinition& def : room->m_instances)
        {
            Instance* instance = instances[i++];

            if (instance)
            {
                staticExecutor.pushSelfDouble(instance);
                execute_bytecode_safe(def.m_cc);
                staticExecutor.popSelfDouble();
            }
        }
    }

    // execute room creation code
    // context is a dummy instance.
    Instance* instance = new Instance();
    staticExecutor.pushSelfDouble(instance);
    execute_bytecode_safe(room->m_cc);
    staticExecutor.popSelfDouble();
    delete instance;
}

template<bool write>
void Frame::serialize(typename state_stream<write>::state_stream_t& s)
{
    // so we don't have to serialize m_queued_collision_updates.
    if (write)
    {
        process_collision_updates();
    }
    else
    {
        #ifdef QUEUE_COLLISION_UPDATES
        m_queued_collision_updates.clear();
        #endif
    }

    // TODO: serialize data structures
    // TODO: serialize buffers
    // TODO: serialize network (maybe??)
    // TODO: filesystem (maybe??)
    // TODO: assets (maybe?)
    // TODO: bytecode (maybe?)
    // TODO: reflection (maybe???)
    // TODO: m_display
    // TODO: m_config

    _serialize_canary<write>(s);

    // m_tiles:
    _serialize<write>(s, m_tiles);

    // m_data:
    _serialize<write>(s, m_data.m_prg_end);
    _serialize<write>(s, m_data.m_prg_reset);
    _serialize<write>(s, m_data.m_views_enabled);
    for (size_t i = 0; i < k_view_count; ++i)
    {
        _serialize<write>(s, m_data.m_view_visible[i]);
        _serialize<write>(s, m_data.m_view_position[i]);
        _serialize<write>(s, m_data.m_view_dimension[i]);
        _serialize<write>(s, m_data.m_view_angle[i]);
    }
    _serialize<write>(s, m_data.m_desired_fps);
    _serialize<write>(s, m_data.m_view_current);
    _serialize<write>(s, m_data.m_room_index);
    _serialize<write>(s, m_data.m_show_background_colour);
    _serialize<write>(s, m_data.m_background_colour);
    _serialize<write>(s, m_data.m_room_dimension);
    _serialize_vector_replace<write>(s, m_data.m_clargs);
    _serialize<write>(s, m_data.m_room_goto_queued);
    _serialize<write>(s, m_data.m_event_context);

    // global variables
    m_globals.serialize<write>(s);

    // instance data -- replace old instances with new.
    #ifdef QUEUE_COLLISION_UPDATES
    ogm_assert(m_queued_collision_updates.empty());
    #endif

    auto instance_ptr_to_id = [](Instance* instance, ex_instance_id_t& out)
    {
        if (instance && instance->m_data.m_serializable)
        {
            out = instance->m_data.m_id;
        }
        else
        {
            out = k_noone;
        }
    };

    auto instance_id_to_ptr = [this](ex_instance_id_t id, Instance*& out)
    {
        if (id == k_noone)
        {
            out = nullptr;
            return;
        }
        out = this->m_instances.at(id);
    };

    auto is_nullptr = [](void* o) -> bool {return !o;};

    std::vector<Instance*> unserializable_instances;

    size_t instance_count;
    _serialize_canary<write>(s);
    if (write)
    {
        instance_count = m_instances.size();
        _serialize<write>(s, instance_count);
        for (auto& [ id, instance ] : m_instances)
        {
            direct_instance_id_t _id = id;

            // skip non-serializable instances
            if (!instance->m_data.m_serializable)
            {
                _id = k_noone;
                unserializable_instances.push_back(instance);
            }

            // write id.
            _serialize<write>(s, _id);

            if (instance->m_data.m_serializable)
            {
                instance->template serialize<write>(s);
            }
        }
    }
    else
    {
        _serialize<write>(s, instance_count);

        // delete & erase previous serializable instances
        for (auto it = m_instances.begin(); it != m_instances.end();)
        {
            auto& [id, instance] = *it;

            // TODO: reuse serializable instances? no need to delete.
            if (instance->m_data.m_serializable)
            {
                delete instance;
                m_valid.erase(id);
                m_active.erase(id);
                it = m_instances.erase(it);
            }
            else
            {
                unserializable_instances.push_back(instance);
                ++it;
            }
        }

        for (size_t i = 0; i < instance_count; ++i)
        {
            instance_id_t id;
            _serialize<write>(s, id);
            if (id != k_noone)
            {
                Instance* instance = new Instance();
                instance->m_data.m_frame_owner = this;
                instance->template serialize<write>(s);
                m_instances[id] = instance;
                m_active[id] = instance->m_data.m_frame_active;
                m_valid[id] = instance->m_data.m_frame_valid;
            }
        }
    }
    _serialize_canary<write>(s);

    // FIXME: if an unserializable instance is in here, preserve it / don't include it.
    _serialize_vector_replace<write>(s, m_instances_delete);

    // instance lists
    _serialize_vector_map<write, instance_id_t>(s, m_depth_sorted_instances, instance_ptr_to_id, instance_id_to_ptr);
    _serialize_vector_map<write, instance_id_t>(s, m_resource_sorted_instances, instance_ptr_to_id, instance_id_to_ptr);
    _serialize_vector_map<write, instance_id_t>(s, m_input_listener_instances, instance_ptr_to_id, instance_id_to_ptr);
    _serialize_vector_map<write, instance_id_t>(s, m_async_listener_instances, instance_ptr_to_id, instance_id_to_ptr);

    if (!write)
    // clear nullptrs from these lists.
    {
        // TODO: do we really need two passes when nullptr is so rare?
        erase_if(m_depth_sorted_instances, is_nullptr);
        erase_if(m_resource_sorted_instances, is_nullptr);
        erase_if(m_input_listener_instances, is_nullptr);
        erase_if(m_async_listener_instances, is_nullptr);
    }

    _serialize_canary<write>(s);

    // object instances list
    {
        size_t len;
        if (write) len = m_object_instances.size();
        _serialize<write>(s, len);
        if (!write)
        {
            // clear previous vector
            for (auto& [object_index, vec] : m_object_instances)
            {
                if (vec)
                {
                    delete vec;
                }
            }
            m_object_instances.clear();

            // read in new vectors.
            for (size_t i = 0; i < len; ++i)
            {
                asset_index_t index;
                _serialize<write>(s, index);
                auto* vec = new std::vector<Instance*>;
                _serialize_vector_map<write, instance_id_t>(s, *vec, instance_ptr_to_id, instance_id_to_ptr);

                // OPTIMIZE: do we really need two passes, when unserializable instances are so rare?
                // remove nullptrs
                erase_if(*vec, is_nullptr);
                m_object_instances[index] = vec;
            }
        }
        else
        {
            for (auto& [index, vec] : m_object_instances)
            {
                auto _index = index;
                _serialize<write>(s, _index);
                _serialize_vector_map<write, instance_id_t>(
                    s, *vec,
                    instance_ptr_to_id,
                    instance_id_to_ptr
                );
            }
        }
    }

    if (!write)
    {
        // add unserializable_instances back in.
        for (Instance* instance : unserializable_instances)
        {
            instance->m_data.m_frame_in_instance_vectors = false;
            add_to_instance_vectors(instance);
        }
    }

    ogm_assert(s.good());
    _serialize_canary<write>(s);

    // serialize display
    {
        bool has_display = !!m_display;
        bool prev_has_display = has_display;
        _serialize<write>(s, has_display);
        if (!write)
        {
            if (prev_has_display && ! has_display)
            {
                delete m_display;
                m_display = nullptr;
            }
            else if (has_display && ! prev_has_display)
            {
                // don't know how to construct a new display without dimensions listed...
                ogm_assert(false);
            }
        }
        if (has_display)
        {
            // serialize display
            m_display->serialize<write>(s);
        }
    }

    ogm_assert(s.good());
    _serialize_canary<write>(s);

    if (!write)
    {
        recalculate_collision();
    }
}

void Frame::recalculate_collision()
{
    // this is a precondition
    #ifdef QUEUE_COLLISION_UPDATES
    ogm_assert(m_queued_collision_updates.empty());
    #endif

    // clear all collision maps
    m_collision.clear();
    m_inactive_collision.clear();

    for (auto& [id, instance] : m_instances)
    {
        // OPTIMIZE: checking validity may not be necessary
        if (!instance_valid(id)) continue;

        // queue instance for collision update.
        // replace existing collision data.
        instance->m_data.m_frame_collision_id = -1;
        instance->m_data.m_frame_inactive_collision_id = -1;
        #ifdef QUEUE_COLLISION_UPDATES
        ogm_assert(!instance->m_data.m_collision_queued);
        #endif
        if (instance_active(instance))
        {
            queue_update_collision(instance);
        }
        else
        {
            add_inactive_collision(instance);
        }
    }
}

// explicit template instantiation
template
void Frame::serialize<false>(typename state_stream<false>::state_stream_t& s);

template
void Frame::serialize<true>(typename state_stream<true>::state_stream_t& s);

void Frame::reset_hard()
{
    // datastructures
    m_ds_list.clear();
    m_ds_map.clear();
    m_ds_grid.clear();
    m_ds_stack.clear();
    m_ds_queue.clear();
    m_ds_priority.clear();

    // tables
    m_assets.clear();
    m_bytecode.clear();
    m_reflection = nullptr;

    // data
    m_tiles.clear();
    m_instances.clear();
    m_collision.clear();
    m_buffers.clear();
    m_background_layers.clear();

    // TODO:
    // m_network.clear();
    // m_config.clear()
    //
    //
    //

    m_data = Data{};

    // todo: determine ownership of m_display pointer.
    if (m_display) delete m_display;
    m_display = nullptr;

    // variables
    for (auto& [id, variable] : m_globals)
    {
        variable.make_not_root();
    }
    m_globals.clear();

    // instances
    m_valid.clear();
    m_active.clear();
    for (auto pair : m_instances)
    {
        delete pair.second;
    }
    m_instances.clear();
    m_instances_delete.clear();
    m_depth_sorted_instances.clear();
    m_resource_sorted_instances.clear();
    m_input_listener_instances.clear();
    m_async_listener_instances.clear();
    for (auto pair : m_object_instances)
    {
        delete pair.second;
    }

    m_object_instances.clear();

    #ifdef QUEUE_COLLISION_UPDATES
    m_queued_collision_updates.clear();
    #endif
}

void Frame::gc_integrity_check() const
{
    // iterate through all globals and all instances
    for (auto& [id, v] : m_globals)
    {
        v.gc_integrity_check();
    }
    
    for (auto& [id, instance] : m_instances)
    {
        if (instance)
        {
            instance->gc_integrity_check();
        }
    }
}

}
