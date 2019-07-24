#pragma once

#include "ogm/geometry/aabb.hpp"

#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include <rstartree/RStarTree.h>

#include <cassert>
#include <vector>

namespace ogm
{
namespace collision
{
using namespace geometry;

enum class Shape
{
    rectangle,
    ellipse,
    diamond,
    precise,
    count
};

typedef uint32_t entity_id_t;

template<typename coord_t, typename payload_t>
class Entity
{
public:
    Shape m_shape;
    AABB<coord_t> m_aabb;
    payload_t m_payload;

    // does this entity meet the given position?
    template<bool precise=true>
    bool collides_vector(Vector<coord_t> v) const
    {
        if (m_shape == Shape::count)
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
        case Shape::rectangle:
            {
                // aabb known to contain this point, so we are done.
                // (other shapes require more work.)
                return true;
            }
        case Shape::ellipse:
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
        if (m_shape == Shape::count)
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
        case Shape::rectangle:
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
        if (m_shape == Shape::count || other.m_shape == Shape::count)
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
        case enum_pair(Shape::rectangle, Shape::rectangle):
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

template<typename coord_t, typename payload_t>
class World
{
    typedef Entity<coord_t, payload_t> entity_t;

private:
    std::vector<entity_t> m_entities;

    // spatial index (indexes into m_entities)
    typedef RStarTree<entity_id_t, 2, 8, 32> rst_t;
    typedef typename rst_t::BoundedItem rst_bitem_t;
    typedef typename rst_t::BoundingBox rst_bbox_t;
    rst_t m_rst;

public:
    inline entity_id_t emplace_entity(const entity_t& entity)
    {
        assert(entity.m_shape != Shape::count);
        entity_id_t id = m_entities.size();
        m_entities.emplace_back(entity);
        activate_entity(id);
        return id;
    }

    void replace_entity(entity_id_t id, const entity_t& replacement)
    {
        assert(entity_exists(id));
        assert(replacement.m_shape != Shape::count);
        deactivate_entity(id);
        m_entities[id] = replacement;
        activate_entity(id);
    }

    void remove_entity(entity_id_t id)
    {
        assert(entity_exists(id));
        m_entities[id].m_shape = Shape::count;
    }

    void clear()
    {
        m_entities.clear();
    }

    template<typename C>
    struct LambdaAcceptor
    {
        C m_c;

        bool operator()(const rst_t::Node* const node) const
        {
            return m_c(node->bound);
        }

        bool operator()(const rst_t::Leaf * const leaf) const
    	{
    		return m_c(leaf->bound);
    	}
    };

    template<typename C>
    LambdaAcceptor<C> lambda_acceptor(C c)
    {
        return LambdaAcceptor<C>{ c };
    }

    template<typename C>
    struct LambdaVisitor
    {
        World<coord_t, payload_t>& m_world;
        C m_c;
        mutable bool ContinueVisiting;

        void operator()(const rst_t::Leaf* const leaf) const
        {
            if (!ContinueVisiting) return;
            ContinueVisiting = m_c(leaf->leaf, m_world.m_entities.at(leaf->leaf));
        }
    };

    template<typename C>
    LambdaVisitor<C> lambda_visitor(C c)
    {
        return LambdaVisitor<C>{ *this, c, true };
    }

    static geometry::AABB<coord_t> bbox_to_aabb(const rst_bbox_t& bbox)
    {
        return
        {
            bbox.edges[0].first,
            bbox.edges[0].second,
            bbox.edges[1].first,
            bbox.edges[1].second
        };
    }

    // iterate over entities which collide with this entity
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C>
    void iterate_entity(entity_t entity, C callback)
    {
        m_rst.Query(
            // acceptor
            lambda_acceptor([this, &entity](rst_bbox_t bbox) -> bool
            {
                auto aabb = bbox_to_aabb(bbox);
                return aabb.intersects(entity.m_aabb);
            }),

            // visitor
            lambda_visitor(callback)
        );
    }

    // iterate over entities which collide with this position
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C>
    void iterate_vector(Vector<coord_t> position, C callback)
    {
        m_rst.Query(
            // acceptor
            lambda_acceptor([this, &position](rst_bbox_t bbox) -> bool
            {
                auto aabb = bbox_to_aabb(bbox);
                return aabb.contains(position);
            }),

            // visitor
            lambda_visitor(callback)
        );
    }

    // iterate over entities which collide with this line
    // callback is (entity_id_t, entity_t) -> bool, return false to stop.
    template<typename C>
    void iterate_line(Vector<coord_t> start, Vector<coord_t> end, C callback)
    {
        m_rst.Query(
            // acceptor
            lambda_acceptor([this, &start, & end](rst_bbox_t bbox) -> bool
            {
                auto aabb = bbox_to_aabb(bbox);
                return aabb.intersects_line(start, end);
            }),

            // visitor
            lambda_visitor(callback)
        );
    }

private:

    inline rst_bbox_t entity_bounds(entity_id_t id)
    {
        auto aabb = m_entities.at(id).m_aabb;
        rst_bbox_t bbox;
        bbox.edges[0].first = aabb.m_start.x;
        bbox.edges[0].second = aabb.m_start.y;
        bbox.edges[1].first = aabb.m_end.x;
        bbox.edges[1].second = aabb.m_end.y;
        return bbox;
    }

    // adds entity to spatial hash
    inline void activate_entity(entity_id_t id)
    {
        m_rst.Insert(id, entity_bounds(id));
    }

    // removes entity from spatial hash
    inline void deactivate_entity(entity_id_t id)
    {
        m_rst.RemoveItem(id);
    }

    inline bool entity_exists(entity_id_t id) const
    {
        return id < m_entities.size() && m_entities.at(id).m_shape != Shape::count;
    }
};

}
}
