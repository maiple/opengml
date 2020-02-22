#include <string>

namespace ogm { namespace interpreter
{
    extern const char* k_vertex_shader_pre;
    extern const char* k_fragment_shader_pre;
    
    extern const char* k_default_vertex_shader_source;
    extern const char* k_default_geometry_shader_source;
    extern const char* k_default_fragment_shader_source;
    
    // adjusts source to be compatible with opengl.
    std::string fix_fragment_shader_source(std::string);
    std::string fix_vertex_shader_source(std::string);
}}
