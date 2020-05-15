#include "ogm/interpreter/async/HTTP.hpp"

#ifdef OGM_CURL
#include "curl/curl.h"
#endif

#include <string>
#include <map>
#include <vector>
#include <queue>

namespace ogm::interpreter {
    
void HTTPEvent::produce_info(ds_index_t map_index)
{
    produce_real(map_index, "id", m_id);
    produce_real(map_index, "status", m_status);
    produce_string(map_index, "url", m_url);
    produce_real(map_index, "http_status", m_http_status);
    produce_string(map_index, "result", m_buffer.str());
    
    if (m_status == 1)
    {
        // progress report
        produce_real(map_index, "sizeDownloaded", m_progress);
        produce_real(map_index, "contentLength", m_content_length);
    }
    
    if (!m_headers.empty())
    {
        std::map<std::string, std::string> headermap;
        for (const std::string& header : m_headers)
        {
            size_t split_index = header.find(":");
            if (split_index == std::string::npos) continue;
            
            std::string pre = header.substr(0, split_index);
            std::string post = header.substr(split_index + 1);
            trim(pre);
            trim(post);
            
            if (pre.length() && post.length())
            {
                headermap[pre] = post;
            }
        }
        
        produce_map(map_index, "response_headers", headermap);
    }
    else
    {
        produce_real(map_index, "response_headers", -1);
    }
}

#ifdef OGM_CURL
static CURLM* g_curl_multi_handle;

static bool g_active;
static bool g_proceed;

static std::map<CURL*, std::unique_ptr<HTTPEvent>> g_handle_acc;

static_assert(
    sizeof(curl_socket_t) <= sizeof(HTTPEvent::socket),
    "curl_socket_t must fit in HTTPEvent::socket."
);

size_t cb_socket(CURL* handle, curl_socket_t s, int what, void* userp, void* socketp)
{
    // this callback tells us what information we should be monitoring for on the
    // given socket to know when to next call curl_multi_socket_action().
    // However, in the current implementation, we call curl_multi_socket_action() once
    // per frame, so we ignore this callback.
    // TODO: In the future, this could be implemented to process http data more quickly...
    
    switch (what)
    {
    case CURL_POLL_IN:
        break;
    case CURL_POLL_OUT:
        break;
    case CURL_POLL_INOUT:
        break;
    case CURL_POLL_REMOVE:
        break;
    }
    
    auto iter = g_handle_acc.find(handle);
    if (iter != g_handle_acc.end())
    {
        iter->second->socket = s;
    }
    
    return 0;
}

int cb_timer(CURLM* multi, long timeout_ms, void* userp)
{
    // proceed with another update call immediately only if timeout is 0.
    g_proceed = timeout_ms == 0;
    
    // success
    return 0;
}

size_t cb_recv_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
    CURL* handle = static_cast<CURL*>(userp);
    auto iter = g_handle_acc.find(handle);
    if (iter == g_handle_acc.end())
    {
        // TODO: error?
        return size * nmemb;
    }
    
    std::stringstream& ss = iter->second->m_buffer;
    ss.write(
        (const char*)buffer,
        size * nmemb
    );
    return size * nmemb;
}

size_t cb_header(void* buffer, size_t size, size_t nmemb, void* userp)
{
    CURL* handle = static_cast<CURL*>(userp);
    auto iter = g_handle_acc.find(handle);
    if (iter == g_handle_acc.end())
    {
        // TODO: error?
        return size * nmemb;
    }
    
    iter->second->m_headers.emplace_back((char*)buffer, size * nmemb);
    return size * nmemb;
}

static std::queue<std::unique_ptr<HTTPEvent>> m_progress_events;

int cb_progress(void* userp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    // create an "in progress" async event
    CURL* handle = static_cast<CURL*>(userp);
    auto iter = g_handle_acc.find(handle);
    
    // TODO: only do this for file downloads.
    if (iter != g_handle_acc.end())
    {
        HTTPEvent* base = iter->second.get();
        HTTPEvent* event = new HTTPEvent(base->m_listener_id, base->m_id);
        event->m_url = base->m_url;
        event->m_status = 1; // "in progress" status
        event->m_progress = dlnow;
        event->m_content_length = dltotal;
        m_progress_events.emplace(event);
    }
    return 0;
}
#endif

bool HTTPManager::init()
{
    static bool is_init = false;
    if (is_init) return true;
    
    #ifdef OGM_CURL
    g_active = false;
    g_curl_multi_handle = curl_multi_init();
    if (!g_curl_multi_handle) return false;
    
    curl_multi_setopt(g_curl_multi_handle, CURLMOPT_SOCKETFUNCTION, cb_socket);
    curl_multi_setopt(g_curl_multi_handle, CURLMOPT_SOCKETDATA, nullptr);
    curl_multi_setopt(g_curl_multi_handle, CURLMOPT_TIMERFUNCTION, cb_socket);
    curl_multi_setopt(g_curl_multi_handle, CURLMOPT_TIMERDATA, nullptr);
    #endif
    
    is_init = true;
    return true;
}

http_id_t HTTPManager::request(async_listener_id_t id, const std::string& method, const std::string& url, const char* body, size_t bodylen, const std::map<std::string, std::string>* headers)
{
    static http_id_t _http_id = 0;
    http_id_t http_id = _http_id++;
    if (!init())
    {
        return -2;
    }
    
    #ifdef OGM_CURL
    // add handle to the list.
    HTTPEvent* info = new HTTPEvent(id, http_id);
    info->m_url = url;
    
    CURL* handle = curl_easy_init(); 
    
    if (!handle) return -1;
    
    // request url
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    if (method == "GET")
    {
        curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    }
    else if (method == "POST")
    {
        curl_easy_setopt(handle, CURLOPT_HTTPPOST, 1L);
    }
    else
    {
        curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, method.c_str());
    }
    
    if (body)
    {
        info->m_post_body = std::string(body, bodylen);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, bodylen);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, info->m_post_body.c_str());
    }
    
    // set headers
    if (headers)
    {
        struct curl_slist* chunk = nullptr;
        for (const auto& [key, value] : *headers)
        {
            std::string joined = key + ": " + value;
            chunk = curl_slist_append(chunk, joined.c_str());
        }
        
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, chunk);
    }
    
    // set callback
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, cb_recv_data);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, handle);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, cb_progress);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, handle);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, cb_header);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, handle);
    // curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L); // follow 3xx location responses?
    info->m_error = new char[CURL_ERROR_SIZE];
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, info->m_error);
    
    g_active = true;
    g_handle_acc.emplace(handle, info);
    CURLMcode result = curl_multi_add_handle(g_curl_multi_handle, handle);
    if (result != CURLM_OK)
    {
        std::cout << "Failed to add HTTP handle: " << curl_multi_strerror(result) << std::endl;
        return -1;
    }
    #endif
    
    return http_id;
}
    
http_id_t HTTPManager::get(async_listener_id_t id, const std::string& url)
{
    return request(id, "GET", url);
}

http_id_t HTTPManager::post(async_listener_id_t id, const std::string& url, const std::string& body)
{
    return request(id, "POST", url, body.c_str(), body.length());
}

void HTTPManager::receive(std::vector<std::unique_ptr<AsyncEvent>>& out)
{
    init();
    
    #ifdef OGM_CURL
    
    // add queued progress events
    while (!m_progress_events.empty())
    {
        out.emplace_back(std::move(m_progress_events.front()));
        m_progress_events.pop();
    }
    
    // completion events
    if (g_active)
    {
        int running = 1;
        
        // find the sockets to iterate over.
        std::vector<curl_socket_t> sockets;
        sockets.push_back(CURL_SOCKET_TIMEOUT);
        for (const auto& iter : g_handle_acc)
        {
            long s = iter.second->socket;
            if (s >= 0)
            {
                sockets.push_back(s);
            }
        }
        const int actions[3] = {0,  CURL_CSELECT_IN, CURL_CSELECT_OUT};
        
        // let curl process the requests
        // (this will invoke some of the above cb_ functions)
        g_proceed = true;
        while (g_proceed)
        {
            g_proceed = false;
            
            for (curl_socket_t socket : sockets)
            {
                for (int32_t action = 0; action < 3; ++ action)
                {
                    CURLMcode mcode = curl_multi_socket_action(
                        g_curl_multi_handle,
                        socket,
                        actions[action],
                        &running
                    );
                    
                    if (mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM)
                    {
                        std::cout << "HTTP request update error: " << curl_multi_strerror(mcode) << std::endl;
                    }
                }
            }
        }
        
        // all transfers complete?
        g_active = !!running;
        
        // find out what transfers are done (successful or not).
        int msgs_remaining;
        while (CURLMsg* info = curl_multi_info_read(g_curl_multi_handle, &msgs_remaining))
        {
            CURL* handle = info->easy_handle;
            if (info->msg == CURLMSG_DONE)
            {
                // event is done; let's add it to the output.
                auto iter = g_handle_acc.find(handle);
                if (iter != g_handle_acc.end())
                {
                    // populate a few remaining fields on the event
                    
                    // status
                    HTTPEvent* event = iter->second.get();
                    CURLcode result = info->data.result;
                    if (result == CURLE_OK)
                    {
                        event->m_status = 0;
                    }
                    else
                    {
                        event->m_status = -std::abs(static_cast<int32_t>(result));
                    }
                    
                    // http status
                    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &event->m_http_status);
                    
                    // move http event to out queue
                    out.emplace_back(iter->second.release());
                    g_handle_acc.erase(iter);
                }
                
                // curl needs to clean up too
                curl_multi_remove_handle(g_curl_multi_handle, handle);
                curl_easy_cleanup(handle);
            }
            
            // no more messages to process?
            // (should be redundent with info == nullptr)
            if (msgs_remaining == 0) break;
        }
    }
    #endif
}
    
}