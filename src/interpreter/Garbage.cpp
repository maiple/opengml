#include "ogm/interpreter/Garbage.hpp"

namespace ogm::interpreter
{
    size_t GarbageCollector::process()
    {
        // mark all root nodes and all nodes referenced by them.
        for (GCNode* node : m_nodes)
        {
            if (node->m_root)
            {
                node->mark();
            }
        }

        // erase and clean up all unmarked nodes.
        auto iter = std::remove_if(
            m_nodes.begin(),
            m_nodes.end(),
            [](GCNode* node) -> bool
            {
                if (node->sweep())
                {
                    delete node;
                    return true;
                }
                return false;
            }
        );

        // number of nodes removed.
        size_t count = m_nodes.end() - iter;

        // erase needs to be called after remove_if.
        m_nodes.erase(
            iter,
            m_nodes.end()
        );

        return count;
    }
    
    void GarbageCollector::integrity_check_begin()
    {
        m_integrity_check_map.clear();
    }
    
    void GarbageCollector::integrity_check_touch(GCNode* node)
    {
        auto iter = m_integrity_check_map.find(node);
        if (iter == m_integrity_check_map.end())
        {
            m_integrity_check_map[node] = 1;
        }
        else
        {
            iter->second++;
        }
    }
    
    void GarbageCollector::integrity_check_end()
    {
        std::map<GCNode*, int32_t> count;
        
        // internal reference count check
        for (GCNode* node : m_nodes)
        {
            for (GCNode::GCReference& neighbour : node->m_references)
            {
                auto iter = count.find(neighbour.m_node);
                if (iter == count.end())
                {
                    count[neighbour.m_node] = 0;
                }
                else
                {
                    ++iter->second;
                }
            }
        }
        
        // compare
        for (GCNode* node : m_nodes)
        {
            auto iter = m_integrity_check_map.find(node);
            if (iter != m_integrity_check_map.end())
            {
                if (iter->second > node->m_references.size())
                {
                    throw MiscError("Integrity check failed on node at address " + std::to_string(
                        reinterpret_cast<intptr_t>(static_cast<void*>(node))
                    ));
                }
            }
        }
        
        m_integrity_check_map.clear();
    }
}
