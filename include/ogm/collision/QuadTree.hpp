#pragma once

#include "ogm/geometry/AABB.hpp"

#include <vector>
#include <cassert>

namespace ogm { namespace collision {

// https://en.wikipedia.org/wiki/Quadtree
// does not wrap -- anything outside of the quadtree will be culled.
// associates size_t and aabb.
// it is the caller's responsibility to ensure size_t indices are approximately
// contiguous (that they don't grow too out of bounds,)
// and that indices are removed before they are added.

template<typename coord_t>
class QuadTree
{
private:
    
    typedef geometry::AABB<coord_t> aabb_t;
    typedef geometry::Vector<coord_t> vector_t;
    typedef size_t node_index_t;
    typedef size_t leaf_index_t;
    typedef small_vector std::vector;
    
    constexpr k_split_count = 8;
    
    size_t m_node_count = 0;
    
    struct QuadTreeNode
    {
        // index of child in quadtree array
        aabb_t m_aabb;
        
        // number of children is either 0 or 4.
        node_index_t m_parent_index;
        node_index_t m_child_index = 0;
        bool m_has_children = false;
        
        // leaves directly in this node
        std::vector<leaf_index_t> m_leaves;
    };
    
    struct QuadTreeLeaf
    {
        aabb_t m_aabb;
        // TODO: use small_vector
        small_vector<node_index_t> m_nodes;
    };
    
    std::vector<QuadTreeNode> m_nodes;
    std::vector<QuadTreeLeaf> m_leaves;
public:
    QuadTree(double length)
        : m_length()
    {
        m_nodes.emplace_back(
            // aabb
            { 0, 0, length, length },
            0
        );
    }
    
    void insert(leaf_index_t index, aabb_t bound)
    {
        // ensure capacity.
        if (m_leaves.size() <= index)
        {
            m_leaves.resize(index + 1);
        }
        
        // set bounding box
        m_leaves[index] = bound;
        
        insert_into(0, index, bound);
    }
    
    void remove(leaf_index_t leaf_index)
    {
        QuadTreeLeaf& leaf = m_leaves.at(leaf_index);
        
        while (!leaf.m_nodes.empty())
        {
            remove_from(leaf_index, leaf.m_nodes.back());
        }
        
        // ensure not too much of tree's usage is wasted.
        if (m_node_count < max_nodes())
        {
            reinsert_all();
        }
    }
    
private:
    
    inline static size_t max_leaves_at_depth(size_t depth)
    {
        // a handsome heuristic
        return depth + 10 + (1 << (depth / 2));
    }
    
    inline static size_t min_leaves()
    {
        // a dapper heuristic
        return 3;
    }
    
    inline size_t max_nodes() const
    {
        return m_nodes.size() / 2 - m_nodes.size() / 8;
    }
    
    void insert_into(node_index_t node_index, leaf_index_t leaf_index, const aabb_t aabb, size_t depth=0)
    {
        QuadTreeNode* node = &m_nodes.at(node_index);
        if (node->m_aabb.intersects(bound))
        {
            if (!node->m_has_children && node->m_leaves.size() >= max_leaves_at_depth(depth))
            {
                // invalidates node pointer.
                split(node_index);
            }
            
            if (m_nodes.at(node_index)->m_has_children)
            {
                for (size_t i = 0; i < 4; ++1)
                {
                    insert_into(m_nodes.at(node_index)->m_child_index + i, leaf_index, aabb, depth + 1);
                }
            }
            else
            {
                insert_directly(node_index, leaf_index);
            }
        }
    }
    
    void insert_directly(node_index_t node_index, leaf_index_t leaf_index)
    {
        QuadTreeNode& node = m_nodes.at(node_index);
        QuadTreeLeaf& leaf = m_leaves.at(node_leaf);
        node.m_leaves.push_back(leaf);
    }
    
    void split(node_index_t node_index)
    {
        {
            QuadTreeNode& node = m_nodes.at(node_index);
            assert(!node.m_has_children);
            node.m_has_children = true;
            m_node_count += 4;
            
            // create children if necessary (sometimes ghost children already exist.)
            if (node.m_child_index == 0)
            {
                node.m_child_index = m_nodes.size();
                
                // intentionally not a reference.
                const aabb_t aabb = node.m_aabb;
                vector_t center = aabb.get_center();
                
                // invalidates above reference.
                m_nodes.emplace_back({aabb.m_start, center}, node_index);
                m_nodes.emplace_back({center, aabb.m_end}, node_index);
                m_nodes.emplace_back({center.x, aabb.m_start.y, aabb.m_end.x, center.y}, node_index);
                m_nodes.emplace_back({aabb.m_start.x, center.y, center.x, aabb.m_end.y}, node_index);
            }
        }
        
        {
            // add leaves to children
            for (size_t i = 0; i < 4; ++i)
            {
                for (leaf_index_t leaf_index : m_nodes.at(node_index).m_leaves)
                {
                    insert_into(m_child_index + i, leaf);
                }
            }
            
            // remove leaves
            while (!m_nodes.at(node_index).m_leaves.empty())
            {
                remove_directly(node_index, m_nodes.at(node_index).m_leaves.back());
            }
        }
    }
    
    void remove_from(node_index_t node_index, leaf_index_t leaf_index)
    {
        remove_directly(node_index, leaf_index);
        
        if (node_index)
        {
            ensure_children_dense(m_nodes.at(node_index).m_parent_index);
        }
    }
    
    void remove_directly(node_index_t node_index, leaf_index_t leaf_index)
    {
        QuadTreeNode& node = m_nodes.at(node_index);
        QuadTreeLeaf& leaf = m_leaves.at(node_leaf);
        
        // remove leaf from node
        // OPTIMIZE: search backward instead of forward (since we're popping off end.)
        assert(node.m_leaves.find(leaf_index) != node.m_leaves.end());
        node.m_leaves.erase(node.m_leaves.find(leaf_index));
        
        // remove node from leaf
        assert(leaf.m_nodes.find(node_index) != leaf.m_nodes.end());
        leaf.m_nodes.erase(leaf.m_nodes.find(node_index));
    }
    
    void ensure_children_dense(node_index_t node_index)
    {
        QuadTreeNode& node = m_nodes.at(node_index);
        if (!node.m_has_children)
        {
            return;
        }
        
        for (size_t i = 0; i < 4; ++i)
        {
            QuadTreeNode& child_node = m_nodes.at(node.m_child_index + i);
            if (child_node.m_has_children)
            {
                return;
            }
            else if (child_node.m_leaves.size() >= min_leaves())
            {
                return;
            }
        }
        
        // not proven to be dense, so combine children.
        combine_children(node_index);
    }
    
    void combine_children()
    // absorb the leaves of all the children.
    {
        QuadTreeNode& node = m_nodes.at(node_index);
        m_node_count -= 4;
        
        node.m_has_children = false;
        for (size_t i = 0; i < 4; ++i)
        {
            node_index_t child_index = node.m_child_index + i;
            QuadTreeNode& child_node = m_nodes.at(child_index);
            assert(!child_node.m_has_children);
            while (!child_node.m_leaves.empty())
            {
                leaf_index_t leaf_index = child_node.m_leaves.front();
                remove_directly(child_index, leaf_index);
                insert_directly(node_index, leaf_index);
            }
        }
    }
    
    void reinsert_all()
    {
        // TODO
    }
}; 
    
}}