#pragma once

#include "ogm/common/types.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Buffer.hpp"
#include "ogm/interpreter/ds/DataStructureManager.hpp"
#include "ogm/asset/Event.hpp"

#include <map>
#include <string>
#include <memory>

namespace ogm::interpreter {

typedef id_t async_listener_id_t;

// these are queued up and distributed to object instances during execution.
class AsyncEvent
{
public:
    // the object that will receive this event.
    async_listener_id_t m_listener_id;
    
    // the async event that will be triggered.
    asset::DynamicSubEvent m_async_subevent;
    
    AsyncEvent(async_listener_id_t id, asset::DynamicSubEvent subevent)
        : m_listener_id(id)
        , m_async_subevent(subevent)
    { }
    
    // adds event info to the given ds map
    virtual void produce_info(ds_index_t)=0;
    
    // performs any resource deallocation needed after produce_info()
    virtual void cleanup();
    
protected:
    void produce_real(ds_index_t, const std::string& name, real_t value);
    void produce_string(ds_index_t, const std::string& name, const std::string& value);
    void produce_buffer(ds_index_t, const std::string& name, std::shared_ptr<Buffer> buffer);
    void produce_map(ds_index_t, const std::string& name, const std::map<std::string, std::string>& map);

private:
    // for cleanup
    std::vector<int32_t> m_buffer_indices;
    std::vector<int32_t> m_map_indices;
};
    
}