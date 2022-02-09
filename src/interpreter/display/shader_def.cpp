#include "ogm/common/util.hpp"
#include "shader_def.hpp"

#include <string>
#include <iostream>

namespace
{

#ifndef EMSCRIPTEN
// glsl 3.3
const char* g_shr_glsl_version = "#version 330 core";
#else
// webgl es 2.0 (uses shader language 1.0)
const char* g_shr_glsl_version = "#version 100";
#endif

// some syntax differs between glsl core and webgl
#ifndef EMSCRIPTEN
#define varying_in "in"
#define varying_out "out"
#define texture_fn "texture"
#define out_frag_colour "FragColor"
#else
#define varying_in "varying"
#define varying_out "varying"
#define texture_fn "texture2D"
#define out_frag_colour "gl_FragColor"
#endif

const char* g_shr_vpre_definitions = R"(
#define MIRROR_WIN32_LIGHTING_EQUATION
#define MATRIX_VIEW 0
#define MATRIX_PROJECTION 1
#define MATRIX_WORLD 2
#define MATRIX_WORLD_VIEW 3
#define MATRIX_WORLD_VIEW_PROJECTION 4
#define MATRICES_MAX 5
#define MAX_VS_LIGHTS 8
)";

const char* g_shr_vpre_in = R"(
attribute vec3 in_Position;
attribute vec4 in_Colour;
attribute vec2 in_TextureCoord;
attribute vec3 in_Normal;
)";

const char* g_shr_vpre_out = R"(
)" varying_out R"( vec4 v_vColour;
)" varying_out R"( vec2 v_vTexcoord;
)";

const char* g_shr_vpre_uniform = R"(
// Lighting
uniform bool gm_LightingEnabled;
uniform vec4 gm_AmbientColour;
uniform vec4 gm_Lights_Direction[MAX_VS_LIGHTS];
uniform vec4 gm_Lights_PosRange[MAX_VS_LIGHTS];
uniform vec4 gm_Lights_Colour[MAX_VS_LIGHTS];

// Distance Fog
uniform bool gm_VS_FogEnabled;
uniform float gm_FogStart;
uniform float gm_RcpFogRange;

// Projection matrices
uniform mat4 gm_Matrices[MATRICES_MAX];
)";

const char* g_shr_vpre_functions =  R"(
)";

const std::string g_shr_vpre =
    std::string() + g_shr_glsl_version + g_shr_vpre_definitions + g_shr_vpre_uniform + g_shr_vpre_functions;

const char* g_shr_fpre_in =  R"(
precision mediump float;
)" varying_in R"( vec4 v_vColour;
)" varying_in R"( vec2 v_vTexcoord;
)";

#ifndef EMSCRIPTEN
// glsl declares varying out colour.
const char* g_shr_fpre_out =  R"(
)" varying_out R"( vec4 FragColor;
)";
#else
// webgl just uses gl_FragColor; no need to declare.
const char* g_shr_fpre_out = "\n";
#endif

const char* g_shr_fpre_uniform =  R"(
uniform sampler2D gm_BaseTexture;

// Alpha testing
uniform bool gm_AlphaTestEnabled;
uniform float gm_AlphaRefValue;

// Distance Fog
uniform bool gm_PS_FogEnabled;
uniform vec4 gm_FogColour;
)";

const char* g_shr_fpre_functions =  R"(
void DoAlphaTest(vec4 col)
{
    if (gm_AlphaTestEnabled && col.a <= gm_AlphaRefValue) discard;
}
)";

const std::string g_shr_fpre =
    std::string() + g_shr_glsl_version + g_shr_fpre_out + g_shr_fpre_uniform + g_shr_fpre_functions;

const std::string g_default_vertex_shader_source_str =
g_shr_vpre + g_shr_vpre_in + g_shr_vpre_out + R"(

)" varying_out R"( float v_vFogVal;

void main()
{
    gl_Position = gm_Matrices[MATRIX_WORLD_VIEW_PROJECTION] * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);
    v_vColour = in_Colour;
    v_vTexcoord = in_TextureCoord;

    // fog
    if (gm_VS_FogEnabled)
    {
        float dist = (gm_Matrices[MATRIX_WORLD_VIEW_PROJECTION] * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0)).z;
        v_vFogVal = clamp(gm_RcpFogRange * (dist - gm_FogStart), 0.0, 1.0);
    }
    else
    {
        v_vFogVal = 0.0;
    }
}
)";

const std::string g_default_fragment_shader_source_str =
g_shr_fpre + g_shr_fpre_in + R"(

)" varying_in R"( float v_vFogVal;

void main()
{
    vec4 col = )" texture_fn R"((gm_BaseTexture, v_vTexcoord);
    DoAlphaTest( col );
    col *= v_vColour;
    if (gm_PS_FogEnabled)
    {
        col.rgb = col.rgb * (1.0 - v_vFogVal) + gm_FogColour.rgb * v_vFogVal;
    }
    )" out_frag_colour R"( = col;
}
)";

}

namespace ogm::interpreter
{
const char* k_vertex_shader_pre = g_shr_vpre.c_str();
const char* k_fragment_shader_pre = g_shr_fpre.c_str();

const char* k_default_vertex_shader_source = g_default_vertex_shader_source_str.c_str();
const char* k_default_geometry_shader_source = "";
const char* k_default_fragment_shader_source = g_default_fragment_shader_source_str.c_str();

namespace
{
    std::string fix_common(std::string s)
    {
        return s;
    }
}

std::string fix_vertex_shader_source(std::string vertex_source)
{
    if (vertex_source == "")
    {
        vertex_source = k_default_vertex_shader_source;
    }
    else
    {
        // prepend preamble
        vertex_source = k_vertex_shader_pre + vertex_source;
    }
    
    vertex_source = fix_common(vertex_source);
    
    return vertex_source;
}

std::string fix_fragment_shader_source(std::string fragment_source)
{
    if (fragment_source == "")
    {
        fragment_source = k_default_fragment_shader_source;
    }
    else
    {
        // prepend preamble
        fragment_source = k_fragment_shader_pre + fragment_source;
    }
    
    fragment_source = fix_common(fragment_source);
    
    return fragment_source;
}

}