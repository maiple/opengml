#include "libpre.h"
    #include "fn_os.h"
#include "libpost.h"

#include <locale>
#include <regex>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

struct locale_info_t
{
    std::string m_language;
    std::string m_region;
    std::string m_codeset;
    std::string m_modifier;
};

locale_info_t get_local_info()
{
    const std::string name = std::locale().name();
    #define TOKEN "([a-zA-Z0-9]*)"
    std::smatch match;
    if(std::regex_match(name, match, std::regex(
        "("  TOKEN "|)"
        "(_" TOKEN "|)"
        "(." TOKEN "|)"
        "(@" TOKEN "|)"
    )))
    {
        return {
            match[2],
            match[4],
            match[6],
            match[8]
        };
    }
    
    return {};
}

void ogm::interpreter::fn::os_get_language(VO out)
{
    out = get_local_info().m_language;
}

void ogm::interpreter::fn::os_get_region(VO out)
{
    out = get_local_info().m_region;
}

void ogm::interpreter::fn::os_is_paused(VO out)
{
    out = false;
}