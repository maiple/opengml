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
    
    for (int32_t index : m_map_indices)
    {
        Variable dummy;
        fn::ds_map_destroy(dummy, index);
        dummy.cleanup();
    }
    
    m_buffer_indices.clear();
    m_map_indices.clear();
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

void AsyncEvent::produce_map(ds_index_t map_index, const std::string& name, const std::map<std::string, std::string>& map)
{
    Variable dummy;
    
    // create map
    // (TODO: read-only?)
    int32_t ds_index = frame.m_ds_map.ds_new();
    m_map_indices.push_back(ds_index);
    
    // populate map
    for (const auto& [key, value] : map)
    {
        Variable vkey{ key };
        Variable vval{ value };
        ogm::interpreter::fn::ds_map_replace(dummy, ds_index, vkey, vval);
        vkey.cleanup();
        vval.cleanup();
        dummy.cleanup();
    }
    
    // add map to output
    Variable key{ name };
    ogm::interpreter::fn::ds_map_replace(dummy, map_index, key, ds_index);
    key.cleanup();
}

}