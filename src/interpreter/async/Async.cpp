#include "ogm/common/util.hpp"
#include "ogm/interpreter/async/Async.hpp"
#include "ogm/interpreter/Executor.hpp"


#include "interpreter/library/libpre.h"
    #include "interpreter/library/fn_ds.h"
#include "interpreter/library/libpost.h"

#define frame staticExecutor.m_frame

namespace ogm::interpreter {
    
void AsyncEvent::cleanup()
{
    for (int32_t index : m_buffer_indices)
    {
        frame.m_buffers.remove_existing_buffer(index);
    }
    
    m_buffer_indices.clear();
}

void AsyncEvent::produce_real(ds_index_t map_index, const std::string& name, real_t value)
{
    Variable key{ name };
    Variable dummy;
    ogm::interpreter::fn::ds_map_replace(dummy, map_index, key, value);
    key.cleanup();
}

void AsyncEvent::produce_string(ds_index_t map_index, const std::string& name, const std::string& value)
{
    Variable key{ name };
    Variable str{ value };
    Variable dummy;
    ogm::interpreter::fn::ds_map_replace(dummy, map_index, key, str);
    key.cleanup();
    str.cleanup();
}

void AsyncEvent::produce_buffer(ds_index_t map_index, const std::string& name, std::shared_ptr<Buffer> value)
{
    auto index = frame.m_buffers.add_existing_buffer(value.get());
    m_buffer_indices.push_back(index);
    
    Variable key{ name };
    Variable dummy;
    ogm::interpreter::fn::ds_map_replace(dummy, map_index, key, index);
    key.cleanup();
}

}