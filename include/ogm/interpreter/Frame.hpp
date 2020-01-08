#pragma once

#include "TileLayer.hpp"
#include "BackgroundLayer.hpp"
#include "WithIterator.hpp"
#include "Filesystem.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/collision/collision.hpp"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "ds/DataStructureManager.hpp"
#include "ds/List.hpp"
#include "ds/Map.hpp"
#include "ds/Grid.hpp"
#include "ds/Stack.hpp"
#include "ds/Queue.hpp"
#include "ds/PriorityQueue.hpp"
#include "Buffers.hpp"
#include "Network.hpp"
#include "ogm/common/serialize.hpp"
#include "Garbage.hpp"

#include <map>

// Stores and manages the program's state, but doesn't interpret bytecode commands (that's Executor.hpp / execute.cpp)
namespace ogm { namespace interpreter
{
    using namespace ogm::bytecode;
    using namespace ogm::asset;

    // an Expanded Instance pointer can be either an instance or a special pointer value.
    typedef Instance ExInstance;

    // special ID values
    constexpr ex_instance_id_t k_self  = -1;
    constexpr ex_instance_id_t k_other = -2;
    constexpr ex_instance_id_t k_all   = -3;
    constexpr ex_instance_id_t k_noone = -4;

    // only used for comparing pointer values
    constexpr uintptr_t k_uint_self   = 1;
    constexpr uintptr_t k_uint_other  = 2;
    constexpr uintptr_t k_uint_multi  = 3;
    static ExInstance* const k_inst_self  = reinterpret_cast<ExInstance*>(k_uint_self);
    static ExInstance* const k_inst_other = reinterpret_cast<ExInstance*>(k_uint_other);
    static ExInstance* const k_inst_multi = reinterpret_cast<ExInstance*>(k_uint_multi);

    class Display;

    class Frame
    {
    public:
        // completely resets the frame as though it were constructed anew.
        void reset_hard();

        typedef collision::Entity<coord_t, direct_instance_id_t> CollisionEntity;

        // adds a new instance and sets its initial values.
        Instance* create_instance(asset_index_t object_index, real_t x=0, real_t y=0);
        Instance* create_instance_as(instance_id_t id, asset_index_t object_index, real_t x=0, real_t y=0);

        // changes instance to a different object
        void change_instance(direct_instance_id_t id, asset_index_t object_index);

        // will delete instance next time sort_instances() is run.
        void invalidate_instance(direct_instance_id_t id);

        inline Instance* get_instance(direct_instance_id_t id)
        {
            return m_instances.at(id);
        }

        inline const Instance* get_instance(direct_instance_id_t id) const
        {
            ogm_assert(m_instances.find(id) != m_instances.end());
            return m_instances.at(id);
        }

        // returns true if the given instance id exists and is valid.
        inline bool instance_valid(direct_instance_id_t id) const
        {
            if (m_instances.find(id) != m_instances.end())
            {
                if (m_valid.at(id))
                {
                    return true;
                }
            }
            return false;
        }

        inline bool instance_pending_deletion(direct_instance_id_t id) const
        {
            bool exists_in_memory = m_instances.find(id) != m_instances.end();
            ogm_assert(!exists_in_memory || std::find(m_instances_delete.begin(), m_instances_delete.end(), id) != m_instances_delete.end());
            return exists_in_memory && !instance_valid(id);
        }

        // returns true if the given instance exists (but may not be active).
        inline bool instance_valid(const Instance* i) const
        {
            if (m_valid.at(i->m_data.m_id))
            {
                ogm_assert(i->m_data.m_frame_valid);
                return true;
            }
            else
            {
                ogm_assert(!m_valid.at(i->m_data.m_id));
                return false;
            }
        }

        // returns true if the given instance id exists (but may not be active).
        // sets outInstance if the instance was found.
        inline bool instance_valid(direct_instance_id_t id, Instance*& outInstance)
        {
            decltype(m_instances)::iterator find_result = m_instances.find(id);
            if (find_result != m_instances.end())
            {
                if (m_valid.at(id))
                {
                    outInstance = (*find_result).second;
                    return true;
                }
            }
            return false;
        }

        inline bool instance_valid(direct_instance_id_t id, const Instance*& outInstance) const
        {
            decltype(m_instances)::const_iterator find_result = m_instances.find(id);
            if (find_result != m_instances.end())
            {
                if (m_valid.at(id))
                {
                    outInstance = (*find_result).second;
                    return true;
                }
            }
            return false;
        }

        // returns true if the given instance id exists and is valid.
        inline bool instance_active(direct_instance_id_t id) const
        {
            if (m_instances.find(id) != m_instances.end())
            {
                if (m_active.at(id))
                {
                    return true;
                }
            }
            return false;
        }

        // returns true if the given instance exists and is active.
        inline bool instance_active(const Instance* i) const
        {
            if (i->m_data.m_frame_active)
            {
                ogm_assert(m_active.at(i->m_data.m_id));
                ogm_assert(m_valid.at(i->m_data.m_id));
                ogm_assert(i->m_data.m_frame_valid);
                return true;
            }
            else
            {
                ogm_assert(!m_active.at(i->m_data.m_id));
                return false;
            }
        }

        // returns true if the given instance id exists and is valid.
        // sets outInstance if the instance was found.
        inline bool instance_active(direct_instance_id_t id, Instance*& outInstance)
        {
            decltype(m_instances)::iterator find_result = m_instances.find(id);
            if (find_result != m_instances.end())
            {
                if (m_active.at(id))
                {
                    outInstance = (*find_result).second;
                    return true;
                }
            }
            return false;
        }

        inline bool instance_active(direct_instance_id_t id, const Instance*& outInstance) const
        {
            decltype(m_instances)::const_iterator find_result = m_instances.find(id);
            if (find_result != m_instances.end())
            {
                if (m_active.at(id))
                {
                    outInstance = (*find_result).second;
                    return true;
                }
            }
            return false;
        }

        // Returns a pointer to the given instance, or a special value such as k_inst_self
        ExInstance* get_ex_instance_from_ex_id(ex_instance_id_t id)
        {
            ExInstance* instance;
            if (instance_valid(id, instance))
            {
                return instance;
            }

            switch(id)
            {
                case k_self:
                    return k_inst_self;
                case k_other:
                    return k_inst_other;
                case k_all:
                    return k_inst_multi;
                case k_noone:
                    return nullptr;
            }

            // check object resource list.
            if (m_assets.get_asset<AssetObject*>(id))
            {
                // it would be nice if we could return this value somehow, even though
                // it is not a pointer to an ExInstance* per se.
                return k_inst_multi;
            }

            return nullptr;
        }

        // check if the given instance matches the given ex_instance (e.g. my_instance could match object `objSolid`, and certainly matches `all`)
        inline bool instance_matches_ex_id(const Instance* instance, ex_instance_id_t id, const Instance* self, const Instance* other) const
        {
            ogm_assert(instance_valid(instance->m_data.m_id));
            if (instance->m_data.m_id == id)
            {
                return true;
            }
            if (instance->m_data.m_object_index == id)
            {
                return true;
            }

            switch(id)
            {
                case k_self:
                    return instance == self;
                case k_other:
                    return instance == other;
                case k_all:
                    return true;
                case k_noone:
                    return false;
                default:
                    break;
            }

            // check ancestry
            asset_index_t object_id = instance->m_data.m_object_index;
            while (object_id != k_no_asset)
            {
                if (object_id == id)
                {
                    return true;
                }
                object_id = get_object_parent_index(object_id);
            }

            return false;
        }

        inline bool instance_matches_ex_id(direct_instance_id_t instance_id, ex_instance_id_t id, const Instance* self, const Instance* other) const
        {
            const Instance* instance;
            if (instance_valid(instance_id, instance))
            {
                return instance_matches_ex_id(instance, id, self, other);
            }
            return false;
        }

        inline bool has_global_variable(variable_id_t id)
        {
            return m_globals.contains(id);
        }

        inline void store_global_variable(variable_id_t id, Variable&& v)
        {
            #ifdef OGM_GARBAGE_COLLECTOR
            // this will be decremented when cleanup'd.
            v.make_root();
            #endif
            
            m_globals[id] = std::move(v);
        }

        // retrieves variable, adding it if necessary.
        inline Variable& get_global_variable(variable_id_t id)
        {
            return m_globals[id];
        }

        // retrieves variable, throwing ItemNotFoundException on failure.
        inline const Variable& find_global_variable(variable_id_t id) const
        {
            return m_globals.at(id);
        }

        void sort_instances();

        // initializes a WithIterator to the given ex instance id.
        // resulting instance is not guaranteed to be active or valid.
        void get_instance_iterator(ex_instance_id_t id, WithIterator& outIterator, Instance* self = nullptr, Instance* other = nullptr)
        {
            ExInstance* instance = get_ex_instance_from_ex_id(id);

            switch (reinterpret_cast<uintptr_t>(instance))
            {
                case k_uint_self:
                    outIterator.m_instance = self;
                    outIterator.m_single = true;
                    break;
                case k_uint_other:
                    outIterator.m_instance = other;
                    outIterator.m_single = true;
                    break;
                case 0:
                    // empty iterator.
                    outIterator.m_instance = nullptr;
                    outIterator.m_single = true;
                    break;
                case k_uint_multi:
                    get_multi_instance_iterator(id, outIterator);
                    break;
                default:
                    // single instance
                    outIterator.m_instance = instance;
                    outIterator.m_single = true;
                    break;
            }
        }

        // id must either be an object index or the special value k_all.
        // guarantee: the resulting iterator only returns instances which were
        // valid and active at the time of the call to get_multi_instance_iterator.
        void get_multi_instance_iterator(ex_instance_id_t id, WithIterator& outIterator);

        // this count may not take into account recently-removed instances.
        inline size_t get_instance_count() const
        {
            // return m_instance_
            return m_resource_sorted_instances.size();
        }

        // this count may not take into account recently-removed instances.
        inline size_t get_object_instance_count(ex_instance_id_t id)
        {
            return get_object_instances(id).size();
        }

        // returns nth instance in list of valid instances
        // not guaranteed to be a valid instance -- check instance_valid().
        inline Instance* get_instance_nth(size_t n)
        {
            ogm_assert(n < m_resource_sorted_instances.size());
            return m_resource_sorted_instances[n];
        }

        // returns id of nth instance in list of valid instances
        // not guaranteed to be a valid instance -- check instance_valid().
        inline instance_id_t get_instance_id_nth(size_t n) const
        {
            ogm_assert(n < m_resource_sorted_instances.size());
            return m_resource_sorted_instances[n]->m_data.m_id;
        }

        // returns a single instance matching the ex_id, or nullptr.
        inline Instance* get_instance_single(ex_instance_id_t id, Instance* self, Instance* other)
        {
            ExInstance* ex = get_ex_instance_from_ex_id(id);
            switch (reinterpret_cast<uintptr_t>(ex))
            {
                case k_uint_self:
                    return self;
                case k_uint_other:
                    return other;
                case 0:
                    return nullptr;
                case k_uint_multi:
                    {
                        WithIterator iter;
                        get_multi_instance_iterator(id, iter);
                        if (iter.complete())
                        {
                            return nullptr;
                        }

                        return *iter;
                    }
                    break;
                default:
                    return ex;
            }
        }

        // activates the given instance
        // cannot activate instances scheduled for deletion.
        inline void activate_instance(Instance* instance)
        {
            ogm_assert(instance_valid(instance->m_data.m_id));
            direct_instance_id_t id = instance->m_data.m_id;
            ogm_assert(instance_valid(id));
            // TODO: should we simply ogm_assert !active?
            if (!instance->m_data.m_frame_active)
            {
                instance->m_data.m_frame_active = true;
                m_active[id] = true;
                add_collision(instance);
                remove_inactive_collision(instance); // remove from "inactive collision" world.
                add_to_instance_vectors(instance);
            }
        }

        inline void deactivate_instance(Instance* instance)
        {
            ogm_assert(instance_valid(instance->m_data.m_id));
            direct_instance_id_t id = instance->m_data.m_id;
            ogm_assert(instance_valid(id));
            ogm_assert(instance->m_data.m_frame_active);
            remove_collision(instance);
            instance->m_data.m_frame_active = false;
            m_active[id] = false;
            add_inactive_collision(instance); // add to "inactive collision" world.
        }

        inline void activate_instance(instance_id_t id)
        {
            ogm_assert(instance_valid(id));
            Instance* instance = m_instances.at(id);
            activate_instance(instance);
        }

        inline void deactivate_instance(instance_id_t id)
        {
            ogm_assert(instance_valid(id));
            Instance* instance = m_instances.at(id);
            deactivate_instance(instance);
        }

        inline asset_index_t get_object_index_from_variable(const Variable& v)
        {
            asset_index_t index = v.castCoerce<asset_index_t>();
            if (m_assets.get_asset<AssetObject*>(index))
            {
                return index;
            }

            throw MiscError("object does not exist: " + std::to_string(index));
        }

        inline std::vector<Instance*>& get_object_instances(ex_instance_id_t id)
        {
            ogm_assert(m_assets.get_asset<AssetObject*>(id));
            auto iter = m_object_instances.find(id);
            if (iter == m_object_instances.end())
            {
                auto v = new std::vector<Instance*>();
                m_object_instances[id] = v;
                return *v;
            }
            return *std::get<1>(*iter);
        }

        // returns mask, or if that is not set, returns the sprite.
        AssetSprite* get_instance_collision_mask_asset(const Instance* instance) const
        {
            asset_index_t asset = (instance->m_data.m_mask_index == k_no_asset)
                ? instance->m_data.m_sprite_index
                : instance->m_data.m_mask_index;
            if (asset == k_no_asset)
            {
                return nullptr;
            }
            return m_assets.get_asset<AssetSprite*>(asset);
        }

        AssetSprite* get_instance_collision_mask_asset(instance_id_t id) const
        {
            return get_instance_collision_mask_asset(get_instance(id));
        }

        // gets the parent asset index of the given object index, or k_no_asset if no parent.
        asset_index_t get_object_parent_index(asset_index_t asset_index) const
        {
            return m_assets.get_asset<AssetObject*>(asset_index)->m_parent;
        }

        // retrieves bytecode or ancestor's bytecode for static events
        template<ogm::asset::StaticEvent event>
        inline bytecode_index_t get_static_event_bytecode(const AssetObject* object) const
        {
            ogm_assert(event != ogm::asset::StaticEvent::COUNT);
            while (true)
            {
                if (!object)
                {
                    return k_no_event;
                }
                bytecode_index_t bc = object->static_event(event);
                if (bc != k_no_event)
                {
                    return bc;
                }
                object = m_assets.get_asset<AssetObject*>(object->m_parent);
            }
        }

        inline bytecode_index_t get_dynamic_event_bytecode(const AssetObject* object, ogm::asset::DynamicEvent event, ogm::asset::DynamicSubEvent subevent) const
        {
            StaticEvent se;
            if (event_dynamic_to_static(event, subevent, se))
            {
                return get_static_event_bytecode(object, se);
            }

            while (true)
            {
                if (!object)
                {
                    return k_no_event;
                }
                if (object->has_dynamic_event(event, subevent))
                {
                    bytecode_index_t bc = object->dynamic_event(event, subevent);
                    if (bc != k_no_event)
                    {
                        return bc;
                    }
                }
                object = m_assets.get_asset<AssetObject*>(object->m_parent);
            }
        }

        inline bytecode_index_t get_dynamic_event_bytecode(asset_index_t object_index, ogm::asset::DynamicEvent event, ogm::asset::DynamicSubEvent subevent) const
        {
            AssetObject* object = m_assets.get_asset<AssetObject*>(object_index);
            ogm_assert(object);
            return get_dynamic_event_bytecode(object, event, subevent);
        }

        inline bytecode_index_t get_static_event_bytecode(const AssetObject* object, ogm::asset::StaticEvent event) const
        {
            switch (event)
            {
            case StaticEvent::CREATE:
                return get_static_event_bytecode<StaticEvent::CREATE>(object);
            case StaticEvent::DESTROY:
                return get_static_event_bytecode<StaticEvent::DESTROY>(object);
            case StaticEvent::STEP_BEGIN:
                return get_static_event_bytecode<StaticEvent::STEP_BEGIN>(object);
            case StaticEvent::STEP:
                return get_static_event_bytecode<StaticEvent::STEP>(object);
            case StaticEvent::STEP_END:
                return get_static_event_bytecode<StaticEvent::STEP_END>(object);
            case StaticEvent::STEP_BUILTIN:
                return get_static_event_bytecode<StaticEvent::STEP_BUILTIN>(object);
            case StaticEvent::DRAW_BEGIN:
                return get_static_event_bytecode<StaticEvent::DRAW_BEGIN>(object);
            case StaticEvent::DRAW:
                return get_static_event_bytecode<StaticEvent::DRAW>(object);
            case StaticEvent::DRAW_END:
                return get_static_event_bytecode<StaticEvent::DRAW_END>(object);
            default:
                ogm_assert(false);
                return false;
            }
        }

        template<ogm::asset::StaticEvent event>
        inline bytecode_index_t get_static_event_bytecode(asset_index_t object_index) const
        {
            AssetObject* object = m_assets.get_asset<AssetObject*>(object_index);
            ogm_assert(object);
            return get_static_event_bytecode<event>(object);
        }

        template<typename Asset>
        inline asset_index_t get_asset_index_from_variable(const Variable& v_asset_index)
        {
            asset_index_t asset_index = v_asset_index.castCoerce<asset_index_t>();
            Asset* a = m_assets.get_asset<Asset*>(asset_index);
            if (!a)
            {
                throw MiscError("Asset does not exist.");
            }
            return asset_index;
        }

        template<typename Asset>
        inline Asset* get_asset_from_variable(const Variable& v_asset_index)
        {
            asset_index_t asset_index = v_asset_index.castCoerce<asset_index_t>();
            Asset* a = m_assets.get_asset<Asset*>(asset_index);
            if (!a)
            {
                throw MiscError("Asset does not exist.");
            }
            return a;
        }

        template<typename Asset>
        inline Asset* get_asset_from_variable(const Variable& v_asset_index, asset_index_t& out_asset_index)
        {
            out_asset_index = v_asset_index.castCoerce<asset_index_t>();
            Asset* a = m_assets.get_asset<Asset*>(out_asset_index);
            if (!a)
            {
                throw MiscError("Asset does not exist.");
            }
            return a;
        }

    private:

        // clears garbage (tombstones) from instance vectors.
        void remove_inactive_instances();

        // garbage collector
        void process_instance_deletion();

        // remove inactive instances from vector
        void remove_inactive_instances_from_vector(std::vector<Instance*>& vec);

        // add instance to the instance vectors
        void add_to_instance_vectors(Instance* instance);

        #ifndef DEBUG_INSTANCE_VECTORS
        void assert_instance_vector_does_not_contain(Instance* instance);
        #endif

    public:
        // produces a collision entity for the given instance, factoring in its sprite, mask, scale, angle, etc.
        inline CollisionEntity instance_collision_entity(const Instance* instance, geometry::Vector<coord_t> position)
        {
            const AssetSprite* mask = get_instance_collision_mask_asset(instance);
            if (mask && instance->m_data.m_scale.x != 0 && instance->m_data.m_scale.y != 0)
            {
                geometry::AABB<coord_t> aabb_scaled = (mask->m_aabb + (-mask->m_offset)) * (instance->m_data.m_scale);
                double angle = -instance->m_data.m_angle * TAU / 360.0; // right-handed radians
                geometry::AABB<coord_t> aabb = aabb_scaled.rotated(angle);
                aabb = aabb + position;
                switch(mask->m_shape)
                {
                case asset::AssetSprite::rectangle:
                    if (angle == 0)
                    {
                        // non-rotated rectangle is just an aabb.
                        return {
                            collision::ShapeType::rectangle,
                            aabb,
                            instance->m_data.m_id
                        };
                    }
                    else
                    {
                        Triangle<coord_t> a {
                            aabb_scaled.top_left(),
                            aabb_scaled.top_right(),
                            aabb_scaled.bottom_left()
                        };

                        Triangle<coord_t> b {
                            aabb_scaled.bottom_left(),
                            aabb_scaled.top_right(),
                            aabb_scaled.bottom_right()
                        };
                        a = a.rotated(angle);
                        b = b.rotated(angle);
                        a = a + position;
                        b = b + position;
                        a.make_ccw();
                        b.make_ccw();
                        Triangle<coord_t> t[2] = { a, b };
                        return {
                            collision::ShapeType::triangles2,
                            aabb,
                            t,
                            instance->m_data.m_id
                        };
                    }
                    break;
                case asset::AssetSprite::diamond:
                    {
                        Triangle<coord_t> a {
                            aabb_scaled.top_center(),
                            aabb_scaled.left_center(),
                            aabb_scaled.right_center()
                        };

                        Triangle<coord_t> b {
                            aabb_scaled.left_center(),
                            aabb_scaled.right_center(),
                            aabb_scaled.bottom_center()
                        };
                        a = a.rotated(angle);
                        b = b.rotated(angle);
                        a = a + position;
                        b = b + position;
                        a.make_ccw();
                        b.make_ccw();
                        Triangle<coord_t> t[2] = { a, b };
                        return {
                            collision::ShapeType::triangles2,
                            aabb,
                            t,
                            instance->m_data.m_id
                        };
                    }
                    break;
                case asset::AssetSprite::raster:
                    {
                        ogm_assert(!mask->m_raster.empty());
                        const auto& raster = mask->m_raster.at(
                            static_cast<uint32_t>(std::floor(instance->m_data.m_image_index))
                            % mask->m_raster.size()
                        );

                        collision::TransformedCollisionRaster tcr{
                            &raster,
                            mask->m_offset,
                            instance->m_data.m_scale,
                            angle,
                            position
                        };

                        return {
                            collision::ShapeType::raster,
                            aabb,
                            std::move(tcr),
                            instance->m_data.m_id
                        };
                    }
                    break;
                case asset::AssetSprite::ellipse:
                    {
                        Ellipse e {
                            aabb.get_center(),
                            aabb_scaled.diagonal() * 0.5,
                            angle
                        };

                        return {
                            collision::ShapeType::raster,
                            aabb,
                            std::move(e),
                            instance->m_data.m_id
                        };
                    }
                    break;
                default:
                    throw MiscError("Unknown collision type.");
                }
            }
            else
            {
                return { collision::ShapeType::count, instance->m_data.m_id };
            }
        }

        inline CollisionEntity instance_collision_entity(const Instance* instance)
        {
            return instance_collision_entity(instance, instance->m_data.m_position);
        }

        // endows entity with collision
        void add_collision(Instance* instance)
        {
            ogm_assert(instance->m_data.m_frame_collision_id == -1);
            ogm_assert(instance_active(instance));
            CollisionEntity entity = instance_collision_entity(instance);
            if (entity.m_shape != collision::ShapeType::count)
            {
                instance->m_data.m_frame_collision_id = m_collision.emplace_entity(entity);
            }
        }

        // when QUEUE_COLLISION_UPDATES is enabled, updates to the collision world
        // are done all at once when they are needed, rather than online.
        // This is more efficient (as some updates supercede earlier ones.)
        // This should be called every time before the collision world is queried.
        void process_collision_updates()
        {
            #ifdef QUEUE_COLLISION_UPDATES
            for (Instance* instance : m_queued_collision_updates)
            {
                instance->m_data.m_collision_queued = false;
                update_collision(instance);
            }

            m_queued_collision_updates.clear();
            #endif
        }

        // either updates the instance's collision body immediately, or,
        // if QUEUE_COLLISION_UPDATES is enabled, it will queue the update to
        // be processed later when it is needed.
        void queue_update_collision(Instance* instance)
        {
            #ifdef QUEUE_COLLISION_UPDATES
            // defer update until later.
            if (!instance->m_data.m_collision_queued)
            {
                instance->m_data.m_collision_queued = true;
                m_queued_collision_updates.push_back(instance);
            }
            #else

            // update collision immediately.
            update_collision(instance);
            #endif
        }

        template<bool write>
        void serialize(typename state_stream<write>::state_stream_t& s);
private:
        // updates collision data for instance.
        void update_collision(Instance* instance)
        {
            if (!instance_active(instance->m_data.m_id))
            {
                // instances which are inactive do not exist in the collision world.
                // TODO: update inactive_collision instead?
                return;
            }

            // makes a new collision entity for the instance.
            CollisionEntity entity = instance_collision_entity(instance);

            if (instance->m_data.m_frame_collision_id == -1)
            // first time being added to the collision world.
            {
                if (entity.m_shape != collision::ShapeType::count)
                {
                    // add new data
                    instance->m_data.m_frame_collision_id = m_collision.emplace_entity(entity);
                }
            }
            else
            // already exists in the collision world; overwrite the old data.
            {
                if (entity.m_shape == collision::ShapeType::count)
                {
                    // remove old data without replacement
                    m_collision.remove_entity(instance->m_data.m_frame_collision_id);
                    instance->m_data.m_frame_collision_id = -1;
                }
                else
                {
                    // edit existing data.
                    m_collision.replace_entity(instance->m_data.m_frame_collision_id, entity);
                }
            }
        }

        // removes collision from entity.
        void remove_collision(Instance* instance)
        {
            // just in case any collision updates involve this entity, process those first to be safe.
            process_collision_updates();
            ogm_assert(instance_valid(instance->m_data.m_id));
            if (instance->m_data.m_frame_collision_id != static_cast<decltype(instance->m_data.m_frame_collision_id)>(-1))
            {
                m_collision.remove_entity(instance->m_data.m_frame_collision_id);
                instance->m_data.m_frame_collision_id = -1;
            }
        }

        // there is an entirely orthogonal "inactive collision" world, for
        // inactive entities, so that they can be located for instance_activate_region.
        void add_inactive_collision(Instance* instance)
        {
            ogm_assert(instance->m_data.m_frame_inactive_collision_id == static_cast<decltype(instance->m_data.m_frame_inactive_collision_id)>(-1));
            ogm_assert(instance_valid(instance));
            ogm_assert(!instance_active(instance));
            CollisionEntity entity = instance_collision_entity(instance);
            if (entity.m_shape != collision::ShapeType::count)
            {
                instance->m_data.m_frame_inactive_collision_id = m_inactive_collision.emplace_entity(entity);
            }
        }

        void remove_inactive_collision(Instance* instance)
        {
            if (instance->m_data.m_frame_inactive_collision_id != static_cast<decltype(instance->m_data.m_frame_inactive_collision_id)>(-1))
            {
                m_inactive_collision.remove_entity(instance->m_data.m_frame_inactive_collision_id);
                instance->m_data.m_frame_inactive_collision_id = -1;
            }
        }

        // use for deserialization.
        void recalculate_collision();

public:
        // switches to the appropriate room
        // warning: this invalidates any active with-iterators!
        // Therefore, it's best if this is only called by top-level code during
        // specific parts of the tick.
        void change_room(asset_index_t room_index);

        inline tile_id_t add_tile(Tile&& tile)
        {
            m_tiles.add_tile_as(std::move(tile), m_config.m_next_instance_id);
            return m_config.m_next_instance_id++;
        }

    public:
        //// member variables ////
        DataStructureManager<DSList> m_ds_list;
        DataStructureManager<DSMap> m_ds_map;
        DataStructureManager<DSGrid> m_ds_grid;
        DataStructureManager<DSStack> m_ds_stack;
        DataStructureManager<DSQueue> m_ds_queue;
        DataStructureManager<DSPriorityQueue> m_ds_priority;
        BufferManager m_buffers;
        NetworkManager m_network;
        Filesystem m_fs;
        AssetTable m_assets;
        BytecodeTable m_bytecode;
        ReflectionAccumulator* m_reflection = nullptr;
        collision::World<real_t, direct_instance_id_t, collision::SpatialHash<coord_t, 64951>> m_collision;
        collision::World<real_t, direct_instance_id_t, collision::SpatialHash<coord_t, 127>> m_inactive_collision;
        Display* m_display;
        ogm::asset::Config m_config;

        TileWorld m_tiles;
        std::vector<BackgroundLayer> m_background_layers;

        struct EventContext
        {
            DynamicEvent m_event;
            DynamicSubEvent m_sub_event;
            asset_index_t m_object;
        };

        // global data
        static constexpr size_t k_view_count = 16;
        struct
        {
            bool m_prg_end = false; // true if the program should finish
            bool m_prg_reset = false; // true if the program should reset
            bool m_views_enabled = false;
            bool m_view_visible[k_view_count];
            geometry::Vector<coord_t> m_view_position[k_view_count];
            geometry::Vector<coord_t> m_view_dimension[k_view_count];
            real_t m_view_angle[k_view_count];
            real_t m_desired_fps = 30;
            size_t m_view_current = 0;
            asset_index_t m_room_index{ k_no_asset };
            bool m_show_background_colour = false;
            int32_t m_background_colour = 0;
            geometry::Vector<coord_t> m_room_dimension;
            std::vector<std::string> m_clargs;
            asset_index_t m_room_goto_queued = k_no_asset;
            EventContext m_event_context;
        } m_data;

    private:
        SparseContiguousMap<variable_id_t, Variable> m_globals;

        std::map<direct_instance_id_t, bool> m_valid;
        std::map<direct_instance_id_t, bool> m_active;

    public:
        ///////////////// lists of instances /////////////////

        // all valid instances (even inactive ones)
        std::map<direct_instance_id_t, Instance*> m_instances;
        std::vector<direct_instance_id_t> m_instances_delete;

        // Each phase, the list of all instances is sorted by depth
        // and by resource-order in these arrays:

        std::vector<Instance*> m_depth_sorted_instances; // decreasing, iterated forward
        std::vector<Instance*> m_resource_sorted_instances; // increasing, iterated forward

        // instances who should be notified if a key or mouse button is pressed
        std::vector<Instance*> m_input_listener_instances; // TODO: what is traversal order?

        // instances who should be notified if an async event occurs
        std::vector<Instance*> m_async_listener_instances; // TODO: what is traversal order?

        // these vectors are never sorted, but iterated through in reverse order.
        // they contain all instances of the given object or descendants.
        std::map<asset_index_t, std::vector<Instance*>*> m_object_instances;

        // list of instances whose collision updates are queued.
        #ifdef QUEUE_COLLISION_UPDATES
        std::vector<Instance*> m_queued_collision_updates;
        #endif
    };

    // functions accessible to Instance.hpp
    namespace FrameImpl
    {
        inline void queue_update_collision(Frame* f, Instance* i)
        {
            f->queue_update_collision(i);
        }

        inline bytecode_index_t get_event_static_bytecode(Frame*f, AssetObject* object, StaticEvent event)
        {
            return f->get_static_event_bytecode(object, event);
        }

        inline bytecode_index_t get_event_dynamic_bytecode(Frame*f, AssetObject* object, DynamicEvent ev, DynamicSubEvent sev)
        {
            StaticEvent static_event;
            if (event_dynamic_to_static(ev, sev, static_event))
            {
                return f->get_static_event_bytecode(object, static_event);
            }
            else
            {
                throw NotImplementedError("Dynamic bytecode event");
            }
        }

        inline asset::AssetTable* get_assets(Frame* f)
        {
            return &(f->m_assets);
        }

        inline bytecode::ReflectionAccumulator* get_reflection(Frame* f)
        {
            return (f->m_reflection);
        }
    }
}}
