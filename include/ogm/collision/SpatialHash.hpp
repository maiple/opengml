#pragma once

#include "ogm/geometry/aabb.hpp"

#include <vector>
#include <cassert>

namespace ogm { namespace collision {

    // it is the caller's responsibility to ensure size_t indices are approximately
    // contiguous (that they don't grow too out of bounds,)
    // and that indices are removed before they are added.
template<typename coord_t, uint16_t bucket_count>
class SpatialHash
{
public:
    typedef geometry::AABB<coord_t> aabb_t;
    typedef geometry::Vector<coord_t> vector_t;
    typedef geometry::Vector<int32_t> s32vec_t;
    typedef uint32_t leaf_index_t;
    typedef uint16_t node_index_t;

    #ifdef BOOST_SMALL_VECTOR
    typedef boost::container::small_vector<leaf_index_t, 8> leaf_vector_t;
    typedef boost::container::small_vector<node_index_t, 4> node_vector_t;
    #else
    typedef std::vector<leaf_index_t> leaf_vector_t;
    typedef std::vector<node_index_t> node_vector_t;
    #endif
private:

    // AKA bucket
    struct Node
    {
        leaf_vector_t m_leaf_indices;
    };

    struct Leaf
    {
        node_vector_t m_containers;
    };

    // a moderately large prime number which is smaller than 2^16
    static constexpr node_index_t k_bucket_count = bucket_count;

    // arbitrary, approximately the square root of k_bucket_count.
    static constexpr node_index_t m_y_mult = 256;

    // large!
    Node m_nodes[k_bucket_count];
    std::vector<Leaf> m_leaves;

    // OPTIMIZE -- this is the most important parameter,
    // the bucket side-length. Come up with a way to choose this carefully
    // or to adaptively set it.
    coord_t m_bucket_length = 96.0;
    coord_t m_rec_bucket_length = 1 / 96.0;

public:
    inline coord_t get_bucket_length()
    {
        return m_bucket_length;
    }

    void insert(leaf_index_t l, aabb_t aabb)
    {
        ensure_leaf(l);
        foreach_node_index_in_aabb(
            aabb,
            [&l, this](node_index_t n) -> bool
            {
                this->insert_directly(n, l);

                // continue
                return true;
            }
        );
    }

    void remove(leaf_index_t l)
    {
        assert(m_leaves.size() > l);
        Leaf& leaf = m_leaves[l];
        while(!leaf.m_containers.empty())
        {
            node_index_t node = leaf.m_containers.back();
            auto& node_container = m_nodes[node].m_leaf_indices;
            auto iter = std::find(node_container.begin(), node_container.end(), l);

            // assert leaf was in container before it is removed.
            assert(iter != node_container.end());
            node_container.erase(iter);

            leaf.m_containers.pop_back();
        }
    }

    inline s32vec_t node_coord(vector_t v)
    {
        if (std::is_integral<coord_t>::value)
        {
            v = v / m_bucket_length;
        }
        else
        {
            v = v * m_rec_bucket_length;
        }

        return{
            static_cast<int32_t>(std::floor(v.x)),
            static_cast<int32_t>(std::floor(v.y))
        };
    }

    inline const leaf_vector_t& get_leaves_at_node(s32vec_t location)
    {
        return m_nodes[hash(location)].m_leaf_indices;
    }

    template<typename C>
    inline void foreach_leaf_in_aabb(aabb_t aabb, const C& c)
    {
        foreach_node_index_in_aabb(aabb,
            [&c, this](node_index_t n)
            {
                for (leaf_index_t leaf_index : this->m_nodes[n].m_leaf_indices)
                {
                    if (!c(leaf_index))
                    {
                        // stop
                        return false;
                    }
                }
                // continue
                return true;
            }
        );
    }

private:
    void insert_directly(node_index_t n, leaf_index_t l)
    {
        assert(m_leaves.size() > l);
        m_nodes[n].m_leaf_indices.push_back(l);
        m_leaves[l].m_containers.push_back(n);
    }

    inline node_index_t hash(s32vec_t v)
    {
        // unsigned modulo.
        return static_cast<node_index_t>(v.x + m_y_mult * v.y) % k_bucket_count;
    }

    template<typename C>
    inline void foreach_node_index_in_aabb(aabb_t aabb, C c)
    {
        s32vec_t start = node_coord(aabb.m_start);
        s32vec_t end = node_coord(aabb.m_end);
        for (int32_t x = start.x; x <= end.x; ++x)
        {
            for (int32_t y = start.y; y <= end.y; ++y)
            {
                node_index_t n = hash({ x, y });
                if (!c(n)) return;
            }
        }
    }

    void ensure_leaf(leaf_index_t l)
    {
        if (l >= m_leaves.size())
        {
            m_leaves.resize(l * 2 + 1);
        }
    }
};

}}
