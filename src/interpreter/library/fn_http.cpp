#include "libpre.h"
    #include "fn_http.h"
    #include "fn_string.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/ds/Map.hpp"

#include <string>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::http_get(VO out, V url)
{
    out = static_cast<real_t>(
        frame.m_http.get(staticExecutor.m_self->m_data.m_id, url.castCoerce<std::string>())
    );
}

void ogm::interpreter::fn::http_post_string(VO out, V url, V string)
{
    out = static_cast<real_t>(
        frame.m_http.post(
            staticExecutor.m_self->m_data.m_id,
            url.castCoerce<std::string>(),
            string.castCoerce<std::string>()
        )
    );
}

void ogm::interpreter::fn::http_request(VO out, V url, V method, V vheaders, V body)
{
    const char* data = nullptr;
    size_t datalen = 0;
    std::string bodystore;
    
    std::map<std::string, std::string> headers;
    
    if (body.is_numeric())
    {
        int32_t index = body.castCoerce<int32_t>();
        if (frame.m_buffers.buffer_exists(index))
        {
            Buffer& buffer = frame.m_buffers.get_buffer(index);
            datalen = buffer.size();
            
            // [sic]: body is only sent if buffer.tell() > 0
            if (datalen > 0 && buffer.tell() > 0)
            {
                data = reinterpret_cast<char*>(buffer.get_data());
            }
            else
            {
                datalen = 0;
            }
        }
    }
    else if (body.is_string())
    {
        bodystore = body.castCoerce<std::string>();
        data = bodystore.c_str();
        datalen = body.castCoerce<std::string>().length();
        if (datalen == 0) data = nullptr;
    }
    
    if (vheaders.is_numeric())
    {
        int32_t index = vheaders.castCoerce<int32_t>();
        if (frame.m_ds_map.ds_exists(index))
        {
            DSMap& map = frame.m_ds_map.ds_get(index);
            for (auto& [key, value] : map.m_data)
            {
                Variable keystr;
                fn::string(keystr, key);
                Variable valuestr;
                fn::string(valuestr, value.v);
                
                headers.emplace(
                    keystr.castCoerce<std::string>(),
                    valuestr.castCoerce<std::string>()
                );
                
                keystr.cleanup();
                valuestr.cleanup();
            }
        }
    }
    
    out = static_cast<real_t>(
        frame.m_http.request(
            staticExecutor.m_self->m_data.m_id,
            method.castCoerce<std::string>(),
            url.castCoerce<std::string>(),
            data,
            datalen,
            &headers
        )
    );
}