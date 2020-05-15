#include "libpre.h"
    #include "fn_http.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

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