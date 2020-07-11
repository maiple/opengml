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

template<bool min>
Variable DSPriorityQueue::remove()
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
        Variable rv = std::move(chain.second.front());
        chain.second.pop_front();
        
        // delete empty chain.
        if (chain.second.empty())
        {
            if (min)
            {
                m_data.front().first.cleanup();
                m_data.pop_front();
            }
            else
            {
                m_data.back().first.cleanup();
                m_data.pop_back();
            }
        }
        
        return std::move(rv);
    }
}

template<bool min>
const Variable& DSPriorityQueue::peek() const
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
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        out = 0.0;
        return;
    }
    
    DSPriorityQueue& ds = dspm.ds_get(index);
    
    out = static_cast<real_t>(ds.m_data.empty());
}
    
void fn::ds_priority_add(VO out, V vindex, V vval, V priority)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        out = false;
        return;
    }
    
    Variable val;
    val.copy(vval);
    val.make_root();
    
    DSPriorityQueue& ds = dspm.ds_get(index);
    
    ds.emplace(priority, std::move(val));
}

void fn::ds_priority_delete_min(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        out = false;
        return;
    }
    DSPriorityQueue& ds = dspm.ds_get(index);
    if (ds.empty()) return;
    out = ds.remove<true>();    
}

void fn::ds_priority_delete_max(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        out = false;
        return;
    }
    DSPriorityQueue& ds = dspm.ds_get(index);
    if (ds.empty()) return;
    out = ds.remove<false>();    
}

void ogm::interpreter::fn::ds_priority_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dspm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent map datastructure.");
    }
    
    DSPriorityQueue& q = dspm.ds_get(index);
    for (auto& iter : q.m_data)
    {
        iter.first.cleanup();
        for (Variable& v : iter.second)
        {
            v.cleanup();
        }
    }
    dspm.ds_delete(index);
}

}