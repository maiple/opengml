#pragma once

#include "Async.hpp"

#include "ogm/common/types.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Buffer.hpp"

#include <memory>
#include <map>
#include <cstdlib>

namespace ogm::interpreter {

typedef int32_t http_id_t;

// event to notify the listener about
// new connection, data received, etc.
class HTTPEvent : public AsyncEvent
{
public:
    HTTPEvent(
        async_listener_id_t listener,
        http_id_t id
    )
        : AsyncEvent(listener, asset::DynamicSubEvent::OTHER_ASYNC_HTTP)
        , m_id(id)
    { }
    
    void produce_info(ds_index_t) override;

    std::stringstream m_buffer;
    std::string m_post_body;
    http_id_t m_id;
    std::string m_url;
    int32_t m_status;
    char* m_error = nullptr;
    long m_http_status;
    long socket = -1;
    double m_progress = 0;
    double m_content_length = -1;
    
    ~HTTPEvent()
    {
        if (m_error) delete[] m_error;
    }
};

enum class HTTPRequestType
{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE
};

class HTTPManager
{
public:
    http_id_t get(async_listener_id_t, const std::string& url);
    http_id_t post(async_listener_id_t, const std::string& url, const std::string& body);
    http_id_t request(async_listener_id_t, HTTPRequestType, const std::string& url, const char* body=nullptr);
    
    // receives queued data updates
    void receive(std::vector<std::unique_ptr<AsyncEvent>>& out);
    
private:
    bool init();
};

}
