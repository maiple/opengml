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

        std::vector<GCNode*> erased;

        // erase and clean up all unmarked nodes.
        const auto erase_begin = std::remove_if(
            m_nodes.begin(),
            m_nodes.end(),
            [&erased](GCNode* node) -> bool
            {
                if (node->sweep())
                {
                    node->cleanup();
                    // delete later
                    erased.push_back(node);
                    return true;
                }
                return false;
            }
        );
        
        m_nodes.erase(erase_begin, m_nodes.end());
        
        // number of nodes removed.
        const size_t count = erased.size();
        
        // call delete function
        for (GCNode* node : erased)
        {
            node->_delete();
            delete node;
            node = nullptr;
        }
        
        #ifndef NDEBUG
        for (GCNode* node : m_nodes)
        {
            ogm_assert(node);
            VALGRIND_CHECK_INITIALIZED(node->m_root);
        }
        #endif

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
    
    std::string GarbageCollector::graph() const
    {
        std::stringstream ss;
        
        // enumerate nodes
        std::map<const GCNode*, size_t> node_index;
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            node_index[m_nodes.at(i)] = i;
        }
        
        // TODO: topological sort.
        
        // print each node.
        bool first = true;
        for (const GCNode* node : m_nodes)
        {
            if (!first) ss << "\n";
            first = false;
            ss << "Node " << node_index[node];
            if (node->m_root)
            {
                ss << " (root)";
            }
            ss << "\n";
            for (const GCNode::GCReference& ref : node->m_references)
            {
                ss << "  -> ";
                ss << node_index[ref.m_node];
                if (ref.m_count != 1)
                {
                    ss << " (x" << ref.m_count << ")";
                }
                ss << "\n";
            }
        }
        return ss.str();
    }
}
