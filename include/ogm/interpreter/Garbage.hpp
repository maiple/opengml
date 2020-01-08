#pragma once

#include "ogm/common/util.hpp"

#include <vector>
#include <functional>
#include <memory>

namespace ogm::interpreter
{
class GarbageCollector;

// component of anything which should be reasoned about by the GC.
class GCNode
{
    friend GarbageCollector;

    typedef std::function<void()> cleanup_fn;

    cleanup_fn m_cleanup;
    bool m_marked = false;

public:
    // root node act as the origin points of the sweep phase,
    // so they will not be deleted.
    bool m_root = false;

    // OPTIMIZE: use small_set or something?
    std::vector<GCNode*> m_nodes;

public:
    void add_reference(GCNode* node)
    {
        if (std::find(m_nodes.begin(), m_nodes.end(), node) == m_nodes.end())
        {
            m_nodes.push_back(node);
        }
    }

    void remove_reference(const GCNode* node)
    {
        auto iter = std::find(m_nodes.begin(), m_nodes.end(), node);
        if (iter != m_nodes.end())
        {
            m_nodes.erase(iter);
        }
    }

private:
    GCNode(cleanup_fn&& cleanup)
        : m_cleanup(std::move(cleanup))
    {}

private:
    void mark()
    {
        if (!m_marked)
        {
            m_marked = true;
            for (GCNode* node : m_nodes)
            {
                node->mark();
            }
        }
    }

    bool sweep()
    {
        if (m_marked || m_root)
        {
            m_marked = false;
            return false;
        }
        else
        {
            cleanup();
            return true;
        }
    }

    void cleanup()
    {
        ogm_assert(m_cleanup != nullptr);
        m_cleanup();
        m_cleanup = nullptr;
    }
};

// garbage collector
class GarbageCollector
{
    std::vector<GCNode*> m_nodes;

public:
    // ownership: returned GCNode* is owned by this GarbageCollector.
    // caller should not delete the provided GCNode.
    //
    // furthermore, the provided cleanup function is expected to
    // delete the object that has a copy of the GCNode*, though
    // this is not strictly necessary.
    GCNode* construct_node(GCNode::cleanup_fn&& cleanup)
    {
        return m_nodes.emplace_back(new GCNode(std::move(cleanup)));
    }

    // cleans up all unreferenced nodes.
    // returns number of deletions that occurred.
    size_t process()
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
                return node->sweep();
            }
        );

        size_t count = m_nodes.end() - iter;

        m_nodes.erase(
            iter,
            m_nodes.end()
        );

        return count;
    }
};

#ifdef OGM_GARBAGE_COLLECTOR
// global garbage collector.
// defined in Frame.cpp
extern GarbageCollector g_gc;
#endif
}
