#include "libpre.h"
    #include "fn_ds.h"
#include "libpost.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/ds/List.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define dspm staticExecutor.m_frame.m_ds_priority

namespace ogm::interpreter {
    
#define DS_PQ_ACCESS(ds, vindex) \
ds_index_t index = vindex.castCoerce<ds_index_t>(); \
if (!dspm.ds_exists(index)) \
{ \
    throw MiscError("Attempted to access non-existent priority queue datastructure."); \
} \
DSPriorityQueue& ds = dspm.ds_get(index);

template<bool min>
SafeVariable DSPriorityQueue::remove()
{
    if (m_data.empty())
    {
        throw MiscError("Attempted to remove value from empty PriorityQueue.");
    }
    else
    {
        chained_priority_elt& chain = (min)
            ? m_data.front()
            : m_data.back();
        if (chain.second.empty())
        {
            throw MiscError("Empty chain in PriorityQueue.");
        }
        
        // take first-added element from chain.
        SafeVariable rv = std::move(chain.second.front());
        chain.second.pop_front();
        --m_size;
        
        // delete empty chain.
        if (chain.second.empty())
        {
            if (min)
            {
                m_data.pop_front();
            }
            else
            {
                m_data.pop_back();
            }
        }
        
        return rv;
    }
}

template<bool min>
const SafeVariable& DSPriorityQueue::peek() const
{
    if (m_data.empty())
    {
        throw MiscError("Attempted to peek in empty PriorityQueue.");
    }
    else
    {
        const chained_priority_elt& chain = (min)
            ? m_data.front()
            : m_data.back();
        if (chain.second.empty())
        {
            throw MiscError("Empty chain in PriorityQueue.");
        }
        
        // take first-added element from chain.
        return chain.second.front();
    }
}
    
void ogm::interpreter::fn::ds_priority_create(VO out)
{
    out = static_cast<real_t>(dspm.ds_new());
}

void ogm::interpreter::fn::ds_priority_empty(VO out,  V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    
    out = static_cast<real_t>(ds.m_data.empty());
}

void ogm::interpreter::fn::ds_priority_size(VO out,  V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    
    out = static_cast<real_t>(ds.m_size);
}
    
void fn::ds_priority_add(VO out, V vindex, V vval, V priority)
{
    DS_PQ_ACCESS(ds, vindex)
    
    Variable val;
    val.copy(vval);
    val.make_root();
    
    ds.emplace(priority, std::move(val));
}

void fn::ds_priority_delete_min(VO out, V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    if (ds.empty()) return;
    out = std::move(ds.remove<true>());
}

void fn::ds_priority_delete_max(VO out, V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    if (ds.empty()) return;
    out = std::move(ds.remove<false>());
}

void fn::ds_priority_find_min(VO out, V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    if (ds.empty()) return;
    out.copy(ds.peek<true>());
}

void fn::ds_priority_find_max(VO out, V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    if (ds.empty()) return;
    out.copy(ds.peek<false>());
}

void fn::ds_priority_clear(VO out, V vindex)
{
    DS_PQ_ACCESS(ds, vindex)
    if (ds.empty()) return;
    ds.m_data.clear();
    ds.m_size = 0;
}

void ogm::interpreter::fn::ds_priority_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent priority queue datastructure.");
    }
    
    dspm.ds_delete(index);
}

}