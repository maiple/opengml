#pragma once

#include "ogm/common/util.hpp"

#include <vector>
#include <functional>
#include <memory>
#include <map>

namespace ogm::interpreter
{
class GarbageCollector;

// component of anything which should be reasoned about by the GC.
class GCNode
{
    friend GarbageCollector;
    
    struct GCReference
    {
        GCNode* m_node;
        uint32_t m_count;
        GCReference()
            : m_node(nullptr)
            , m_count(0)
        { }
        GCReference(GCNode* node)
            : m_node(node)
            , m_count(1)
        { }
        GCReference(const GCReference&)=default;
        GCReference(GCReference&&)=default;
        GCReference& operator=(const GCReference&)=default;
        ~GCReference()=default;
        
        bool operator<(const GCReference& other) const { return m_node < other.m_node; }
        bool operator==(const GCReference& other) const { return m_node == other.m_node; }
    };

    typedef std::function<void()> cleanup_fn;

    cleanup_fn m_cleanup;
    cleanup_fn m_delete;
    void* m_underlying;
    bool m_marked = false;

public:
    // root node act as the origin points of the sweep phase,
    // so they will not be deleted.
    bool m_root = false;

    // OPTIMIZE: use small_set or something?
    // these are the nodes this node knows about.
    std::vector<GCReference> m_references;

public:
    void add_reference(GCNode* node)
    {
        if (!node) return;
        
        auto iter = std::find(m_references.begin(), m_references.end(), node);

        if (iter == m_references.end())
        {
            m_references.emplace_back(node);
        }
        else
        {
            ++iter->m_count;
        }
    }

    void remove_reference(GCNode* node)
    {
        auto iter = std::find(m_references.begin(), m_references.end(), node);
        if (iter != m_references.end())
        {
            if (--iter->m_count == 0)
            {
                m_references.erase(iter);
            }
        }
    }
    
    void* get()
    {
        return m_underlying;
    }

private:
    GCNode(cleanup_fn&& cleanup, cleanup_fn&& _delete, void* underlying)
        : m_cleanup(std::move(cleanup))
        , m_delete(std::move(_delete))
        , m_underlying(underlying)
    {}

private:
    void mark()
    {
        if (!m_marked)
        {
            m_marked = true;
            for (auto& ref : m_references)
            {
                ref.m_node->mark();
            }
        }
    }

    // returns true if the node should be deleted.
    // unmarks the node.
    bool sweep()
    {
        if (m_marked || m_root)
        {
            m_marked = false;
            return false;
        }
        else
        {
            return true;
        }
    }

    void cleanup()
    {
        ogm_assert(m_cleanup != nullptr);
        m_cleanup();
        m_cleanup = nullptr;
    }
    
    void _delete()
    {
        ogm_assert(m_delete != nullptr);
        m_delete();
        m_delete = nullptr;
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
    // furthermore, the provided delete function is expected to
    // delete the object that has a copy of the GCNode*, though
    // this is not strictly necessary.
    // 
    // additionally, the cleanup function is provided to allow
    // the data to unlink from any other GC nodes it has references
    // to before those nodes would be deleted.
    inline GCNode* construct_node(GCNode::cleanup_fn&& cleanup, GCNode::cleanup_fn&& _delete, void* underlying)
    {
        return m_nodes.emplace_back(
            new GCNode(std::move(cleanup), std::move(_delete), underlying)
        );
    }

    // cleans up all unreferenced nodes.
    // returns number of deletions that occurred.
    size_t process();

    // returns number of items known to the GC.
    size_t get_heap_count()
    {
        return m_nodes.size();
    }
    
    std::string graph() const;

    // GC integrity check routine.
    // compares reference count with internal,
    // asserts that internal model is a subset of the GC's.
    void integrity_check_begin();

    void integrity_check_touch(GCNode*);

    void integrity_check_end();

    ~GarbageCollector()
    {
        // we could simply iterate through all nodes and delete them,
        // but this way we can detect lifespan memory leaks.
        process();
    }

private:
    std::map<GCNode*, int32_t> m_integrity_check_map;
};

#ifdef OGM_GARBAGE_COLLECTOR
// global garbage collector.
// defined in Frame.cpp
extern GarbageCollector g_gc;
#endif
}
