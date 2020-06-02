#pragma once

#include "ogm/geometry/aabb.hpp"
#include "ogm/geometry/Triangle.hpp"
#include "ogm/geometry/Ellipse.hpp"

#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include "ogm/common/error.hpp"
#include <vector>

#ifdef OPTIMIZE_COLLISION
#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize ("Ofast")
#endif
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

// collision data for a pixel-precise sprite
struct CollisionRaster
{
    uint32_t m_width;
    uint32_t m_length;
    // width-major order
    bool* m_data;

    bool index(int64_t x, int64_t y, uint64_t& o_index) const
    {
        if (x < 0 || y < 0)
        {
            return false;
        }
        if (x >= m_width)
        {
            return false;
        }
        o_index = y * m_width + x;
        if (o_index >= m_length)
        {
            return false;
        }
        return true;
    }

    bool at(int64_t x, int64_t y) const
    {
        uint64_t i;
        if (!index(x, y, i))
        {
            return false;
        }
        return m_data[i];
    }
    
    void set(int64_t x, int64_t y, bool c)
    {
        uint64_t i;
        if (index(x, y, i))
        {
            m_data[i] = c;
        }
    }
    
    void set(uint64_t i, bool c)
    {
        if (i < m_length)
        {
            m_data[i] = c;
        }
    }
};

struct TransformedCollisionRaster
{
    // non-owning pointer to collision raster data
    const CollisionRaster* m_raster;

    /// transformation: collision raster has the following operations applied in order.
    geometry::Vector<coord_t> m_scale;

    // angle
    real_t m_cos, m_sin;
    geometry::Vector<coord_t> m_translation;
    geometry::Vector<coord_t> m_offset;

    TransformedCollisionRaster(const TransformedCollisionRaster& other)=default;
    TransformedCollisionRaster(
        const CollisionRaster* data,
        geometry::Vector<coord_t> origin,
        geometry::Vector<coord_t> scale,
        double theta, // right-handed radians
        geometry::Vector<coord_t> translation
    )
        : m_raster(data)
        , m_scale{ 1.0/scale.x, 1.0/scale.y }
        , m_cos(std::cos(-theta))
        , m_sin(std::sin(-theta))
        , m_translation(translation)
        , m_offset(origin)
    {
        ogm_assert(m_raster);
        ogm_assert(m_raster->m_data);
    }

    bool collides_point(geometry::Vector<coord_t> p) const
    {
        p -= m_translation;
        geometry::Vector<coord_t> q { p.x * m_cos - p.y * m_sin, p.y * m_cos + p.x * m_sin };
        q.scale_apply(m_scale);
        q += m_offset;
        int64_t x = std::floor(q.x);
        int64_t y = std::floor(q.y);
        return m_raster->at(x, y);
    }

    bool collides_in_aabb(geometry::AABB<coord_t> aabb_intersect) const
    {
        // sufficient? only 1-pixel intervals...
        for (coord_t x = aabb_intersect.m_start.x; x < aabb_intersect.m_end.x; x += 1.0)
        {
            for (coord_t y = aabb_intersect.m_start.y; y < aabb_intersect.m_end.y; y += 1.0)
            {
                if (!collides_point({x, y}))
                {
                    continue;
                }

                return true;
            }
        }
        return false;
    }

    bool collides_line(Vector<coord_t> v1, Vector<coord_t> v2) const
    {
        real_t len = (v1 - v2).length();
        for (real_t lambda = 0; lambda <= 1.0; lambda += std::max(1/len, 0.00001))
        {
            Vector<coord_t> v = v1 * (1.0 - lambda) + v2 * lambda;
            if (collides_point(v))
            {
                return true;
            }
        }

        return false;
    }
};

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
        TransformedCollisionRaster m_transformed_raster;
        Ellipse<coord_t>  m_ellipse;
    };

    payload_t m_payload;

    Entity(ShapeType st, payload_t payload)
        : m_shape{ st }
        , m_payload(payload)
    {
        ogm_assert(st == ShapeType::count);
    }

    Entity(const Entity& other)
        : m_shape{ other.m_shape }
        , m_aabb{ other.m_aabb }
        , m_payload{ other.m_payload }
    {
        ogm_assert(sizeof(m_triangles2) >= sizeof(m_transformed_raster));
        // biggest member of union
        memcpy(&m_triangles2, &other.m_triangles2,
            sizeof(m_triangles2)
        );
    }

    Entity(ShapeType st, AABB<coord_t> aabb, payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_payload(payload)
    {
        ogm_assert(st == ShapeType::count || st == ShapeType::rectangle);
    }

    Entity(ShapeType st, AABB<coord_t> aabb, Triangle<coord_t> triangles[2], payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_triangles2{ triangles }
        , m_payload(payload)
    {
        ogm_assert(st == ShapeType::triangles2);
    }

    Entity(ShapeType st, AABB<coord_t> aabb, TransformedCollisionRaster&& raster, payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_transformed_raster{ raster }
        , m_payload(payload)
    {
        ogm_assert(st == ShapeType::raster);
    }

    Entity(ShapeType st, AABB<coord_t> aabb, Ellipse<coord_t>&& ellipse, payload_t payload)
        : m_shape{ st }
        , m_aabb(aabb)
        , m_ellipse{ ellipse }
        , m_payload(payload)
    {
        ogm_assert(st == ShapeType::ellipse);
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
                return m_ellipse.contains(v);
            }
        case ShapeType::raster:
            {
                return m_transformed_raster.collides_point(v);
            }
            break;
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
        case ShapeType::raster:
            {
                // TODO: cull line to aabb for quicker checking.
                return m_transformed_raster.collides_line(v1, v2);
            }
        case ShapeType::ellipse:
            {
                return m_ellipse.collides_line_segment(v1, v2);
            }
        default:
            // TODO
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
        case enum_pair(ShapeType::rectangle, ShapeType::triangles2):
            {
                return other.m_triangles2.intersects(SmallTriangleList<2>{m_aabb});
            }
        case enum_pair(ShapeType::rectangle, ShapeType::raster):
            {
                return other.m_transformed_raster.collides_in_aabb(
                    m_aabb.intersection(other.m_aabb)
                );
            }
        case enum_pair(ShapeType::rectangle, ShapeType::ellipse):
            {
                return other.m_ellipse.collides_aabb(
                    m_aabb
                );
            }
        case enum_pair(ShapeType::triangles2, ShapeType::rectangle):
            {
                return m_triangles2.intersects(SmallTriangleList<2>{other.m_aabb});
            }
        case enum_pair(ShapeType::triangles2, ShapeType::triangles2):
            {
                return m_triangles2.intersects(other.m_triangles2);
            }
        case enum_pair(ShapeType::raster, ShapeType::rectangle):
            {
                return m_transformed_raster.collides_in_aabb(
                    m_aabb.intersection(other.m_aabb)
                );
            }
        case enum_pair(ShapeType::raster, ShapeType::triangles2):
            {
                return collision_raster_triangles2(other);
            }
        case enum_pair(ShapeType::triangles2, ShapeType::raster):
            {
                return other.collision_raster_triangles2(*this);
            }
        case enum_pair(ShapeType::raster, ShapeType::raster):
            {
                AABB<coord_t> aabb_intersect = m_aabb.intersection(other.m_aabb);
                // sufficient? only 1-pixel intervals...
                for (coord_t x = aabb_intersect.m_start.x; x < aabb_intersect.m_end.x; x += 1.0)
                {
                    for (coord_t y = aabb_intersect.m_start.y; y < aabb_intersect.m_end.y; y += 1.0)
                    {
                        if (!m_transformed_raster.collides_point({x, y}))
                        {
                            continue;
                        }

                        if (!other.m_transformed_raster.collides_point({x, y}))
                        {
                            continue;
                        }

                        return true;
                    }
                }
                return false;
            }
        case enum_pair(ShapeType::ellipse, ShapeType::rectangle):
            {
                return m_ellipse.collides_aabb(
                    other.m_aabb
                );
            }
        case enum_pair(ShapeType::ellipse, ShapeType::ellipse):
            {
                // TODO
                return m_ellipse.collides_aabb(
                    other.m_aabb
                ) && other.m_ellipse.collides_aabb(
                    m_aabb
                );
            }
        default:
            // TODO
            return true;
        }
    }

private:
    FORCEINLINE bool collision_raster_triangles2(const Entity& other) const
    {
        ogm_assert(m_shape == ShapeType::raster);
        ogm_assert(other.m_shape == ShapeType::triangles2);
        AABB<coord_t> aabb_intersect = m_aabb.intersection(other.m_aabb);
        // sufficient? only 1-pixel intervals...
        for (coord_t x = aabb_intersect.m_start.x; x < aabb_intersect.m_end.x; x += 1.0)
        {
            for (coord_t y = aabb_intersect.m_start.y; y < aabb_intersect.m_end.y; y += 1.0)
            {
                if (!m_transformed_raster.collides_point({x, y}))
                {
                    continue;
                }

                if (!other.m_triangles2.contains({x, y}))
                {
                    continue;
                }

                return true;
            }
        }
        return false;
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
        ogm_assert(entity.m_shape != ShapeType::count);
        entity_id_t id = m_entities.size();
        m_entities.emplace_back(entity);
        activate_entity(id);
        return id;
    }

    void replace_entity(entity_id_t id, const entity_t& replacement)
    {
        ogm_assert(entity_exists(id));
        ogm_assert(replacement.m_shape != ShapeType::count);
        deactivate_entity(id);
        m_entities[id] = replacement;
        activate_entity(id);
    }

    void remove_entity(entity_id_t id)
    {
        ogm_assert(entity_exists(id));
        deactivate_entity(id);
        m_entities[id].m_shape = ShapeType::count;
    }

    void clear()
    {
        m_entities.clear();
        m_si.clear();
    }

    template<typename C>
    inline void iterate_entity(const entity_t& entity, C callback, bool precise) const
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
    inline void iterate_vector(Vector<coord_t> position, C callback, bool precise) const
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
    inline void iterate_line(Vector<coord_t> start, Vector<coord_t> end, C callback, bool precise) const
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
    inline void _iterate_entity(const entity_t& entity, C callback) const
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
    inline void _iterate_vector(Vector<coord_t> position, C callback) const
    {
        auto node_coord = m_si.node_coord(position);
        for (entity_id_t id : m_si.get_leaves_at_node(node_coord))
        {
            const entity_t& entity = m_entities.at(id);
            if (entity.template collides_vector<precise>(position))
            {
                if (!callback(id, entity))
                {
                    // early-out.
                    return;
                }
            }
        }
    }

    // iterate over entities which collide with this line
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C, bool precise>
    inline void _iterate_line(Vector<coord_t> start, Vector<coord_t> end, C callback) const
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

#ifdef OPTIMIZE_COLLISION
#ifdef __GNUC__
#pragma GCC pop_options
#endif
#endif
