#include "libpre.h"
    #include "fn_crypto.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <md5.h>
#include <base64.h>

#include <string>
#include <cctype>
#include <cstdlib>
#include <chrono>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::base64_encode(VO out, V vs)
{
    std::string_view s = vs.string_view();
    out = ::base64_encode(s);
}

void ogm::interpreter::fn::base64_decode(VO out, V vs)
{
    std::string_view s = vs.string_view();
    out = ::base64_decode(s);
}

void ogm::interpreter::fn::md5_string_utf8(VO out, V s)
{
    out = md5(s.castCoerce<std::string>());
}

void ogm::interpreter::fn::md5_string_unicode(VO out, V s)
{
    // TODO
    out = "00000000000000000000000000000000";
}