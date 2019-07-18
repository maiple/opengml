#include <string>

namespace
{

const char* g_shr_glsl_version = "#version 330 core";

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
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec4 in_Colour;
layout (location = 2) in vec2 in_TextureCoord;
)";

const char* g_shr_vpre_out = R"(
out vec4 v_vColour;
out vec2 v_vTexcoord;
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

const std::string g_shr_vpre =
    std::string() + g_shr_glsl_version + g_shr_vpre_definitions + g_shr_vpre_in + g_shr_vpre_out + g_shr_vpre_uniform;

const char* g_shr_fpre_in =  R"(
in vec4 v_vColour;
in vec2 v_vTexcoord;
)";

const char* g_shr_fpre_out=  R"(
out vec4 FragColor;
)";

const char* g_shr_fpre_uniform =  R"(
uniform sampler2D gm_BaseTexture;

// Alpha testing
uniform bool gm_AlphaTestEnabled;
uniform float gm_AlphaRefValue;

// Distance Fog
uniform bool gm_PS_FogEnabled;
uniform vec4 gm_FogColour;
)";

const std::string g_shr_fpre =
    std::string() + g_shr_glsl_version + g_shr_fpre_in + g_shr_fpre_out + g_shr_fpre_uniform;

const std::string g_default_vertex_shader_source_str =
g_shr_vpre + R"(

out float v_vFogVal;

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
g_shr_fpre + R"(

in float v_vFogVal;

void main()
{
    vec4 col = texture(gm_BaseTexture, v_vTexcoord);
    if (gm_AlphaTestEnabled && col.a <= gm_AlphaRefValue) discard;
    col *= v_vColour;
    if (gm_PS_FogEnabled)
    {
        col.rgb = col.rgb * (1.0 - v_vFogVal) + gm_FogColour.rgb * v_vFogVal;
    }
    FragColor = col;
}
)";

}

namespace ogmi
{
    const char* k_default_vertex_shader_source = g_default_vertex_shader_source_str.c_str();
    const char* k_default_geometry_shader_source = "";
    const char* k_default_fragment_shader_source = g_default_fragment_shader_source_str.c_str();
}
