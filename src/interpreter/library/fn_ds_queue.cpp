#include "libpre.h"
    #include "fn_ds.h"
#include "libpost.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/ds/Queue.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define dsqm staticExecutor.m_frame.m_ds_queue

namespace ogm::interpreter {
    
#define DS_QUEUE_ACCESS(ds, vindex) \
ds_index_t index = vindex.castCoerce<ds_index_t>(); \
if (!dsqm.ds_exists(index)) \
{ \
    throw MiscError("Attempted to access non-existent queue datastructure."); \
} \
DSQueue& ds = dsqm.ds_get(index);
   
void ogm::interpreter::fn::ds_queue_create(VO out)
{
    out = static_cast<real_t>(dsqm.ds_new());
}

void ogm::interpreter::fn::ds_queue_clear(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    ds.m_data.clear();
}

void ogm::interpreter::fn::ds_queue_copy(VO out, V vindex)
{
    size_t ds2i = dsqm.ds_new();
    out = static_cast<real_t>(ds2i);
    DS_QUEUE_ACCESS(ds, vindex);
    DSQueue& ds2 = dsqm.ds_get(ds2i);
    
    for (const SafeVariable& v : ds.m_data)
    {
        ds2.m_data.emplace_back();
        ds2.m_data.back().copy(v);
    }
}

void ogm::interpreter::fn::ds_queue_empty(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    out = static_cast<real_t>(ds.m_data.empty());
}

void ogm::interpreter::fn::ds_queue_size(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    out = static_cast<real_t>(ds.m_data.size());
}

void ogm::interpreter::fn::ds_queue_enqueue(VO out, V vindex, V value)
{
    DS_QUEUE_ACCESS(ds, vindex);
    ds.m_data.emplace_back();
    ds.m_data.back().copy(value);
}

void ogm::interpreter::fn::ds_queue_dequeue(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    out = std::move(ds.m_data.front());
    ds.m_data.pop_front();
}

void ogm::interpreter::fn::ds_queue_head(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    out.copy(ds.m_data.front());
}

void ogm::interpreter::fn::ds_queue_tail(VO out, V vindex)
{
    DS_QUEUE_ACCESS(ds, vindex);
    out.copy(ds.m_data.back());
}

void ogm::interpreter::fn::ds_queue_destroy(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();
    if (!dsqm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent queue datastructure.");
    }
    
    dsqm.ds_delete(index);
}

}