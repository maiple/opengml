#pragma once

#include "ogm/geometry/aabb.hpp"
#include "ogm/geometry/Triangle.hpp"

#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <cassert>
#include <vector>

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize ("Ofast")
#endif

#include "SpatialHash.hpp"

namespace ogm
{
namespace collision
{
using namespace geometry;

enum class ShapeType
{
    // axis-aligned bounding box
    rectangle,

    // pair of triangles
    triangles2,

    // ellipse
    ellipse,

    // image data
    raster,
    count
};

typedef uint32_t entity_id_t;

template<typename coord_t, typename payload_t>
class Entity
{
public:
    // the type of shape this entity is
    ShapeType m_shape;

    // bounding box which completely contains the entity
    AABB<coord_t> m_aabb;

    // shape data
    union
    {
        SmallTriangleList<2, coord_t> m_triangles2;

        // TODO: ellipse and pixel data
    };

    payload_t m_payload;

    Entity(ShapeType st, payload_t payload)
        : m_shape{ st }
        , m_payload(payload)
    {
        assert(st == ShapeType::count);
    }

    Entity(const Entity& other)
        : m_shape{ other.m_shape }
        , m_aabb{ other.m_aabb }
        , m_triangles2{ m_triangles2 }
        , m_payload{ other.m_payload }
    { }

    Entity(ShapeType st, AABB<coord_t> aabb, payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_payload(payload)
    {
        assert(st == ShapeType::count || st == ShapeType::rectangle);
    }

    Entity(ShapeType st, AABB<coord_t> aabb, Triangle<coord_t> triangles[2], payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_triangles2{ triangles }
        , m_payload(payload)
    {
        assert(st == ShapeType::triangles2);
    }

    // does this entity meet the given position?
    template<bool precise=true>
    bool collides_vector(Vector<coord_t> v) const
    {
        if (m_shape == ShapeType::count)
        {
            return false;
        }

        if (!m_aabb.contains(v))
        {
            return false;
        }

        if (!precise) return true;

        switch (m_shape)
        {
        case ShapeType::rectangle:
            {
                // aabb known to contain this point, so we are done.
                // (other shapes require more work.)
                return true;
            }
        case ShapeType::triangles2:
            {
                return m_triangles2.contains(v);
            }
        case ShapeType::ellipse:
            {
                Vector<coord_t> center = m_aabb.get_center();
                Vector<coord_t> offset = v - center;
                Vector<coord_t> dim = m_aabb.get_dimensions();
                if (dim.x == 0 || dim.y == 0)
                {
                    return false;
                }
                else
                {
                    offset.x /= dim.x;
                    offset.y /= dim.y;
                    return (offset.lengthsq()) < 1;
                }
            }
        default:
            throw MiscError("point collision not implemented yet for this shape.");
            return true;
        }
    }

    // does this entity intersect the given line?
    template<bool precise=true>
    bool collides_line(Vector<coord_t> v1, Vector<coord_t> v2) const
    {
        if (m_shape == ShapeType::count)
        {
            return false;
        }

        if (!m_aabb.intersects_line(v1, v2))
        {
            return false;
        }

        if (!precise) return true;

        switch (m_shape)
        {
        case ShapeType::rectangle:
            {
                // aabb known to contain this point, so we are done.
                // (other shapes require more work.)
                return true;
            }
        default:
            throw MiscError("line collision not implemented yet for this shape.");
            return true;
        }
    }

    template<bool precise=true>
    bool collides_entity(const Entity& other) const
    {
        if (m_shape == ShapeType::count || other.m_shape == ShapeType::count)
        {
            return false;
        }

        if (!m_aabb.intersects(other.m_aabb))
        {
            return false;
        }

        if (!precise) return true;

        switch (enum_pair(m_shape, other.m_shape))
        {
        case enum_pair(ShapeType::rectangle, ShapeType::rectangle):
            {
                // aabb already overlaps!
                return true;
            }
        default:
            throw MiscError("collision not implemented yet for this pair.");
            return true;
        }
    }

};

template<typename coord_t, typename payload_t, typename spatial_index_t>
class World
{
    typedef Entity<coord_t, payload_t> entity_t;

private:
    std::vector<entity_t> m_entities;

    // spatial index (indexes into m_entities)
    spatial_index_t m_si;

public:
    inline entity_id_t emplace_entity(const entity_t& entity)
    {
        assert(entity.m_shape != ShapeType::count);
        entity_id_t id = m_entities.size();
        m_entities.emplace_back(entity);
        activate_entity(id);
        return id;
    }

    void replace_entity(entity_id_t id, const entity_t& replacement)
    {
        assert(entity_exists(id));
        assert(replacement.m_shape != ShapeType::count);
        deactivate_entity(id);
        m_entities[id] = replacement;
        activate_entity(id);
    }

    void remove_entity(entity_id_t id)
    {
        assert(entity_exists(id));
        m_entities[id].m_shape = ShapeType::count;
    }

    void clear()
    {
        m_entities.clear();
    }

    template<typename C>
    inline void iterate_entity(entity_t entity, C callback, bool precise)
    {
        if (precise)
        {
            _iterate_entity<C, true>(entity, callback);
        }
        else
        {
            _iterate_entity<C, false>(entity, callback);
        }
    }

    template<typename C>
    inline void iterate_vector(Vector<coord_t> position, C callback, bool precise)
    {
        if (precise)
        {
            _iterate_vector<C, true>(position, callback);
        }
        else
        {
            _iterate_vector<C, false>(position, callback);
        }
    }

    template<typename C>
    inline void iterate_line(Vector<coord_t> start, Vector<coord_t> end, C callback, bool precise)
    {
        if (precise)
        {
            _iterate_line<C, true>(start, end, callback);
        }
        else
        {
            _iterate_line<C, false>(start, end, callback);
        }
    }

private:
    // iterate over entities which collide with this entity
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C, bool precise>
    inline void _iterate_entity(entity_t entity, C callback)
    {
        auto aabb = entity.m_aabb;
        m_si.foreach_leaf_in_aabb(aabb,
            [this, &callback, &entity](entity_id_t id) -> bool
            {
                const entity_t& _entity = this->m_entities.at(id);
                if (entity.template collides_entity<precise>(_entity))
                {
                    return callback(id, _entity);
                }

                // continue
                return true;
            }
        );
    }

    // iterate over entities which collide with this position
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C, bool precise>
    inline void _iterate_vector(Vector<coord_t> position, C callback)
    {
        auto node_coord = m_si.node_coord(position);
        if (precise)
        {
            for (entity_id_t id : m_si.get_leaves_at_node(node_coord))
            {
                const entity_t& entity = m_entities.at(id);
                if (entity.template collides_vector<precise>(position))
                {
                    if (callback(id, entity))
                    {
                        // early-out.
                        return;
                    }
                }
            }
        }
    }

    // iterate over entities which collide with this line
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C, bool precise>
    inline void _iterate_line(Vector<coord_t> start, Vector<coord_t> end, C callback)
    {
        // OPTIMIZE: just check the spatial hash nodes which intersect the line.
        geometry::AABB<coord_t> aabb{ start, end};
        aabb.correct_sign();
        m_si.foreach_leaf_in_aabb(aabb,
            [this, &callback, &start, &end](entity_id_t id) -> bool
            {
                const entity_t& entity = this->m_entities.at(id);
                if (entity.template collides_line<precise>(start, end))
                {
                    return callback(id, entity);
                }

                // continue
                return true;
            }
        );
    }

    // adds entity to spatial hash
    inline void activate_entity(entity_id_t id)
    {
        m_si.insert(id, m_entities.at(id).m_aabb);
    }

    // removes entity from spatial hash
    inline void deactivate_entity(entity_id_t id)
    {
        m_si.remove(id);
    }

    inline bool entity_exists(entity_id_t id) const
    {
        return id < m_entities.size() && m_entities.at(id).m_shape != ShapeType::count;
    }
};

}
}

#ifdef __GNUC__
#pragma GCC pop_options
#endif
