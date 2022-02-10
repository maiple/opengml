#ifdef EMSCRIPTEN

#include "ogm/sys/util_sys.hpp"

namespace ogm
{
    
namespace
{
    #define RETURN_JS_STRING(jsString) \
        var lengthBytes = lengthBytesUTF8(jsString)+1; \
        var stringOnWasmHeap = _malloc(lengthBytes); \
        stringToUTF8(jsString, stringOnWasmHeap, lengthBytes); \
        return stringOnWasmHeap;
    
     EM_JS(void, _emjs_browser_open_url_, (url, target, opts), {
         window.open(url, target, opts);
     });
     
     EM_JS(char*, _emjs_browser_get_url_, (), {
        RETURN_JS_STRING(window.location.href);
     });
     
     EM_JS(char*, _emjs_browser_get_domain_, (), {
        RETURN_JS_STRING(window.location.hostname);
     });
}
    
void browser_open_url(const char* url, const char* target, const char* opts)
{
    _emjs_browser_open_url_(
        url ? url : "",
        target ? target : "",
        opts ? opts : ""
    );
}

#define RETURN_JS_STRING_CPP(src) \
    char* s = src; \
    std::string s2 = s; \
    free(s); \
    return s2;

std::string browser_get_url()
{
    RETURN_JS_STRING_CPP(_emjs_browser_get_url_());
}

std::string browser_get_domain()
{
    RETURN_JS_STRING_CPP(_emjs_browser_get_domain_());
}

}

#endif