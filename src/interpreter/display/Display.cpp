#include "ogm/interpreter/display/Display.hpp"
#include "Share.hpp"

#ifdef GFX_AVAILABLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#ifndef EMSCRIPTEN
    #include <GL/glew.h>
    #include <SDL2/SDL.h>
    #ifdef GFX_TEXT_AVAILABLE
        #include <SDL2/SDL_ttf.h>
        extern unsigned char _binary_Default_Font_ttf[];
        extern unsigned int _binary_Default_Font_ttf_len;
    #endif
#else
    #define GL_GLEXT_PROTOTYPES 1
    #include <SDL.h>
    #include <SDL_image.h>
    #include <SDL_opengles2.h>
    
    #define glGenVertexArrays glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
#endif

#include <optional>
#include <string_view>
#include "shader_def.hpp"

using namespace ogm;

namespace
{
    // from https://learnopengl.com/In-Practice/Debugging
    GLenum glCheckError_(const char *file, int line)
    {
        GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
            std::string error;
            switch (errorCode)
            {
                case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            }
            std::cout << "GL Error | " << error << " | " << file;
            if (line >= 0)
            {
                 std::cout << ":" << line;
            }
            std::cout << std::endl;
        }
        return errorCode;
    }

    #ifndef NDEBUG
        #define glCheckErrorStr(str) glCheckError_(str, -1)
        #define glCheckError() glCheckError_(__FILE__, __LINE__)
    #else
        #define glCheckErrorStr(str) {}
        #define glCheckError() {}
    #endif
}

namespace ogm { namespace interpreter {

unsigned int g_gl_framebuffer = 0;

// colour mask
bool g_cm[4] = { true, true, true, true };

namespace
{
    struct colour4;

    struct colour3
    {
        float r, g, b;

        colour3(float r, float g, float b)
            : r(r), g(g), b(b)
        { }

        colour3(const colour3& other) = default;

        colour3(const colour4& other);
    };

    struct colour4
    {
        float r, g, b, a;

        colour4(float r, float g, float b, float a)
            : r(r), g(g), b(b), a(a)
        { }

        colour4(const colour4& other) = default;

        colour4(const colour3& other)
            : r(other.r)
            , g(other.g)
            , b(other.b)
            , a(1.f)
        { }
    };

    colour3::colour3(const colour4& other)
        : r(other.r)
        , g(other.g)
        , b(other.b)
    { }

    Display* g_active_display = nullptr;
    bool g_enable_view_projection = true;
    ogm::geometry::Vector<real_t> g_viewport_dimensions;
    bool g_flip_projection = false;
    uint32_t g_window_width=0;
    uint32_t g_window_height=0;
    SDL_Window* g_window;
    SDL_Renderer* g_renderer;
    SDL_GLContext g_context;
    TexturePage* g_target = nullptr;
    uint32_t g_square_vbo;
    uint32_t g_square_vao;
    uint32_t g_blank_texture;
    uint32_t g_circle_precision=32;
    bool g_blending_enabled = true;
    colour4 g_clear_colour{0, 0, 0, 1};
    colour4 g_draw_colour[4] = {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
    #ifdef GFX_TEXT_AVAILABLE
    TTF_Font* g_font = nullptr;
    #endif

    // default shader
    uint32_t g_shader_program;

    // user-defined shaders
    std::map<asset_index_t, uint32_t> g_shader_programs;

    bool init_sdl = false;
    #ifndef EMSCRIPTEN
    bool init_glew = false;
    #endif
    bool init_buffers = false;

    bool g_sdl_closing = false;

    colour3 bgrz_to_colour3(uint32_t bgr)
    {
        return
        {
            static_cast<float>((bgr & 0xff) / 255.0),
            static_cast<float>(((bgr & 0xff00) >> 8) / 255.0),
            static_cast<float>(((bgr & 0xff0000) >> 16) / 255.0)
        };
    }

    uint32_t colour3_to_bgrz(colour3 c)
    {
        uint32_t rz = 255 * c.r;
        if (rz > 255) rz = 255;
        uint32_t gz = 255 * c.g;
        if (gz > 255) gz = 255;
        uint32_t bz = 255 * c.b;
        if (bz > 255) bz = 255;

        return rz | (gz << 8) | (bz << 16);
    }

    inline void colour3_to_floats(float* dst, colour3 c)
    {
        dst[0] = c.r;
        dst[1] = c.g;
        dst[2] = c.b;
    }

    uint32_t colour4_to_bgraz(colour4 c)
    {
        uint32_t rz = 255 * c.r;
        if (rz > 255) rz = 255;
        uint32_t gz = 255 * c.g;
        if (gz > 255) gz = 255;
        uint32_t bz = 255 * c.b;
        if (bz > 255) bz = 255;
        uint32_t az = 255 * c.a;
        if (bz > 255) bz = 255;

        return az | (rz << 8) | (gz << 16) | (bz << 24);
    }

    colour4 bgraz_to_colour4(uint32_t bgr)
    {
        return
        {
            static_cast<float>(((bgr & 0xff00) >> 8) / 255.0),
            static_cast<float>(((bgr & 0xff0000) >> 16) / 255.0),
            static_cast<float>(((bgr & 0xff000000) >> 24) / 255.0),
            static_cast<float>(((bgr & 0xff)) / 255.0)
        };
    }

    inline void colour4_to_floats(float* dst, colour4 c)
    {
        if (g_blending_enabled)
        {
            dst[0] = c.r;
            dst[1] = c.g;
            dst[2] = c.b;
            dst[3] = c.a;
        }
        else
        {
            dst[0] = 1.0;
            dst[1] = 1.0;
            dst[2] = 1.0;
            dst[3] = 1.0;
        }
    }

    #define CONST(x, y) constexpr ogm_keycode_t x = y;
    #include "../library/fn_keycodes.h"

    ogm_keycode_t sdl_scancode_to_ogm(size_t scancode)
    {
        // A - Z
        if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        {
            return scancode + 65 - SDL_SCANCODE_A;
        }

        // 0-9
        if (scancode >= SDLK_0 && scancode <= SDLK_9)
        {
            return scancode + 48 - SDLK_0;
        }

        // Fn keys
        if (scancode >= SDLK_F1 && scancode <= SDLK_F12)
        {
            return scancode + 112 - SDLK_F1;
        }

        switch(scancode)
        {
        case SDL_SCANCODE_SPACE:
            return vk_space;
        case SDL_SCANCODE_PAGEUP:
            return vk_pageup;
        case SDL_SCANCODE_PAGEDOWN:
            return vk_pagedown;
        case SDL_SCANCODE_ESCAPE:
            return vk_escape;
        case SDL_SCANCODE_RETURN:
            return vk_enter;
        case SDL_SCANCODE_BACKSPACE:
            return vk_backspace;
        case SDL_SCANCODE_TAB:
            return vk_tab;
        case SDL_SCANCODE_HOME:
            return vk_home;
        case SDL_SCANCODE_END:
            return vk_end;
        case SDL_SCANCODE_RIGHT:
            return vk_right;
        case SDL_SCANCODE_LEFT:
            return vk_left;
        case SDL_SCANCODE_DOWN:
            return vk_down;
        case SDL_SCANCODE_UP:
            return vk_up;
        case SDL_SCANCODE_PAUSE:
            return vk_pause;
        case SDL_SCANCODE_LALT:
            return vk_lalt;
        case SDL_SCANCODE_RALT:
            return vk_ralt;
        case SDL_SCANCODE_LCTRL:
            return vk_lcontrol;
        case SDL_SCANCODE_RCTRL:
            return vk_rcontrol;
        case SDL_SCANCODE_LSHIFT:
            return vk_lshift;
        case SDL_SCANCODE_RSHIFT:
            return vk_rshift;
        case SDL_SCANCODE_SEMICOLON:
            return vk_semicolon;
        case SDL_SCANCODE_APOSTROPHE:
            return vk_quote;
        case SDL_SCANCODE_COMMA:
            return vk_comma;
        case SDL_SCANCODE_PERIOD:
            return vk_period;
        case SDL_SCANCODE_SLASH:
            return vk_slash;
        case SDL_SCANCODE_LEFTBRACKET:
            return vk_open_square_bracket;
        case SDL_SCANCODE_RIGHTBRACKET:
            return vk_close_square_bracket;
        default:
            return 0;
        }
    }

    size_t ogm_to_sdl_scancode(ogm_keycode_t keycode)
    {
        // A-Z
        if (keycode >= 65 && keycode < 65 + 26)
        {
            return keycode - 65 + SDL_SCANCODE_A;
        }

        // 0-9
        if (keycode >= 48 && keycode <= 48 + 9)
        {
            return keycode - 48 + SDLK_0;
        }

        // Fn keys
        if (keycode >= 112 && keycode < 112 + 12)
        {
            return keycode - 112 + SDLK_F1;
        }

        switch(keycode)
        {
        case vk_space:
            return SDL_SCANCODE_SPACE;
        case vk_pageup:
            return SDL_SCANCODE_PAGEUP;
        case vk_pagedown:
            return SDL_SCANCODE_PAGEDOWN;
        case vk_escape:
            return SDL_SCANCODE_ESCAPE;
        case vk_enter:
            return SDL_SCANCODE_RETURN;
        case vk_backspace:
            return SDL_SCANCODE_BACKSPACE;
        case vk_tab:
            return SDL_SCANCODE_TAB;
        case vk_home:
            return SDL_SCANCODE_HOME;
        case vk_end:
            return SDL_SCANCODE_END;
        case vk_right:
            return SDL_SCANCODE_RIGHT;
        case vk_left:
            return SDL_SCANCODE_LEFT;
        case vk_down:
            return SDL_SCANCODE_DOWN;
        case vk_up:
            return SDL_SCANCODE_UP;
        case vk_pause:
            return SDL_SCANCODE_PAUSE;
        case vk_lalt:
            return SDL_SCANCODE_LALT;
        case vk_ralt:
            return SDL_SCANCODE_RALT;
        case vk_lcontrol:
            return SDL_SCANCODE_LCTRL;
        case vk_rcontrol:
            return SDL_SCANCODE_RCTRL;
        case vk_lshift:
            return SDL_SCANCODE_LSHIFT;
        case vk_rshift:
            return SDL_SCANCODE_RSHIFT;
        case vk_semicolon:
            return SDL_SCANCODE_SEMICOLON;
        case vk_quote:
            return SDL_SCANCODE_APOSTROPHE;
        case vk_comma:
            return SDL_SCANCODE_COMMA;
        case vk_period:
            return SDL_SCANCODE_PERIOD;
        case vk_slash:
            return SDL_SCANCODE_SLASH;
        case vk_open_square_bracket:
            return SDL_SCANCODE_LEFTBRACKET;
        case vk_close_square_bracket:
            return SDL_SCANCODE_RIGHTBRACKET;
        default:
            return std::numeric_limits<size_t>::max();
        }
    }

    ogm_keycode_t sdl_to_ogm_mousecode(uint8_t mousekey)
    {
        switch (mousekey)
        {
        case SDL_BUTTON_LEFT:
            return mb_left;
        case SDL_BUTTON_RIGHT:
            return mb_right;
        case SDL_BUTTON_MIDDLE:
            return mb_middle;
        default:
            return vk_nokey;
        }
    }

    ogm_keycode_t sdl_to_ogm_keycode(int key)
    {
        // A-Z
        if (key >= SDLK_a && key <= SDLK_z)
        {
            return key - SDLK_a + 65;
        }

        // 0-9
        if (key >= SDLK_0 && key <= SDLK_9)
        {
            return key - SDLK_0 + 48;
        }

        // Fn keys
        if (key >= SDLK_F1 && key <= SDLK_F12)
        {
            return key + 112 - SDLK_F1;
        }

        switch (key)
        // TODO: finish this
        // keycodes: https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlkey.html
        {
        case SDLK_SPACE:
            return vk_space;
        case SDLK_PAGEUP:
            return vk_pageup;
        case SDLK_PAGEDOWN:
            return vk_pagedown;
        case SDLK_ESCAPE:
            return vk_escape;
        case SDLK_RETURN:
            return vk_enter;
        case SDLK_BACKSPACE:
            return vk_backspace;
        case SDLK_TAB:
            return vk_tab;
        case SDLK_HOME:
            return vk_home;
        case SDLK_END:
            return vk_end;
        case SDLK_RIGHT:
            return vk_right;
        case SDLK_LEFT:
            return vk_left;
        case SDLK_DOWN:
            return vk_down;
        case SDLK_UP:
            return vk_up;
        case SDLK_PAUSE:
            return vk_pause;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            return vk_shift;
        case SDLK_RCTRL:
        case SDLK_LCTRL:
            return vk_control;
        case SDLK_LALT:
        case SDLK_RALT:
            return vk_alt;
        case SDLK_SEMICOLON:
            return vk_semicolon;
        case SDLK_QUOTE:
            return vk_quote;
        case SDLK_COMMA:
            return vk_comma;
        case SDLK_KP_PERIOD:
        case SDLK_PERIOD:
            return vk_period;
        case SDLK_KP_DIVIDE:
        case SDLK_SLASH:
            return vk_slash;
        case SDLK_LEFTBRACKET:
            return vk_open_square_bracket;
        case SDLK_RIGHTBRACKET:
            return vk_close_square_bracket;
        default:
            return vk_nokey;
        }
    }

    void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error (CB): %s\n", description);
    }

    constexpr size_t k_keycode_max = 256;

    // key is currently down
    volatile bool g_key_down[k_keycode_max];

    // key has been pressed down since last clear
    volatile bool g_key_pressed[k_keycode_max];

    // key has been released since last clear
    volatile bool g_key_released[k_keycode_max];

    // view matrices
    const int MATRIX_VIEW = 0;
    const int MATRIX_PROJECTION = 1;
    const int MATRIX_MODEL = 2;
    const int MATRIX_MODEL_VIEW = 3;
    const int MATRIX_MODEL_VIEW_PROJECTION = 4;
    const int MATRIX_MAX = 5;
    glm::mat4 g_matrices[MATRIX_MAX];

    glm::mat4 g_pre_model;

    // inverse of matrices
    // as the use of these are limited, some are not used.
    glm::mat4 g_imatrices[MATRIX_MAX];

    glm::mat4 g_matrix_identity(1.0f);
    glm::mat4 g_matrix_surface_view(1.0f);
    glm::mat4 g_matrix_surface_model_view(1.0f);

    const int ATTR_LOC_POSITION = 0;
    const int ATTR_LOC_COLOUR = 1;
    const int ATTR_LOC_TEXCOORD = 2;
    const int ATTR_LOC_NORMAL = 3;

    // sets the common attribute locations for the given program.
    void shader_bind_attribute_locations(uint32_t program)
    {
        glBindAttribLocation(program, ATTR_LOC_POSITION, "in_Position");
        glBindAttribLocation(program, ATTR_LOC_COLOUR, "in_Colour");
        glBindAttribLocation(program, ATTR_LOC_TEXCOORD, "in_TextureCoord");
        glBindAttribLocation(program, ATTR_LOC_NORMAL, "in_Normal");
    }

    // returns true on failure.
    // combines source with default source header.
    bool compile_shader(std::string vertex_source, std::string fragment_source, uint32_t& out_shader)
    {
        int success;

        // adjust source

        vertex_source = fix_vertex_shader_source(vertex_source);
        fragment_source = fix_fragment_shader_source(fragment_source);

        // compile

        char infoLog[512];
        uint32_t vertex_shader, fragment_shader;
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertex_source_c = vertex_source.c_str();
        glShaderSource(vertex_shader, 1, &vertex_source_c, nullptr);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cout << vertex_source << std::endl;
            glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            return true;
        }

        const char* fragment_source_c = fragment_source.c_str();
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_source_c, nullptr);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            std::cout << fragment_source << std::endl;
            glGetShaderInfoLog(fragment_shader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            return true;
        }

        // set shader program
        out_shader = glCreateProgram();
        glAttachShader(out_shader, vertex_shader);
        glAttachShader(out_shader, fragment_shader);
        shader_bind_attribute_locations(out_shader);
        glLinkProgram(out_shader);
        glGetProgramiv(out_shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(out_shader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
            return true;
        }

        // clean up shaders
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // success.
        return false;
    }

    // fills the given pointer with 3 floats
    inline void floats3(float* dst, coord_t x=0, coord_t y=0, coord_t z=0)
    {
        dst[0] = x;
        dst[1] = y;
        dst[2] = z;
    }

    // fills the given pointer with 3 floats multiplied by pre-model-matrix.
    inline void floats3pm(float* dst, coord_t x=0, coord_t y=0, coord_t z=0)
    {
        glm::vec4 v{ x, y, z, 1 };
        v = g_pre_model * v;
        dst[0] = v.x;
        dst[1] = v.y;
        dst[2] = v.z;
    }

    struct VertexFormat
    {
        bool m_used;
        bool m_done;

        std::vector<VertexFormatAttribute> m_attributes;
        size_t m_size;
        uint32_t m_vao;
    };

    struct VertexBuffer
    {
        bool m_used;
        enum
        {
            clean,
            dirty,
            frozen
        } m_state;
        std::vector<char> m_data;
        size_t m_size;
        uint32_t m_vbo;
        uint32_t m_format = std::numeric_limits<uint32_t>::max();
    };

    struct Model
    {
        bool m_used;

        // pair: vertex buffer, glenum
        std::vector<std::pair<uint32_t, uint32_t>> m_buffers;

        // vertex format (just a copy of the standard.)
        uint32_t m_format;
    };

    std::vector<VertexFormat> g_vertex_formats;
    std::vector<VertexBuffer> g_vertex_buffers;
    std::vector<Model> g_models;

    const int32_t k_transform_stack_size = 16; // TODO: confirm
    std::vector<glm::mat4> g_transform_stack;
}

// number of floats per vertex (in default format)
// 3 xyz coordinates, 4 colour coordinates, 2 texture coordinates
const size_t k_vertex_data_size = (3 + 4 + 2);

inline size_t VertexFormatAttribute::get_size() const
{
    switch (m_type)
    {
    case F1:
        return sizeof(float);
    case F2:
        return sizeof(float) * 2;
    case F3:
        return sizeof(float) * 3;
    case F4:
    case RGBA:
        return sizeof(float) * 4;
    case U4:
        return sizeof(unsigned char) * 4;
    default:
        throw MiscError("Unknown data type");
    }
}

inline size_t VertexFormatAttribute::get_component_count() const
{
    switch (m_type)
    {
    case F1:
        return 1;
    case F2:
        return 2;
    case F3:
        return 3;
    case F4:
    case RGBA:
        return 4;
    case U4:
        return 4;
    default:
        throw MiscError("Unknown data type");
    }
}

inline uint32_t VertexFormatAttribute::get_location() const
{
    switch (m_dest)
    {
    case POSITION:
        return ATTR_LOC_POSITION;
    case COLOUR:
        return ATTR_LOC_COLOUR;
    case TEXCOORD:
        return ATTR_LOC_TEXCOORD;
    case NORMAL:
        return ATTR_LOC_NORMAL;
    default:
        throw LanguageFeatureNotImplementedError("No static location set yet for attribute.");
    }
}

bool Display::start(uint32_t width, uint32_t height, const char* caption)
{
    if (g_active_display != nullptr)
    {
        throw MiscError("Multiple displays not supported");
    }

    if (!init_sdl)
    {
        if (
            SDL_Init(
                SDL_INIT_VIDEO | SDL_INIT_JOYSTICK
            ) != 0
        )
        {
            printf("Unable to initialize SDL: %s\n", SDL_GetError());
            return false;
        }
        init_sdl = true;

        #ifdef GFX_TEXT_AVAILABLE
        if (TTF_Init())
        {
            printf("Unable to initialize SDL_ttf: %s\n", TTF_GetError());
            return false;
        }
        #endif
    }

    #ifdef EMSCRIPTEN
    SDL_CreateWindowAndRenderer(width, height, 0, &g_window, &g_renderer);
    #else
    g_window = SDL_CreateWindow(
        caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height, SDL_WINDOW_OPENGL);
    #endif

    if (!g_window)
    {
        printf("Unable to create SDL window.\n");
        return false;
    }

    #ifdef EMSCRIPTEN
    if (!g_renderer)
    {
        // TODO: do we care about the renderer?
        printf("Unable to create SDL renderer.\n");
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    #else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    g_context = SDL_GL_CreateContext(g_window);
    #endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetSwapInterval( 1 );

    #ifndef EMSCRIPTEN

    // init GLEW
    if (!init_glew)
    {
        glewExperimental = GL_TRUE;
        GLenum result = glewInit();
        if (result != GLEW_OK)
        {
            std::cerr << "Error (glew): " << glewGetErrorString(result) << std::endl;
            std::cerr << "could not initialize glew.\n";
            return false;
        }

        init_glew = true;

        if (!GLEW_VERSION_3_0)
        {
            std::cerr << "Error (glew): OpenGL 3.0 not supported.\n";
            return false;
        }
    }
    #endif

    glCheckErrorStr("graphics module initialization.");

    #ifdef GFX_TEXT_AVAILABLE
    // TODO: use Arimo https://www.fontsquirrel.com/fonts/arimo
    // (it is metrically compatible with the default font.)
    g_font = TTF_OpenFontRW(
        SDL_RWFromConstMem(_binary_Default_Font_ttf, _binary_Default_Font_ttf_len), 1,
        12);
    if (!g_font)
    {
        std::cerr << "Error (ttf): could not open font file.\n";
        std::cerr << TTF_GetError();
        return false;
    }
    #endif

    g_active_display = this;
    g_window_width = width;
    g_window_height = height;
    g_viewport_dimensions.x = width;
    g_viewport_dimensions.y = height;
    g_gl_framebuffer = 0;
    set_matrix_projection();
    begin_render();

    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // compile shaders
    if (compile_shader("", "", g_shader_program))
    {
        return false;
    }

    glUseProgram(g_shader_program);

    // set up vertex format
    {
        glGenVertexArrays(1, &g_square_vao);
        glBindVertexArray(g_square_vao);

        // TODO: there's no need to generate a vbo here.
        glGenBuffers(1, &g_square_vbo);

        glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);

        // position
        glVertexAttribPointer(ATTR_LOC_POSITION, 3, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)0);
        glEnableVertexAttribArray(ATTR_LOC_POSITION);

        // colour
        glVertexAttribPointer(ATTR_LOC_COLOUR, 4, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_COLOUR);

        // texture coordinate
        glVertexAttribPointer(ATTR_LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_TEXCOORD);
        init_buffers = true;
    }

    // set uniforms
    {
        // initialize matrices to identity
        for (size_t i = 0; i <= MATRIX_MODEL; ++i)
        {
            g_matrices[i] = glm::mat4(1);
            g_imatrices[i] = glm::mat4(1);
        }

        update_camera_matrices();
    }

    // reset pre-model matrix.
    set_matrix_pre_model();

    // enable alpha blending
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );

    // generate the basic "blank" texture
    blank_image();

    // disable distance fog
    set_fog(false);

    glCheckErrorStr("ogm graphics initialization.");

    return true;
}

Display::~Display()
{

    std::cout << "~Display()\n";

    if (init_buffers)
    {
        glDeleteVertexArrays(1, &g_square_vao);
        glDeleteBuffers(1, &g_square_vbo);

        init_buffers = false;
    }

    #ifdef GFX_TEXT_AVAILABLE
    TTF_CloseFont(g_font);
    TTF_Quit();
    #endif

    SDL_Quit();

    g_active_display = nullptr;
}

// loads single white pixel texture
void Display::blank_image()
{
    unsigned char data[3] = {0xff, 0xff, 0xff};
    glGenTextures(1, &g_blank_texture);
    glBindTexture(GL_TEXTURE_2D, g_blank_texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0, // mipmap level
        GL_RGBA, // texture format
        1, 1, // dimensions
        0,
        GL_RGB, // source format
        GL_UNSIGNED_BYTE, // source format
        data // image data
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Display::render_vertices(float* vertices, size_t count, uint32_t texture, uint32_t render_glenum)
{
    glBindVertexArray(g_square_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * k_vertex_data_size, vertices, GL_STREAM_DRAW);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(render_glenum, 0, count);
}

void Display::draw_image_tiled(TextureView* texture, bool tiled_x, bool tiled_y, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2)
{
    double width = x2 - x1;
    double height = y2 - y1;
    if (texture->m_tpage->cache())
    {
        // construct a mesh of sufficient size to cover the view.

        // determine extent of view
        int32_t offset_minx;
        int32_t offset_maxx;
        int32_t offset_miny;
        int32_t offset_maxy;
        {
            glm::vec4 p[4] = {
                { 1, 1, 0, 1 },
                { -1, -1, 0, 1 },
                { 1, 1, 0, 1 },
                { -1, 1, 0, 1 }
            };
            for (size_t i = 0; i < 4; ++i)
            {
                p[i] = g_imatrices[MATRIX_VIEW] * p[i];
            }

            if (tiled_x)
            {
                coord_t xmin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
                    [](glm::vec4& v) -> coord_t
                    {
                        return v.x;
                    }
                );

                coord_t xmax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
                    [](glm::vec4& v)  -> coord_t
                    {
                        return v.x;
                    }
                );

                offset_minx = (xmin - x1) / width - 1;
                offset_maxx = (xmax - x1) / width + 1;
            }
            else
            {
                offset_minx = 0;
                offset_maxx = 1;
            }

            if (tiled_y)
            {
                coord_t ymin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
                    [](glm::vec4& v) -> coord_t
                    {
                        return v.y;
                    }
                );

                coord_t ymax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
                    [](glm::vec4& v)  -> coord_t
                    {
                        return v.y;
                    }
                );

                offset_miny = (ymin - y1) / height - 1;
                offset_maxy = (ymax - y1) / height + 1;
            }
            else
            {
                offset_miny = 0;
                offset_maxy = 1;
            }
        }

        auto offset_to_world = [&](int32_t x, int32_t y) -> geometry::Vector<coord_t>
        {
            return { x1 + width * x, y1 + height * y };
        };

        const size_t vertex_count = (offset_maxx - offset_minx) * (offset_maxy - offset_miny) * 6;
        float* const vertices_o = new float[vertex_count * k_vertex_data_size];
        float* vertices = vertices_o;
        for (int32_t i = offset_minx; i < offset_maxx; ++i)
        {
            for (int32_t j = offset_miny; j < offset_maxy; ++j)
            {
                auto p = offset_to_world(i, j);
                floats3pm(vertices + 0*k_vertex_data_size, p.x, p.y);
                floats3pm(vertices + 1*k_vertex_data_size, p.x, p.y + height);
                floats3pm(vertices + 2*k_vertex_data_size, p.x + width, p.y);
                floats3pm(vertices + 3*k_vertex_data_size, p.x + width, p.y);
                floats3pm(vertices + 4*k_vertex_data_size, p.x, p.y + height);
                floats3pm(vertices + 5*k_vertex_data_size, p.x + width, p.y + height);

                colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
                colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
                colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
                colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[2]);
                colour4_to_floats(vertices + 4*k_vertex_data_size + 3, g_draw_colour[1]);
                colour4_to_floats(vertices + 5*k_vertex_data_size + 3, g_draw_colour[3]);

                vertices[0*k_vertex_data_size + 7] = texture->u_global(tx1);
                vertices[0*k_vertex_data_size + 8] = texture->v_global(ty1);

                vertices[1*k_vertex_data_size + 7] = texture->u_global(tx1);
                vertices[1*k_vertex_data_size + 8] = texture->v_global(ty2);

                vertices[2*k_vertex_data_size + 7] = texture->u_global(tx2);
                vertices[2*k_vertex_data_size + 8] = texture->v_global(ty1);

                vertices[3*k_vertex_data_size + 7] = texture->u_global(tx2);
                vertices[3*k_vertex_data_size + 8] = texture->v_global(ty1);

                vertices[4*k_vertex_data_size + 7] = texture->u_global(tx1);
                vertices[4*k_vertex_data_size + 8] = texture->v_global(ty2);

                vertices[5*k_vertex_data_size + 7] = texture->u_global(tx2);
                vertices[5*k_vertex_data_size + 8] = texture->v_global(ty2);

                // TODO: switch to trianglestrip for more efficient vertex packing.
                vertices += k_vertex_data_size * 6;
            }
        }

        render_vertices(vertices_o, vertex_count, texture->m_tpage->m_gl_tex, GL_TRIANGLES);
        delete[] vertices_o;
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::draw_image(TextureView* texture, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2)
{
    if (texture->m_tpage->cache())
    {
        // set render data

        float vertices[k_vertex_data_size * 4];
        floats3pm(vertices + 0*k_vertex_data_size, x1, y1);
        floats3pm(vertices + 1*k_vertex_data_size, x2, y1);
        floats3pm(vertices + 2*k_vertex_data_size, x1, y2);
        floats3pm(vertices + 3*k_vertex_data_size, x2, y2);

        // colour data
        colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
        colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
        colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
        colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[3]);

        // texture coordinates
        vertices[0*k_vertex_data_size + 7] = texture->u_global(tx1);
        vertices[0*k_vertex_data_size + 8] = texture->v_global(ty1);

        vertices[1*k_vertex_data_size + 7] = texture->u_global(tx2);
        vertices[1*k_vertex_data_size + 8] = texture->v_global(ty1);

        vertices[2*k_vertex_data_size + 7] = texture->u_global(tx1);
        vertices[2*k_vertex_data_size + 8] = texture->v_global(ty2);

        vertices[3*k_vertex_data_size + 7] = texture->u_global(tx2);
        vertices[3*k_vertex_data_size + 8] = texture->v_global(ty2);

        // how to interpret `vertices`
        render_vertices(vertices, 4, texture->m_tpage->m_gl_tex, GL_TRIANGLE_STRIP);
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::draw_image(TextureView* texture, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4)
{
    if (texture->m_tpage->cache())
    {
        // set render data

        float vertices[k_vertex_data_size * 4];
        floats3(vertices + 0*k_vertex_data_size, x1, y1);
        floats3(vertices + 1*k_vertex_data_size, x2, y2);
        floats3(vertices + 2*k_vertex_data_size, x3, y3);
        floats3(vertices + 3*k_vertex_data_size, x4, y4);

        // colour data
        colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
        colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
        colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
        colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[3]);

        // texture coordinates
        vertices[0*k_vertex_data_size + 7] = texture->u_global(tx1);
        vertices[0*k_vertex_data_size + 8] = texture->v_global(ty1);

        vertices[1*k_vertex_data_size + 7] = texture->u_global(tx2);
        vertices[1*k_vertex_data_size + 8] = texture->v_global(ty2);

        vertices[2*k_vertex_data_size + 7] = texture->u_global(tx3);
        vertices[2*k_vertex_data_size + 8] = texture->v_global(ty3);

        vertices[3*k_vertex_data_size + 7] = texture->u_global(tx4);
        vertices[3*k_vertex_data_size + 8] = texture->v_global(ty4);

        // how to interpret `vertices`
        render_vertices(vertices, 4, texture->m_tpage->m_gl_tex, GL_TRIANGLE_FAN);
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::begin_render()
{
    glCheckErrorStr("pre-begin");
    glViewport(0, 0, g_window_width, g_window_height);
    g_viewport_dimensions.x = g_window_width;
    g_viewport_dimensions.y = g_window_height;
    glCheckErrorStr("begin");
}

void Display::clear_render()
{
    glClearColor(g_clear_colour.r, g_clear_colour.g, g_clear_colour.b, g_clear_colour.a);
    glClear( GL_COLOR_BUFFER_BIT );
}

void Display::end_render()
{
    SDL_GL_SwapWindow(g_window);
    glCheckErrorStr("Render end");
}

bool Display::window_close_requested()
{
    return g_sdl_closing;
}

void Display::set_clear_colour(uint32_t z)
{
    g_clear_colour = bgrz_to_colour3(z);
}

uint32_t Display::get_clear_colour()
{
    return colour3_to_bgrz(g_clear_colour);
}

uint32_t Display::get_colour()
{
    return colour3_to_bgrz(g_draw_colour[0]);
}

void Display::set_colour(uint32_t c)
{
    float prev_alpha = g_draw_colour[0].a;
    for (size_t i = 0; i < 4; ++i)
    {
        g_draw_colour[i] = bgrz_to_colour3(c);
        g_draw_colour[i].a = prev_alpha;
    }
}

float Display::get_alpha()
{
    return g_draw_colour[0].a;
}

void Display::set_alpha(float a)
{
    for (size_t i = 0; i < 4; ++i)
    {
        g_draw_colour[i].a = a;
    }
}

void Display::get_colours4(uint32_t* colours)
{
    for (size_t i = 0; i < 4; ++i)
    {
        colours[i] = colour4_to_bgraz(g_draw_colour[i]);
    }
}

void Display::set_colours4(uint32_t colours[4])
{
    for (size_t i = 0; i < 4; ++i)
    {
        g_draw_colour[i] = bgraz_to_colour4(colours[i]);
    }
}

void Display::set_font(asset::AssetFont* af, TextureView* tv)
{
    m_font = af;
    m_font_texture = tv;
}

namespace
{
    // ogm -> opengl
    inline int32_t bm_constant(int32_t type)
    {
        switch(type)
        {
        // see fn_draw.h constants
        case 4: return GL_ZERO;
        case 5: return GL_ONE;
        case 6: return GL_SRC_COLOR;
        case 7: return GL_ONE_MINUS_SRC_COLOR;
        case 8: return GL_SRC_ALPHA;
        case 9: return GL_ONE_MINUS_SRC_ALPHA;
        case 10: return GL_DST_ALPHA;
        case 11: return GL_ONE_MINUS_DST_ALPHA;
        case 12: return GL_DST_COLOR;
        case 13: return GL_ONE_MINUS_DST_COLOR;
        case 14: return GL_SRC_ALPHA_SATURATE;
        default:
            // error
            return GL_ZERO;
        }
    }
}

void Display::set_blendmode(int32_t src, int32_t dst)
{
    glBlendFunc(bm_constant(src), bm_constant(dst));
}

void Display::set_blending_enabled(bool c)
{
    g_blending_enabled = c;
}

void Display::shader_set_alpha_test_enabled(bool enabled)
{
    int32_t alpha_enable_loc = glGetUniformLocation(g_shader_program, "gm_AlphaTestEnabled");
    glUniform1i(alpha_enable_loc, enabled);
}

void Display::shader_set_alpha_test_threshold(real_t value)
{
    int32_t alpha_enable_loc = glGetUniformLocation(g_shader_program, "gm_AlphaRefValue");
    glUniform1f(alpha_enable_loc, value);
}


void Display::disable_scissor()
{
    glDisable(GL_SCISSOR_TEST);
}

void Display::enable_scissor(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{
    glEnable(GL_SCISSOR_TEST);
    if (g_flip_projection)
    {
        y1 = g_viewport_dimensions.y - y1;
        y2 = g_viewport_dimensions.y - y2;
    }

    if (x2 < x1) std::swap(x1, x2);
    if (y2 < y1) std::swap(y1, y2);

    glScissor(x1, y1, x2 - x1, y2 - y1);
}

// TODO: allow taking uv coordinates
// TODO: allow 6-vertex GL_TRIANGLES instead of 4-vertex for GL_TRIANGLE_STRIP.
namespace
{
    inline void rectangle_vertices(float* vertices, coord_t x1, coord_t y1, coord_t x2, coord_t y2)
    {
        floats3pm(vertices + 0*k_vertex_data_size, x1, y1);
        floats3pm(vertices + 1*k_vertex_data_size, x1, y2);
        floats3pm(vertices + 2*k_vertex_data_size, x2, y1);
        floats3pm(vertices + 3*k_vertex_data_size, x2, y2);

        // colour data
        colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
        colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
        colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
        colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[3]);

        // texture coordinates
        vertices[0*k_vertex_data_size + 7] = 0;
        vertices[0*k_vertex_data_size + 8] = 0;

        vertices[1*k_vertex_data_size + 7] = 0;
        vertices[1*k_vertex_data_size + 8] = 1;

        vertices[2*k_vertex_data_size + 7] = 1;
        vertices[2*k_vertex_data_size + 8] = 0;

        vertices[3*k_vertex_data_size + 7] = 1;
        vertices[3*k_vertex_data_size + 8] = 1;
    }
}

void Display::draw_filled_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{
    // set render data
    float vertices[k_vertex_data_size * 4];
    rectangle_vertices(vertices, x1, y1, x2, y2);

    // how to interpret `vertices`
    render_vertices(vertices, 4, g_blank_texture, GL_TRIANGLE_STRIP);
}

void Display::draw_filled_circle(coord_t x, coord_t y, coord_t r)
{
    // set render data
    const size_t vertex_size = k_vertex_data_size;
    const size_t vertex_count = (g_circle_precision + 2);
    float* vertices = new float[vertex_size * vertex_count];

    // xyz
    floats3pm(vertices + 0, x, y);
    colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[1]);
    vertices[0*k_vertex_data_size + 7] = 0.5;
    vertices[0*k_vertex_data_size + 8] = 0.5;

    for (size_t i = 1; i <= g_circle_precision + 1; ++i)
    {
        float p = static_cast<float>(i)/g_circle_precision;
        float theta = p * 6.283185307;
        float cost = -std::cos(theta);
        float sint = std::sin(theta);
        floats3pm(vertices + i*k_vertex_data_size, x + r * sint, y + r*cost);
        colour4_to_floats(vertices + i*k_vertex_data_size + 3, g_draw_colour[0]);
        vertices[i*k_vertex_data_size + 7] = sint;
        vertices[i*k_vertex_data_size + 8] = cost;
    }

    render_vertices(vertices, vertex_count, g_blank_texture, GL_TRIANGLE_FAN);
    delete[] vertices;
}

void Display::draw_outline_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{
    // set render data
    float vertices[k_vertex_data_size * 5];
    floats3pm(vertices + 0*k_vertex_data_size, x1, y1);
    floats3pm(vertices + 1*k_vertex_data_size, x2, y1);
    floats3pm(vertices + 2*k_vertex_data_size, x2, y2);
    floats3pm(vertices + 3*k_vertex_data_size, x1, y2);
    floats3pm(vertices + 4*k_vertex_data_size, x1, y1);

    // colour data
    colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
    colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
    colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[3]);
    colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[2]);
    colour4_to_floats(vertices + 4*k_vertex_data_size + 3, g_draw_colour[0]);

    // texture coordinates
    vertices[0*k_vertex_data_size + 7] = 0;
    vertices[0*k_vertex_data_size + 8] = 0;

    vertices[1*k_vertex_data_size + 7] = 1;
    vertices[1*k_vertex_data_size + 8] = 0;

    vertices[2*k_vertex_data_size + 7] = 1;
    vertices[2*k_vertex_data_size + 8] = 1;

    vertices[3*k_vertex_data_size + 7] = 0;
    vertices[3*k_vertex_data_size + 8] = 1;

    vertices[4*k_vertex_data_size + 7] = 0;
    vertices[4*k_vertex_data_size + 8] = 0;

    // how to interpret `vertices`
    render_vertices(vertices, 5, g_blank_texture, GL_LINE_STRIP);
}

void Display::draw_outline_circle(coord_t x, coord_t y, coord_t r)
{
    // set render data
    const size_t vertex_size = k_vertex_data_size;
    const size_t vertex_count = (g_circle_precision);
    float* vertices = new float[vertex_size * vertex_count];

    // xyz

    for (size_t i = 0; i < g_circle_precision; ++i)
    {
        float p = static_cast<float>(i)/g_circle_precision;
        float theta = p * 6.283185307;
        float cost = -std::cos(theta);
        float sint = std::sin(theta);
        floats3pm(vertices + i*k_vertex_data_size, x + r * sint, y + r*cost);
        colour4_to_floats(vertices + i*k_vertex_data_size + 3, g_draw_colour[0]);
        vertices[i*k_vertex_data_size + 7] = sint;
        vertices[i*k_vertex_data_size + 8] = cost;
    }

    render_vertices(vertices, vertex_count, g_blank_texture, GL_LINE_LOOP);
    delete[] vertices;
}

void Display::set_circle_precision(uint32_t prec)
{
    g_circle_precision = prec;
}

uint32_t Display::get_circle_precision()
{
    return g_circle_precision;
}

void Display::draw_fill_colour(uint32_t colour)
{
    colour4 c = bgraz_to_colour4(colour);
    glClearColor(c.r, c.g, c.b, c.a );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Display::draw_text(coord_t _x, coord_t _y, const char* text, real_t halign, real_t valign, bool use_max_width, coord_t max_width, bool use_sep, coord_t sep)
{
    if (!m_font || m_font->m_ttf)
    {
        draw_text_ttf(_x, _y, text, halign, valign, use_max_width, max_width, use_sep, sep);
    }
    else
    {
        draw_text_image(_x, _y, text, halign, valign, use_max_width, max_width, use_sep, sep);
    }
}

void Display::draw_text_image(coord_t _x, coord_t _y, const char* text, real_t halign, real_t valign, bool use_max_width, coord_t max_width, bool use_sep, coord_t sep)
{
    if (!m_font || !m_font_texture) return;

    if (!m_font_texture->m_tpage->cache()) return;

    const size_t lenstr = strlen(text);
    const size_t max_vertex_count = lenstr * 6;
    float* const vertices = new float[max_vertex_count * k_vertex_data_size];
    float* const end_vertices = vertices + max_vertex_count * k_vertex_data_size;

    real_t h = 0;
    std::vector<real_t> text_width;
    const std::string_view sv = text;
    std::vector<std::string_view> lines;
    {
        size_t line_start = 0;
        size_t line_width = 0;
        int32_t last_whitespace = -1; // -1 means no whitespace on this line yet to break at.
        size_t width_at_whitespace = 0;
        for (size_t i = 0; i < sv.length(); ++i)
        {
            char c = sv[i];
            if (c == '\n')
            {
                // newline character encountered -- add a new line.
                lines.push_back(sv.substr(line_start, i - line_start));
                text_width.push_back(line_width);
                line_start = i + 1;

            start_newline:
                line_width = 0;
                last_whitespace = -1;
                if (use_sep) h += sep;
                else h += m_font->m_vshift;
            }
            else
            {
                // note position if line-breakable whitespace
                if (c == ' ' || c == '\t')
                {
                    last_whitespace = i;
                    width_at_whitespace = line_width;
                }

                // add a character to the current line
                if (m_font->m_glyphs.find(c) != m_font->m_glyphs.end())
                {
                    size_t cwidth = m_font->m_glyphs.at(c).m_shift;
                    line_width += cwidth;
                    // start a new line if it would spill past width
                    if (use_max_width)
                    {
                        if (line_width > max_width)
                        {
                            if (last_whitespace == -1)
                            // no whitespace to break at -- break here instead
                            {
                                bool breakafter = cwidth >= line_width; // break after this character if necessary.
                                if (!breakafter)
                                    line_width -= cwidth;
                                lines.push_back(sv.substr(line_start, i - line_start + breakafter));
                                text_width.push_back(line_width);

                                // reset variables for new line
                                line_start = i + !breakafter;
                                if (breakafter) --i; // try this character again.
                                goto start_newline;
                            }
                            else
                            {
                                i = last_whitespace + 1;
                                lines.push_back(sv.substr(line_start, last_whitespace - line_start));
                                text_width.push_back(width_at_whitespace);

                                // reset variables for new line
                                line_start = i;
                                goto start_newline;
                            }
                        }
                    }
                }
            }
        }
        // last line
        lines.push_back(sv.substr(line_start, sv.length() - line_start));
        text_width.push_back(line_width);

        // vshift
        if (use_sep) h += sep;
        else h += m_font->m_vshift;
    }

    ogm::geometry::Vector<real_t> texture_dim_i = m_font_texture->dim_global_i();

    size_t vertex_count = 0;

    coord_t draw_y = _y - valign * h;
    float* mvertices = vertices;
    size_t cc = 0;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        coord_t draw_x = _x - halign * text_width.at(i);
        const std::string_view& line = lines.at(i);

        for (size_t j = 0; j < line.length(); ++j)
        {
            if (cc >= lenstr) goto render;
            char c = line[j];
            ++cc;
            if (m_font->m_glyphs.find(c) != m_font->m_glyphs.end())
            {
                const asset::Glyph& glyph = m_font->m_glyphs.at(c);

                if (c != 32) // never render spaces, even if there is image data
                {
                    ogm_assert(mvertices < end_vertices);
                    floats3pm(mvertices + 0*k_vertex_data_size, draw_x + glyph.m_offset, draw_y);
                    floats3pm(mvertices + 1*k_vertex_data_size, draw_x + glyph.m_offset + glyph.m_dimensions.x , draw_y);
                    floats3pm(mvertices + 2*k_vertex_data_size, draw_x + glyph.m_offset, draw_y + glyph.m_dimensions.y);
                    floats3pm(mvertices + 3*k_vertex_data_size, draw_x + glyph.m_offset, draw_y + glyph.m_dimensions.y);
                    floats3pm(mvertices + 4*k_vertex_data_size, draw_x + glyph.m_offset + glyph.m_dimensions.x, draw_y);
                    floats3pm(mvertices + 5*k_vertex_data_size, draw_x + glyph.m_offset + glyph.m_dimensions.x, draw_y + glyph.m_dimensions.y);

                    // colour data (TODO: interpolate this globally, not locally)
                    colour4_to_floats(mvertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
                    colour4_to_floats(mvertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
                    colour4_to_floats(mvertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
                    colour4_to_floats(mvertices + 3*k_vertex_data_size + 3, g_draw_colour[2]);
                    colour4_to_floats(mvertices + 4*k_vertex_data_size + 3, g_draw_colour[1]);
                    colour4_to_floats(mvertices + 5*k_vertex_data_size + 3, g_draw_colour[3]);

                    // UV
                    // OPTIMIZE: precompute these values per font.
                    mvertices[0*k_vertex_data_size + 7] = m_font_texture->u_global(glyph.m_position.x / texture_dim_i.x);
                    mvertices[0*k_vertex_data_size + 8] = m_font_texture->v_global(glyph.m_position.y / texture_dim_i.y);

                    mvertices[1*k_vertex_data_size + 7] = m_font_texture->u_global((glyph.m_position.x + glyph.m_dimensions.x) / texture_dim_i.x);
                    mvertices[1*k_vertex_data_size + 8] = m_font_texture->v_global(glyph.m_position.y / texture_dim_i.y);

                    mvertices[2*k_vertex_data_size + 7] = m_font_texture->u_global(glyph.m_position.x / texture_dim_i.x);
                    mvertices[2*k_vertex_data_size + 8] = m_font_texture->v_global((glyph.m_position.y + glyph.m_dimensions.y) / texture_dim_i.y);

                    mvertices[3*k_vertex_data_size + 7] = m_font_texture->u_global(glyph.m_position.x / texture_dim_i.x);
                    mvertices[3*k_vertex_data_size + 8] = m_font_texture->v_global((glyph.m_position.y + glyph.m_dimensions.y) / texture_dim_i.y);

                    mvertices[4*k_vertex_data_size + 7] = m_font_texture->u_global((glyph.m_position.x + glyph.m_dimensions.x) / texture_dim_i.x);
                    mvertices[4*k_vertex_data_size + 8] = m_font_texture->v_global(glyph.m_position.y / texture_dim_i.y);

                    mvertices[5*k_vertex_data_size + 7] = m_font_texture->u_global((glyph.m_position.x + glyph.m_dimensions.x) / texture_dim_i.x);
                    mvertices[5*k_vertex_data_size + 8] = m_font_texture->v_global((glyph.m_position.y + glyph.m_dimensions.y) / texture_dim_i.y);

                    vertex_count+= 6;
                    mvertices += k_vertex_data_size * 6;
                }
                draw_x += glyph.m_shift;
            }
        }

        if (use_sep) draw_y += sep;
        else draw_y += m_font->m_vshift;
    }

render:
    render_vertices(vertices, vertex_count,
        //g_blank_texture,
        m_font_texture->m_tpage->m_gl_tex,
        GL_TRIANGLES);

    delete[] vertices;
}

void Display::draw_text_ttf(coord_t _x, coord_t _y, const char* text, real_t halign, real_t valign, bool use_max_width, coord_t max_width, bool use_sep, coord_t sep)
{
    #ifdef GFX_TEXT_AVAILABLE
    ogm_assert(halign >= 0 && halign <= 1);
    ogm_assert(valign >= 0 && valign <= 1);

    // metrics
    uint32_t lineskip = TTF_FontLineSkip(g_font);
    if (use_sep)
    {
        lineskip = sep;
    }

    // OPTIMIZE remove this split
    std::vector<std::string> lines;
    split(lines, text, "\n", false);
    if (use_max_width)
    {
        // split long lines
        // OPTIMIZE
        for (size_t i = 0; i < lines.size(); ++i)
        {
            int w, h;
            if (TTF_SizeText(g_font, lines.at(i).c_str(), &w, &h))
            {
                if (w > max_width)
                {
                    std::string line = lines.at(i);
                    while (true)
                    {
                        // identify split point (could be binary search?)
                        for (size_t j = 2; j < line.size(); ++i)
                        {
                            std::string sub = line.substr(0, j);
                            if (TTF_SizeText(g_font, sub.c_str(), &w, &h))
                            {
                                if (w > max_width)
                                {
                                    std::string insert = line.substr(0, j - 1);
                                    line = line.substr(j - 1);
                                    ++i;
                                    lines.insert(lines.begin() + i, insert);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    real_t total_h = lineskip * lines.size();
    coord_t y = _y - total_h * valign + TTF_FontDescent(g_font);

    for (const std::string& line : lines)
    {
        if (line.length() > 0)
        {
            SDL_Surface* text_srf = TTF_RenderText_Solid(g_font, line.c_str(), SDL_Color{255, 255, 255});
            if (!text_srf)
            {
                std::cerr << "Failed to render text: \"" << line << "\"\n";
                std::cerr << TTF_GetError();
                y += lineskip;
                continue;
            }

            uint32_t w = power_of_two(text_srf->w);
        	uint32_t h = power_of_two(text_srf->h);

        	SDL_Surface* tmp = SDL_CreateRGBSurface(0, w, h, 32,
        			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

            SDL_BlitSurface(text_srf, nullptr, tmp, nullptr);

            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels);

            coord_t x = _x - text_srf->w * halign;

            float vertices[k_vertex_data_size * 4];
            rectangle_vertices(vertices, x, y, x + w, y + h);

            render_vertices(vertices, 4, texture, GL_TRIANGLE_STRIP);

            glDeleteTextures(1, &texture);
            SDL_FreeSurface(text_srf);
            SDL_FreeSurface(tmp);
        }
        y += lineskip;
    }
    #endif
}

namespace
{
    bool g_frame_key_down[k_keycode_max];
    bool g_frame_key_pressed[k_keycode_max];
    bool g_frame_key_released[k_keycode_max];
}

void Display::clear_keys()
{
    for (size_t i = 0; i < k_keycode_max; ++i)
    {
        g_frame_key_down[i] = false;
        g_frame_key_pressed[i] = false;
        g_frame_key_released[i] = false;
        g_key_down[i] = false;
        g_key_pressed[i] = false;
        g_key_released[i] = false;
    }
}

namespace
{
    // TODO: cleanup with SDL_GameControllerClose()
    std::vector<SDL_GameController*> g_joysticks;

    struct JSInfo
    {
        std::vector<bool> m_button_down;
        std::vector<bool> m_button_pressed;
        std::vector<bool> m_button_released;
    };
    std::vector<JSInfo> g_jsinfo;

    SDL_GameController* get_joystick_from_index(int index)
    {
        SDL_GameController* js = SDL_GameControllerFromInstanceID(index);
        if (!js)
        {
            js = SDL_GameControllerOpen(index);
            if (js)
            {
                g_joysticks.push_back(js);
                return js;
            }
        }
        else
        {
            return js;
        }
        return nullptr;
    }

    void sdl_process_joysticks()
    {
        SDL_JoystickUpdate();

        for (size_t i = 0; i < SDL_NumJoysticks(); ++i)
        {
            while (g_jsinfo.size() <= i) g_jsinfo.emplace_back();
            SDL_GameController* gc = get_joystick_from_index(i);
            if (gc)
            {
                SDL_Joystick* js = SDL_GameControllerGetJoystick(gc);
                JSInfo& info = g_jsinfo.at(i);
                if (js)
                {
                    size_t num_buttons = SDL_JoystickNumButtons(js);
                    size_t num_hats = SDL_JoystickNumHats(js);
                    size_t total_virtual_buttons = num_buttons + 4*num_hats;
                    info.m_button_down.resize(total_virtual_buttons, 0);
                    info.m_button_pressed.resize(total_virtual_buttons, 0);
                    info.m_button_released.resize(total_virtual_buttons, 0);
                    uint8_t hat_value = 0;
                    for (size_t b = 0; b < total_virtual_buttons; b++)
                    {
                        bool prev = info.m_button_down.at(b);
                        bool _new;
                        if (b < num_buttons)
                        {
                            // button state
                            _new = SDL_JoystickGetButton(js, b);
                        }
                        else
                        {
                            // hat state
                            size_t hat_index = b - num_buttons;
                            size_t hat_subindex = hat_index % 4;
                            hat_index /= 4;

                            if (hat_subindex == 0) hat_value = SDL_JoystickGetHat(js, hat_index);

                            _new = false;
                            switch (hat_subindex)
                            {
                            case 0: // left
                                if (hat_value == SDL_HAT_LEFTUP || hat_value == SDL_HAT_LEFT || hat_value == SDL_HAT_LEFTDOWN)
                                    _new = true;
                                break;
                            case 1: // right
                                if (hat_value == SDL_HAT_RIGHTUP || hat_value == SDL_HAT_RIGHT || hat_value == SDL_HAT_RIGHTDOWN)
                                    _new = true;
                                break;
                            case 2: // up
                                if (hat_value == SDL_HAT_LEFTUP || hat_value == SDL_HAT_UP || hat_value == SDL_HAT_RIGHTUP)
                                    _new = true;
                                break;
                            case 3: // down
                                if (hat_value == SDL_HAT_LEFTDOWN || hat_value == SDL_HAT_DOWN || hat_value == SDL_HAT_RIGHTDOWN)
                                    _new = true;
                                break;
                            default:
                                break;
                            }
                        }
                        info.m_button_down.at(b) = _new;
                        info.m_button_pressed.at(b) = _new && !prev;
                        info.m_button_released.at(b) = !_new && prev;
                    }
                }
                else
                {
                    info.m_button_down.clear();
                    info.m_button_pressed.clear();
                    info.m_button_released.clear();
                }
            }
        }
    }

    void sdl_process_keys()
    {
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            ogm_keycode_t keycode;
            switch(event.type)
            {
            case SDL_KEYDOWN:
                keycode = sdl_to_ogm_keycode(event.key.keysym.sym);
                g_key_pressed[keycode] = true;
                g_key_down[keycode] = true;
                break;

            case SDL_KEYUP:
                keycode = sdl_to_ogm_keycode(event.key.keysym.sym);
                g_key_released[keycode] = true;
                g_key_down[keycode] = false;
                break;

            case SDL_QUIT:
                g_sdl_closing = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
                keycode = sdl_to_ogm_mousecode(event.button.button);
                g_key_pressed[keycode] = true;
                g_key_down[keycode] = true;
                break;

            case SDL_MOUSEBUTTONUP:
                keycode = sdl_to_ogm_mousecode(event.button.button);
                g_key_released[keycode] = true;
                g_key_down[keycode] = false;
                break;

            default:
                break;
            }
        }
    }
}

void Display::process_keys()
{
    // copy over to new buffer
    sdl_process_keys();

    // not threadsafe, but it doesn't really matter.
    for (size_t i = 0; i < k_keycode_max; ++i)
    {
        // determine
        bool prev_down = g_frame_key_down[i];
        bool down = g_key_down[i];
        if (prev_down && g_key_released[i])
        {
            down = false;
        }
        else if (!prev_down && g_key_pressed[i])
        {
            down = true;
        }

        g_key_pressed[i] = false;
        g_key_released[i] = false;

        g_frame_key_down[i] = down;
        g_frame_key_released[i] = prev_down && !down;
        g_frame_key_pressed[i] = !prev_down && down;
    }

    sdl_process_joysticks();
}

bool Display::get_key_down(ogm_keycode_t i)
{
    return g_frame_key_down[i];
}

bool Display::get_key_pressed(ogm_keycode_t i)
{
    return g_frame_key_pressed[i];
}

bool Display::get_key_released(ogm_keycode_t i)
{
    return g_frame_key_released[i];
}

bool Display::get_key_direct(ogm_keycode_t i)
{
    size_t scancode = ogm_to_sdl_scancode(i);
    if (scancode != std::numeric_limits<size_t>::max())
    {
        const uint8_t* state = SDL_GetKeyboardState(nullptr);
        return !! state[scancode];
    }
    return false;
}

ogm_keycode_t Display::get_current_key()
{
    int num;
    const uint8_t* state = SDL_GetKeyboardState(&num);
    for (int i = 0; i < num; ++i)
    {
        if (state[i])
        {
            return sdl_scancode_to_ogm(i);
        }
    }
    return 0;
}

void Display::set_matrix_projection()
{
    glm::mat4& projection = g_matrices[MATRIX_PROJECTION];
    projection = glm::mat4(1.0f);

    if (g_flip_projection)
    {
        g_matrices[MATRIX_PROJECTION] =
            glm::scale(glm::mat4(1.0), {1, -1, 1})
            * g_matrices[MATRIX_PROJECTION];
    }

    update_camera_matrices();
}

void Display::set_matrix_view(coord_t x1, coord_t y1, coord_t x2, coord_t y2, real_t angle)
{
    glm::mat4& view = g_matrices[MATRIX_VIEW];
    view = glm::mat4(1);
    view = glm::rotate(view, static_cast<float>(angle), glm::vec3(0, 0, -1));
    view = glm::translate(view, glm::vec3(-1, 1, 0));
    view = glm::scale(view, glm::vec3(2, -2, 1));
    view = glm::scale(view, glm::vec3(1.0/(x2 - x1), 1.0/(y2 - y1), 1));
    view = glm::translate(view, glm::vec3(-x1, -y1, 0));

    // inverse matrix
    glm::mat4& iview = g_imatrices[MATRIX_VIEW];
    iview = glm::mat4(1);
    iview = glm::translate(iview, glm::vec3(x1, y1, 0));
    iview = glm::scale(iview, glm::vec3((x2 - x1), (y2 - y1), 1));
    iview = glm::scale(iview, glm::vec3(0.5, -0.5, 1));
    iview = glm::translate(iview, glm::vec3(1.0, -1.0, 0));
    iview = glm::rotate(iview, -static_cast<float>(angle), glm::vec3(0, 0, -1));

    update_camera_matrices();
}

void Display::set_matrix_pre_model(coord_t x, coord_t y, coord_t xscale, coord_t yscale, real_t angle)
{
    glm::mat4& model = g_pre_model;
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0));
    model = glm::rotate(model, static_cast<float>(angle), glm::vec3(0, 0, -1));
    model = glm::scale(model, glm::vec3(xscale, yscale, 1));
}

void Display::set_matrix_pre_model()
{
    glm::mat4& model = g_pre_model;
    model = glm::mat4(1.0f);
}

void Display::set_matrix_model(coord_t x, coord_t y, coord_t xscale, coord_t yscale, real_t angle)
{
    glm::mat4& model = g_matrices[MATRIX_MODEL];
    model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(x, y, 0));
    model = glm::rotate(model, static_cast<float>(angle), glm::vec3(0, 0, -1));
    model = glm::scale(model, glm::vec3(xscale, yscale, 1));

    // inverse matrix?
    // Not actually needed presently.

    update_camera_matrices();
}

ogm::geometry::AABB<coord_t> Display::get_viewable_aabb()
{
    glm::vec4 p[4] = {
        { 1, 1, 0, 1 },
        { -1, -1, 0, 1 },
        { 1, 1, 0, 1 },
        { -1, 1, 0, 1 }
    };
    geometry::Vector<coord_t> vecs[4];
    for (size_t i = 0; i < 4; ++i)
    {
        p[i] = g_imatrices[MATRIX_VIEW] * p[i];
        vecs[i] = { p[i].x, p[i].y };
    }

    // vecs to aabb
    coord_t xmin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
        [](glm::vec4& v) -> coord_t
        {
            return v.x;
        }
    );

    coord_t xmax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
        [](glm::vec4& v)  -> coord_t
        {
            return v.x;
        }
    );

    coord_t ymin = mapped_minimum<coord_t>(std::begin(p), std::end(p),
        [](glm::vec4& v) -> coord_t
        {
            return v.y;
        }
    );

    coord_t ymax = mapped_maximum<coord_t>(std::begin(p), std::end(p),
        [](glm::vec4& v)  -> coord_t
        {
            return v.y;
        }
    );

    return { xmin, ymin, xmax, ymax };
}

void Display::enable_view_projection(bool a)
{
    g_enable_view_projection = a;

    update_camera_matrices();
}

void Display::update_camera_matrices()
{
    // calculate combined matrices
    g_matrices[MATRIX_MODEL_VIEW] = g_matrices[MATRIX_VIEW] * g_matrices[MATRIX_MODEL];
    g_matrices[MATRIX_MODEL_VIEW_PROJECTION] = g_matrices[MATRIX_PROJECTION] * g_matrices[MATRIX_MODEL_VIEW];

    if (!g_enable_view_projection)
    {
        g_matrix_surface_view = glm::mat4(1.0f);
        g_matrix_surface_view = glm::translate(g_matrix_surface_view, {-1.0, -1.0, 0.0});
        glm::vec3 scale;
        if (g_viewport_dimensions.x != 0 && g_viewport_dimensions.y != 0)
        {
             scale = {
                 static_cast<float>(2.0/g_viewport_dimensions.x),
                 static_cast<float>(2.0/g_viewport_dimensions.y),
                 1.0f
             };
        }
        g_matrix_surface_view = glm::scale(
            g_matrix_surface_view,
            scale
        );
        g_matrix_surface_model_view = g_matrix_surface_view * g_matrices[MATRIX_MODEL];
    }

    // inverse matrices
    #if 0
    // not actually used anywhere.
    g_imatrices[MATRIX_MODEL_VIEW] = g_imatrices[MATRIX_MODEL] * g_imatrices[MATRIX_VIEW];
    g_imatrices[MATRIX_MODEL_VIEW_PROJECTION] = g_imatrices[MATRIX_MODEL_VIEW] * g_imatrices[MATRIX_PROJECTION];
    #endif

    // set uniforms
    for (size_t i = 0; i < 5; ++i)
    {
        // OPTIMIZE: look up uniform location in advance
        uint32_t matvloc = glGetUniformLocation(g_shader_program, "gm_Matrices");
        if (!g_enable_view_projection)
        {
            // when a surface target is set, view and perspective matrices are ignored.
            switch(i)
            {
            case MATRIX_MODEL:
                glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrices[MATRIX_MODEL]));
                break;
            case MATRIX_PROJECTION:
                glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrix_identity));
                break;
            case MATRIX_VIEW:
                glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrix_surface_view));
                break;
            case MATRIX_MODEL_VIEW:
            case MATRIX_MODEL_VIEW_PROJECTION:
                glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrix_surface_model_view));
                break;
            }
        }
        else
        {
            glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrices[i]));
        }
    }
}

void Display::set_fog(bool enabled, real_t start, real_t end, uint32_t col)
{
    // OPTIMIZE: look up uniform location in advance
    int32_t fog_enabledv_loc = glGetUniformLocation(g_shader_program, "gm_VS_FogEnabled");
    int32_t fog_enabledf_loc = glGetUniformLocation(g_shader_program, "gm_PS_FogEnabled");
    int32_t fog_start_loc = glGetUniformLocation(g_shader_program, "gm_FogStart");
    int32_t fog_rcp_loc = glGetUniformLocation(g_shader_program, "gm_RcpFogRange");
    int32_t fog_col_loc = glGetUniformLocation(g_shader_program, "gm_FogColour");

    if (fog_enabledf_loc < 0 || fog_enabledv_loc < 0 || fog_start_loc < 0 || fog_rcp_loc < 0 || fog_col_loc < 0)
    {
        return;
    }

    colour3 c = bgrz_to_colour3(col);
    glUniform1i(fog_enabledv_loc, enabled);
    glUniform1i(fog_enabledf_loc, enabled);
    if (!enabled)
    {
        glUniform1f(fog_start_loc, 0);
        glUniform1f(fog_rcp_loc, 0);
    }
    else
    {
        if (end > start)
        {
            glUniform1f(fog_start_loc, start);
            glUniform1f(fog_rcp_loc, 1.0 / (end - start));
        }
        else
        {
            // invalid fog distance -- everything is 100% fog.
            glUniform1f(fog_start_loc, -1.0);
            glUniform1f(fog_rcp_loc, 1.0);
        }
    }
    glUniform4f(fog_col_loc, c.r, c.g, c.b, 1.0);
}

std::array<real_t, 16> Display::get_matrix(int mat)
{
    const glm::mat4& matrix = g_matrices[mat];
    std::array<real_t, 16> arr;
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            arr[i + 4*j] = matrix[i][j];
        }
    }
    return arr;
}

void Display::set_matrix(int mat, std::array<real_t, 16> arr)
{
    glm::mat4& matrix = g_matrices[mat];
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            matrix[i][j] = arr[i + 4*j];
        }
    }

    update_camera_matrices();
}

std::array<real_t, 16> Display::get_matrix_view()
{
    return get_matrix(MATRIX_VIEW);
}

std::array<real_t, 16> Display::get_matrix_projection()
{
    return get_matrix(MATRIX_PROJECTION);
}

std::array<real_t, 16> Display::get_matrix_model()
{
    return get_matrix(MATRIX_MODEL);
}

void Display::set_matrix_view(std::array<real_t, 16> array)
{
    set_matrix(MATRIX_VIEW, array);
}

void Display::set_matrix_projection(std::array<real_t, 16> array)
{
    set_matrix(MATRIX_PROJECTION, array);
}

void Display::set_matrix_model(std::array<real_t, 16> array)
{
    set_matrix(MATRIX_MODEL, array);
}

ogm::geometry::Vector<real_t> Display::get_display_dimensions()
{
    // fullscreen (TODO):
    //SDL_GetRendererOutputSize(g_renderer, &w, &h);
    SDL_DisplayMode dm;

    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
         SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
         return { 0, 0 };
    }
    return{ static_cast<real_t>(dm.w), static_cast<real_t>(dm.h) };
}

ogm::geometry::Vector<real_t> Display::get_window_dimensions()
{
    return{
        static_cast<real_t>(g_window_width),
        static_cast<real_t>(g_window_height)
    };
}

ogm::geometry::Vector<real_t> Display::get_window_position()
{
    #ifndef EMSCRIPTEN
    int x, y;
    SDL_GetWindowPosition(g_window, &x, &y);
    return{ static_cast<real_t>(x), static_cast<real_t>(y) };
    #else
    return{ 0, 0 };
    #endif
}

void Display::set_window_position(real_t x, real_t y)
{
    #ifndef EMSCRIPTEN
    SDL_SetWindowPosition(
        g_window,
        static_cast<int>(x),
        static_cast<int>(y)
    );

    #endif
}

void Display::set_window_size(real_t w, real_t h)
{
    #ifndef EMSCRIPTEN
    SDL_SetWindowSize(
        g_window,
        static_cast<int>(w),
        static_cast<int>(h)
    );
    g_window_width = w;
    g_window_height = h;
    glViewport(0, 0, w, h);
    g_viewport_dimensions.x = w;
    g_viewport_dimensions.y = h;
    #endif
}

bool Display::get_joysticks_supported()
{
    return true;
}

size_t Display::get_joystick_max()
{
    return SDL_NumJoysticks();
}

bool Display::get_joystick_connected(size_t index)
{
    SDL_GameController* js = get_joystick_from_index(index);
    return !!js;
}

std::string Display::get_joystick_name(size_t index)
{
    SDL_GameController* js = get_joystick_from_index(index);
    if (js)
    {
        return SDL_GameControllerName(js);
    }
    else
    {
        return "";
    }
}

size_t Display::get_joystick_axis_count(size_t index)
{
    SDL_GameController* js = get_joystick_from_index(index);
    if (!js) return 0;
    SDL_Joystick* _js = SDL_GameControllerGetJoystick(js);
    if (!_js) return 0;
    return SDL_JoystickNumAxes(_js);
}

real_t Display::get_joystick_axis_value(size_t index, size_t axis_index)
{
    SDL_GameController* js = get_joystick_from_index(index);
    if (!js) return 0;
    SDL_Joystick* _js = SDL_GameControllerGetJoystick(js);
    if (!_js) return 0;
    return static_cast<real_t>(
        SDL_JoystickGetAxis(_js, axis_index)
    ) / 32768;
}

size_t Display::get_joystick_button_count(size_t index)
{
    if (g_jsinfo.size() <= index) return 0;
    JSInfo& info = g_jsinfo.at(index);
    return info.m_button_down.size();
}

bool Display::get_joystick_button_down(size_t index, size_t button)
{
    if (g_jsinfo.size() <= index) return 0;
    JSInfo& info = g_jsinfo.at(index);
    if (button >= info.m_button_down.size()) return 0;
    return info.m_button_down.at(button);
}

bool Display::get_joystick_button_pressed(size_t index, size_t button)
{
    if (g_jsinfo.size() <= index) return 0;
    JSInfo& info = g_jsinfo.at(index);
    if (button >= info.m_button_pressed.size()) return 0;
    return info.m_button_pressed.at(button);
}

bool Display::get_joystick_button_released(size_t index, size_t button)
{
    if (g_jsinfo.size() <= index) return 0;
    JSInfo& info = g_jsinfo.at(index);
    if (button >= info.m_button_released.size()) return 0;
    return info.m_button_released.at(button);
}

uint32_t Display::make_vertex_format()
{
    // find or create a vertex format struct
    VertexFormat* vfp = nullptr;
    size_t id = 0;
    for (VertexFormat& vf : g_vertex_formats)
    {
        if (!vf.m_used)
        {
            vfp = & vf;
            break;
        }
        ++id;
    }

    if (!vfp)
    {
        g_vertex_formats.emplace_back();
        vfp = &g_vertex_formats.back();
    }

    // fill out vfp
    vfp->m_used = true;
    vfp->m_done = false;
    vfp->m_attributes.clear();
    vfp->m_size = 0;
    vfp->m_vao = 0;
    return id;
}

void Display::vertex_format_append_attribute(uint32_t id, VertexFormatAttribute attribute)
{
    if (id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    VertexFormat& vfp = g_vertex_formats.at(id);
    if (!vfp.m_used || id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    if (vfp.m_done)
    {
        throw MiscError("vertex format is read-only");
    }

    vfp.m_attributes.push_back(attribute);
    vfp.m_size += attribute.get_size();
}

void Display::vertex_format_finish(uint32_t id)
{
    if (id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    VertexFormat& vfp = g_vertex_formats.at(id);
    if (!vfp.m_used || id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    if (vfp.m_done)
    {
        throw MiscError("vertex format is read-only");
    }

    glGenVertexArrays(1, &vfp.m_vao);

    // mark as read-only.
    vfp.m_done = true;
}

void Display::free_vertex_format(uint32_t id)
{
    if (id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    VertexFormat& vfp = g_vertex_formats.at(id);
    if (!vfp.m_used || id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    vfp.m_attributes.clear();
    if (vfp.m_done)
    {
        glDeleteVertexArrays(1, &vfp.m_vao);
    }

    vfp.m_used = false;
}

uint32_t Display::make_vertex_buffer(size_t size)
{
    // find or create a vertex buffer struct
    VertexBuffer* vbp = nullptr;
    size_t id = 0;
    for (VertexBuffer& vb : g_vertex_buffers)
    {
        if (!vb.m_used)
        {
            vbp = &vb;
            break;
        }
        ++id;
    }

    if (!vbp)
    {
        g_vertex_buffers.emplace_back();
        vbp = &g_vertex_buffers.back();
    }

    // fill out vbp
    vbp->m_used = true;
    vbp->m_state = VertexBuffer::dirty;
    vbp->m_data.clear();
    vbp->m_size = 0;
    glGenBuffers(1, &vbp->m_vbo);
    return id;
}

void Display::add_vertex_buffer_data(uint32_t id, unsigned char* data, size_t length)
{
    if (id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }
    VertexBuffer& vb = g_vertex_buffers.at(id);
    if (!vb.m_used || id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    if (vb.m_state == VertexBuffer::frozen)
    {
        throw MiscError("vertex buffer is read-only.");
    }

    vb.m_state = VertexBuffer::dirty;
    vb.m_data.reserve(vb.m_data.size() + length);
    vb.m_size += length;
    for (size_t i = 0; i < length; ++i)
    {
        vb.m_data.push_back(*(data++));
    }
}

void Display::free_vertex_buffer(uint32_t id)
{
    if (id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }
    VertexBuffer& vb = g_vertex_buffers.at(id);
    if (!vb.m_used || id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    vb.m_data.clear();
    glDeleteBuffers(1, &vb.m_vbo);

    vb.m_used = false;
}

void Display::freeze_vertex_buffer(uint32_t id)
{
    if (id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }
    VertexBuffer& vb = g_vertex_buffers.at(id);
    if (!vb.m_used || id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    glBindBuffer(GL_ARRAY_BUFFER, vb.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vb.m_size, &vb.m_data.front(), GL_STATIC_DRAW);

    vb.m_state = VertexBuffer::frozen;

    // no need for this data anymore.
    vb.m_data.clear();
}

void Display::associate_vertex_buffer_format(uint32_t vb_id, uint32_t vf_id)
{
    if (vb_id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    VertexBuffer& vb = g_vertex_buffers.at(vb_id);
    if (!vb.m_used || vb_id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    if (vf_id >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    VertexFormat& vfp = g_vertex_formats.at(vf_id);

    if (!vfp.m_used)
    {
        throw MiscError("vertex format does not exist");
    }

    glBindVertexArray(vfp.m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vb.m_vbo);
    uintptr_t offset = 0;

    // construct the gl VAO
    for (const VertexFormatAttribute& attribute : vfp.m_attributes)
    {
        uint32_t location = attribute.get_location();
        GLenum type = (attribute.m_type == VertexFormatAttribute::U4) ? GL_UNSIGNED_BYTE : GL_FLOAT;
        uint32_t size = attribute.get_component_count();

        glVertexAttribPointer(location, size, type, GL_FALSE, vfp.m_size, (void*)offset);
        glEnableVertexAttribArray(location);

        offset += attribute.get_size();
    }

    vb.m_format = vf_id;
}

void Display::render_buffer(uint32_t vertex_buffer, TexturePage* texture, uint32_t render_glenum)
{
    if (vertex_buffer >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }
    VertexBuffer& vb = g_vertex_buffers.at(vertex_buffer);
    if (!vb.m_used || vertex_buffer >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    uint32_t vertex_format = vb.m_format;
    VertexFormat& vf = g_vertex_formats.at(vertex_format);
    if (vertex_format >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }
    if (!vf.m_used || vertex_format >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    uint32_t texture_id = g_blank_texture;
    if (texture)
    {
        if (texture->cache())
        {
             texture_id = texture->m_gl_tex;
        }
        else
        {
            throw MiscError("No texture with the given ID exists.");
        }
    }

    glBindVertexArray(vf.m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vb.m_vbo);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // fill array if dirty
    if (vb.m_state == VertexBuffer::dirty)
    {
        glBufferData(GL_ARRAY_BUFFER, vb.m_size, &vb.m_data.front(), GL_STREAM_DRAW);
        vb.m_state = VertexBuffer::clean;
    }

    // sigh...
    // TODO: make this exposed properly
    // (lined up with pr_* definitions presently)
    uint32_t mapped_enum = GL_POINTS;
    switch (render_glenum)
    {
    case 0:
        mapped_enum = GL_POINTS;
        break;
    case 1:
        mapped_enum = GL_LINES;
        break;
    case 2:
        mapped_enum = GL_LINE_STRIP;
        break;
    case 3:
        mapped_enum = GL_TRIANGLES;
        break;
    case 4:
        mapped_enum = GL_TRIANGLE_STRIP;
        break;
    case 5:
        mapped_enum = GL_TRIANGLE_FAN;
        break;
    default:
        // beep boop. I'm a comment.
        break;
    }

    glDrawArrays(mapped_enum, 0, vb.m_size / vf.m_size);
}

size_t Display::vertex_buffer_get_size(uint32_t id)
{
    VertexBuffer& vb = g_vertex_buffers.at(id);
    if (!vb.m_used || id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    return vb.m_size;
}

size_t Display::vertex_buffer_get_count(uint32_t id)
{
    VertexBuffer& vb = g_vertex_buffers.at(id);
    if (!vb.m_used || id >= g_vertex_buffers.size())
    {
        throw MiscError("vertex buffer does not exist");
    }

    if (vb.m_size == 0)
    {
        return 0;
    }

    // calculating the number of vertices requires knowledge of the number of bytes per vertex;
    // we have to consult the vertex format for this.
    // (hopefully) guaranteed to be associated by the library interface.
    ogm_assert(vb.m_format != std::numeric_limits<uint32_t>::max());
    uint32_t vertex_format = vb.m_format;
    VertexFormat& vf = g_vertex_formats.at(vertex_format);
    if (vertex_format >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }
    if (!vf.m_used || vertex_format >= g_vertex_formats.size())
    {
        throw MiscError("vertex format does not exist");
    }

    return vb.m_size / vf.m_size;
}

ogm::geometry::Vector<real_t> Display::get_mouse_coord()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return{ static_cast<double>(x), static_cast<double>(y) };
}

ogm::geometry::Vector<real_t> Display::get_mouse_coord_invm()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    glm::vec4 p {
        static_cast<float>(x)/static_cast<float>(g_window_width)
        , static_cast<float>(y)/static_cast<float>(g_window_height), 0, 1
    };
    p = g_imatrices[MATRIX_VIEW] * p;
    return{ p[0], p[1] };
}

void Display::set_depth_test(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void Display::set_culling(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void Display::set_zwrite(bool enabled)
{
    if (enabled)
    {
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDepthMask(GL_FALSE);
    }
}

void Display::set_colour_mask(bool r, bool g, bool b, bool a)
{
    glColorMask(r, g, b, a);
    g_cm[0] = r;
    g_cm[1] = g;
    g_cm[2] = b;
    g_cm[3] = a;
}

void Display::set_camera(coord_t x1, coord_t y1, coord_t z1, coord_t x2, coord_t y2, coord_t z2, coord_t xup, coord_t yup, coord_t zup, real_t fov, real_t aspect, coord_t near, coord_t far)
{
    // set the perspective matrix
    {
        glm::mat4& projection = g_matrices[MATRIX_PROJECTION];
        projection = glm::perspectiveFov<float>(
            fov,
            aspect,
            1,
            near,
            far
        );

        if (g_flip_projection)
        {
            g_matrices[MATRIX_PROJECTION] =
                glm::scale(glm::mat4(1.0), {1, -1, 1})
                * g_matrices[MATRIX_PROJECTION];
        }
    }

    // set the view (camera) matrix
    // https://math.stackexchange.com/a/476311
    {
        glm::mat4& view = g_matrices[MATRIX_VIEW];
        view = glm::mat4(1.0f);

        glm::vec3 src(x1, y1, z1);
        glm::vec3 dst(x2, y2, z2);
        glm::vec3 up(xup, yup, zup);
        up = glm::normalize(up);
        glm::vec3 from = glm::normalize(dst - src);
        glm::vec3 to{0, 0, 1};

        // for stability reasons, rotate to nearest of {(0, 0, 1), (0, 0, -1)}
        bool flipped = false;

        // this has a bug in it somewhere, so we're disabling it.
        #if 0
        if (glm::dot(from, to) < 0)
        {
            to = glm::vec3{0, 0, -1};
            flipped = true;
        }
        #endif

        glm::vec3 ftcross = glm::cross(from, to);
        if (ftcross.x != 0 || ftcross.y != 0 || ftcross.z != 0)
        {
            glm::vec3 normal = glm::normalize(ftcross);
            real_t cos_theta = glm::dot(from, to);

            float skewf[16] = {
                 0,          -ftcross[2], +ftcross[1], 0,
                +ftcross[2],  0,          -ftcross[0], 0,
                -ftcross[1], +ftcross[0],  0,          0,
                 0,           0,           0,          0
            };
            float scale;
            scale =  1.0 / (1 - cos_theta);
            glm::mat4 skew = glm::make_mat4(
                skewf
            );
            view += skew;
            view += glm::scale(skew * skew, glm::vec3{scale, scale, scale});
        }

        if (flipped)
        {
            // unflip
            glm::mat4 unflip = glm::rotate<float>(
                glm::mat4(1.0f),
                TAU / 2.0,
                glm::vec3{0, 1, 0}
            );
            view = unflip * view;
        }

        // see how the 'up' vector is affected by this transformation
        glm::vec4 up4{up.x, up.y, up.z, 1};
        glm::vec4 upv4 = view * up4;
        if (upv4[0] == 0 && upv4[1] == 0)
        {
            // up vector aligned with camera -- panic.
            view = glm::mat4(0.0f);
            return;
        }

        // counter out rotation of up vector
        real_t upangle = std::atan2(upv4[1], upv4[0]) + TAU / 4.0;
        glm::mat4 rotation(1.0f);
        {
            rotation = glm::rotate<float>(rotation, -upangle, to);
        }
        view = rotation * view;
        view = glm::translate(view, -src);
    }

    // propagate changes
    update_camera_matrices();
}

void Display::set_camera_ortho(coord_t x, coord_t y, coord_t w, coord_t h, coord_t angle)
{
    // set the perspective matrix
    {
        glm::mat4& projection = g_matrices[MATRIX_PROJECTION];
        projection = glm::mat4(1.0);
        projection = glm::translate(projection, {-1, 1, 0.0});
        projection = glm::scale(projection, {
            2.0,
            2.0 * (g_flip_projection ? -1 : 1),
            0.000001
        });


        // remove w-scaling:
        projection[0][3] = 0.0;
        projection[1][3] = 0.0;
        projection[2][3] = 0.0;
        projection[3][3] = 1.0;
    }

    // set the perspective matrix
    {
        glm::mat4& view = g_matrices[MATRIX_VIEW];
        view = glm::mat4(1.0f);
        view = glm::scale(view, {1.0/w, 1.0/h, 0});
        view = glm::translate(view, {-x, -y, 1.0});
    }

    // propagate changes
    update_camera_matrices();
}

void Display::set_target(TexturePage* page)
{
    glCheckError();
    if (page == nullptr)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        g_gl_framebuffer = 0;
        g_target = nullptr;
        glViewport(0, 0, g_window_width, g_window_height);
        g_viewport_dimensions.x = g_window_width;
        g_viewport_dimensions.y = g_window_height;
        g_flip_projection = true;
    }
    else
    {
        if (!page->m_gl_framebuffer)
        {
            throw MiscError("Render target has no framebuffer. (Is it a surface?)");
        }
        g_target = page;
        glBindFramebuffer(GL_FRAMEBUFFER, page->m_gl_framebuffer);
        g_gl_framebuffer = page->m_gl_framebuffer;
        glViewport(0, 0, page->m_dimensions.x, page->m_dimensions.y);
        g_viewport_dimensions.x = page->m_dimensions.x;
        g_viewport_dimensions.y = page->m_dimensions.y;
        g_flip_projection = false;
    }
    glCheckError();
}

void Display::render_array(size_t length, float* vertex_data, TexturePage* texture, uint32_t render_glenum)
{
    uint32_t tex = g_blank_texture;
    if (texture && texture->cache())
    {
        tex = texture->m_gl_tex;
    }
    render_vertices(vertex_data, length / k_vertex_data_size, tex, render_glenum);
}

void Display::transform_identity()
{
    g_matrices[MATRIX_MODEL] = glm::mat4(1.0f);
    update_camera_matrices();
}

void Display::transform_apply(std::array<float, 16> matrix)
{
    // trasnpose
    glm::mat4 a = glm::make_mat4(
        &matrix.at(0)
    );
    g_matrices[MATRIX_MODEL] = a * g_matrices[MATRIX_MODEL];
    update_camera_matrices();
}

void Display::transform_apply_rotation(coord_t angle, coord_t x, coord_t y, coord_t z)
{
    glm::mat4 a = glm::rotate<float>(
        glm::mat4(1.0f),
        angle,
        glm::vec3{x, y, z}
    );
    g_matrices[MATRIX_MODEL] = a * g_matrices[MATRIX_MODEL];
    update_camera_matrices();
}

void Display::transform_apply_translation(coord_t x, coord_t y, coord_t z)
{
    glm::mat4 a = glm::translate<float>(
        glm::mat4(1.0f),
        glm::vec3{x, y, z}
    );
    g_matrices[MATRIX_MODEL] = a * g_matrices[MATRIX_MODEL];
    update_camera_matrices();
}

void Display::transform_stack_clear()
{
    g_transform_stack.clear();
    g_transform_stack.reserve(k_transform_stack_size);
}

bool Display::transform_stack_empty()
{
    return g_transform_stack.empty();
}

bool Display::transform_stack_push()
{
    if (g_transform_stack.size() >= k_transform_stack_size)
    {
        return false;
    }
    g_transform_stack.push_back(g_matrices[MATRIX_MODEL]);
    return true;
}

bool Display::transform_stack_pop()
{
    if (g_transform_stack.empty())
    {
        return false;
    }
    g_matrices[MATRIX_MODEL] = g_transform_stack.back();
    g_transform_stack.pop_back();
    update_camera_matrices();
    return true;
}

bool Display::transform_stack_top()
{
    if (g_transform_stack.empty())
    {
        return false;
    }
    g_matrices[MATRIX_MODEL] = g_transform_stack.back();
    update_camera_matrices();
    return true;
}

bool Display::transform_stack_discard()
{
    if (g_transform_stack.empty())
    {
        return false;
    }
    g_transform_stack.pop_back();
    return true;
}

void Display::transform_vertex(std::array<real_t, 3>& vertex)
{
    glm::vec4 v{vertex[0], vertex[1], vertex[2], 1};
    v = g_matrices[MATRIX_MODEL] * v;
    vertex[0] = v.x;
    vertex[1] = v.y;
    vertex[2] = v.z;
}

void Display::transform_vertex_mv(std::array<real_t, 3>& vertex)
{
    glm::vec4 v{vertex[0], vertex[1], vertex[2], 1};
    v = g_matrices[MATRIX_MODEL_VIEW] * v;
    vertex[0] = v.x;
    vertex[1] = v.y;
    vertex[2] = v.z;
}

void Display::transform_vertex_mvp(std::array<real_t, 3>& vertex)
{
    glm::vec4 v{vertex[0], vertex[1], vertex[2], 1};
    v = g_matrices[MATRIX_MODEL_VIEW_PROJECTION] * v;
    vertex[0] = v.x;
    vertex[1] = v.y;
    vertex[2] = v.z;
}

template<bool write>
void Display::serialize(typename state_stream<write>::state_stream_t& s)
{
    // some things are difficult to serialize, so they haven't bee implemented yet.
    // but some things can be serialized, so we do that.
    _serialize<write>(s, g_enable_view_projection);
    _serialize<write>(s, g_viewport_dimensions);
    _serialize<write>(s, g_flip_projection);
    _serialize<write>(s, g_circle_precision);
    _serialize<write>(s, g_draw_colour);

    // we don't serialize keystates or joysticks yet. (Not sure if we should.)

    // update opengl state:
    if (!write)
    {
        glViewport(0, 0, g_viewport_dimensions.x, g_viewport_dimensions.y);
    }

    // don't serialize nor update projection matrices/transform stack,
    // as that is typically done at the beginning of the frame anyway.
    // (This does assume a typical main loop.)

    // we don't serialize the vertex formats or vertex buffers -- but maybe we
    // should...
}

template
void Display::serialize<false>(typename state_stream<false>::state_stream_t& s);

template
void Display::serialize<true>(typename state_stream<true>::state_stream_t& s);

void Display::bind_and_compile_shader(asset_index_t asset_index, const std::string& vertex_source, const std::string& fragment_source)
{
    uint32_t shader;
    if (!compile_shader(vertex_source, fragment_source, shader))
    {
        g_shader_programs[asset_index] = shader;
    }
}

void Display::use_shader(asset_index_t asset_index)
{
    auto iter = g_shader_programs.find(asset_index);
    if (iter == g_shader_programs.end())
    {
        glUseProgram(g_shader_program);
    }
    else
    {
        glUseProgram(iter->second);
    }
}

int32_t Display::shader_get_uniform_id(asset_index_t asset_index, const std::string& handle)
{
    auto iter = g_shader_programs.find(asset_index);
    if (iter == g_shader_programs.end())
    {
        return -2;
    }
    else
    {
        int32_t loc = glGetUniformLocation(iter->second, handle.c_str());
        return loc;
    }
}

void Display::shader_set_uniform_f(int32_t uniform_id, int c, float* v)
{
    glCheckError();
    if (uniform_id >= 0 && c >= 1)
    {
        switch (c)
        {
        case 1:
            glUniform1f(uniform_id, v[0]);
            break;
        case 2:
            glUniform2f(uniform_id, v[0], v[1]);
            break;
        case 3:
            glUniform3f(uniform_id, v[0], v[1], v[2]);
            break;
        case 4:
        default:
            glUniform4f(uniform_id, v[0], v[1], v[2], v[3]);
            break;
        }
    }
    glCheckErrorStr(("set uniform f (uniform_id=" + std::to_string(uniform_id) + ", c=" + std::to_string(c) + ")").c_str());
}

void Display::check_error(const std::string& text)
{
    glCheckErrorStr(text.c_str());
}

void Display::set_multisample(uint32_t n_samples)
{
    if (n_samples == 0)
    {
        glDisable(GL_MULTISAMPLE);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, n_samples);
        glEnable(GL_MULTISAMPLE);
    }
}

namespace
{
    Model& get_model(model_id_t id)
    {
        if (id >= g_models.size() || !g_models.at(id).m_used)
        {
            throw MiscError("Model does not exist.");
        }
        return g_models.at(id);
    }
}

model_id_t Display::model_make()
{
    model_id_t id;
    for (id = 0; id < g_models.size(); id++)
    {
        if (!g_models.at(id).m_used)
        {
            goto initialize_model;
        }
    }
    id = g_models.size();
    g_models.emplace_back();

initialize_model:
    Model& m = g_models.at(id);
    m.m_used = true;
    m.m_format = make_vertex_format();
    // initialize vertex format.
    // TODO: merge this with the initialization routine in init somehow. (DRY)

    // position (3 floats)
    vertex_format_append_attribute(
        m.m_format,
        {
            VertexFormatAttribute::F3,
            VertexFormatAttribute::POSITION
        }
    );

    // colour (rgba)
    vertex_format_append_attribute(
        m.m_format,
        {
            VertexFormatAttribute::RGBA,
            VertexFormatAttribute::COLOUR
        }
    );

    // coordinate
    vertex_format_append_attribute(
        m.m_format,
        {
            VertexFormatAttribute::F2,
            VertexFormatAttribute::TEXCOORD
        }
    );

    vertex_format_finish(m.m_format);
    return id;
}

void Display::model_add_vertex_buffer(model_id_t id, uint32_t buffer, uint32_t render_glenum)
{
    Model& m = get_model(id);
    m.m_buffers.emplace_back(buffer, render_glenum);
}

void Display::model_draw(model_id_t id, TexturePage* image)
{
    Model& m = get_model(id);

    for (auto& [buffer, glenum] : m.m_buffers)
    {
        render_buffer(buffer, image, glenum);
    }
}

uint32_t Display::model_get_vertex_format(model_id_t id)
{
    Model& m = get_model(id);

    return m.m_format;
}

void Display::model_free(model_id_t id)
{
    Model& model = g_models.at(id);
    model.m_used = false;
    for (auto& [id, glenum] : model.m_buffers)
    {
        free_vertex_buffer(id);
    }

    free_vertex_format(model.m_format);

    model.m_buffers.clear();
}

}}

#else

namespace ogm { namespace interpreter {

bool Display::start(uint32_t width, uint32_t height, const char* caption)
{
    return true;
}

Display::~Display()
{ }

void Display::draw_image(TextureView*, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2) {}
void Display::draw_image(TextureView*, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4) {}
void Display::draw_image_tiled(TextureView*, bool tiled_x, bool tiled_y, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2) {}

void Display::flip()
{ }

void Display::begin_render()
{ }

void Display::end_render()
{ }

bool Display::window_close_requested()
{
    return false;
}

void Display::set_clear_colour(uint32_t z)
{ }

uint32_t Display::get_clear_colour()
{
    return 0;
}

uint32_t Display::get_colour()
{
    return 0;
}

void Display::set_colour(uint32_t c)
{ }

float Display::get_alpha()
{
    return 0;
}

void Display::set_alpha(float a)
{ }

void Display::get_colours4(uint32_t* colours)
{ }

void Display::set_colours4(uint32_t colours[4])
{ }

void Display::set_font(asset::AssetFont*, TextureView* tv)
{ }

void Display::draw_filled_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{ }

void Display::draw_fill_colour(uint32_t colour)
{ }

void Display::clear_keys()
{ }

void Display::process_keys()
{ }

bool Display::get_key_down(ogm_keycode_t i)
{
    return false;
}

bool Display::get_key_pressed(ogm_keycode_t i)
{
    return false;
}

bool Display::get_key_released(ogm_keycode_t i)
{
    return false;
}

void Display::set_matrix_view(coord_t x1, coord_t y1, coord_t x2, coord_t y2, real_t angle)
{ }

void Display::set_matrix_model(coord_t x, coord_t y, coord_t xscale, coord_t yscale, real_t angle)
{ }

void Display::update_camera_matrices()
{ }

ogm::geometry::Vector<real_t> Display::get_display_dimensions()
{
    return{ 0, 0 };
}

ogm::geometry::Vector<real_t> Display::get_window_dimensions()
{
    return{ 0, 0 };
}

ogm::geometry::Vector<real_t> Display::get_window_position()
{
    return{ 0, 0 };
}

void Display::set_window_position(real_t x, real_t y)
{ }

void Display::set_window_size(real_t w, real_t h)
{ }

void Display::draw_filled_circle(coord_t x, coord_t y, coord_t r)
{ }

void Display::set_circle_precision(uint32_t prec)
{ }

uint32_t Display::get_circle_precision()
{
    return 0;
}

bool Display::get_joysticks_supported()
{
    return false;
}

void Display::set_fog(bool enabled, real_t start, real_t end, uint32_t col)
{ }

void Display::draw_outline_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{ }

void Display::draw_outline_circle(coord_t x, coord_t y, coord_t r)
{ }

size_t Display::get_joystick_max()
{
    return 0;
}

bool Display::get_joystick_connected(size_t index)
{
    return 0;
}

std::string Display::get_joystick_name(size_t index)
{
    return 0;
}

size_t Display::get_joystick_axis_count(size_t index)
{
    return 0;
}

real_t Display::get_joystick_axis_value(size_t index, size_t axis_index)
{
    return 0;
}

ogm::geometry::AABB<coord_t> Display::get_viewable_aabb()
{
    return { 0, 0, 1, 1 };
}

void Display::draw_text(coord_t x, coord_t y, const char* text, real_t halign, real_t valign, bool use_max_width, coord_t max_width, bool use_sep, coord_t sep)
{ }

uint32_t Display::make_vertex_format()
{
    return 0;
}
uint32_t Display::make_vertex_buffer(size_t size)
{
    return 0;
}

void Display::vertex_format_append_attribute(uint32_t id, VertexFormatAttribute attribute)
{ }

void Display::vertex_format_finish(uint32_t id)
{ }

void Display::free_vertex_format(uint32_t id)
{ }

void Display::free_vertex_buffer(uint32_t id)
{ }

void Display::add_vertex_buffer_data(uint32_t id, unsigned char* data, size_t length)
{ }

void Display::freeze_vertex_buffer(uint32_t id)
{ }

void Display::render_buffer(uint32_t vertex_buffer, TexturePage* image, uint32_t render_glenum)
{ }

size_t Display::vertex_buffer_get_size(uint32_t id)
{
    return 0;
}

size_t Display::vertex_buffer_get_count(uint32_t id)
{
    return 0;
}

void Display::associate_vertex_buffer_format(uint32_t vb, uint32_t vf)
{ }

ogm::geometry::Vector<real_t> Display::get_mouse_coord()
{
    return {0, 0};
}

ogm::geometry::Vector<real_t> Display::get_mouse_coord_invm()
{
    return {0, 0};
}

std::array<real_t, 16> Display::get_matrix_view()
{
    return{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

std::array<real_t, 16> Display::get_matrix_projection()
{
    return{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

std::array<real_t, 16> Display::get_matrix_model()
{
    return{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

void Display::set_matrix_view(std::array<real_t, 16> array)
{ }

void Display::set_matrix_projection(std::array<real_t, 16> array)
{ }

void Display::set_matrix_model(std::array<real_t, 16> array)
{ }

void Display::set_depth_test(bool)
{ }

void Display::set_culling(bool)
{ }

void Display::set_zwrite(bool)
{ }

void Display::set_camera(coord_t x1, coord_t y1, coord_t z1, coord_t x2, coord_t y2, coord_t z2, coord_t xup, coord_t yup, coord_t zup, real_t fov, real_t aspect, coord_t near, coord_t far)
{ }

void Display::set_camera_ortho(coord_t x, coord_t y, coord_t w, coord_t h, coord_t angle)
{ }

void Display::set_target(TexturePage*)
{ }

void Display::render_array(size_t length, float* vertex_data, TexturePage* texture, uint32_t render_glenum)
{ }

void Display::transform_identity()
{ }

void Display::transform_apply(std::array<float, 16> matrix)
{ }

void Display::transform_apply_rotation(coord_t angle, coord_t x, coord_t y, coord_t z)
{ }

void Display::transform_apply_translation(coord_t x, coord_t y, coord_t z)
{ }

void Display::transform_stack_clear()
{ }

bool Display::transform_stack_empty()
{
    return false;
}

bool Display::transform_stack_push()
{
    return false;
}

bool Display::transform_stack_pop()
{
    return false;
}

bool Display::transform_stack_top()
{
    return false;
}

bool Display::transform_stack_discard()
{
    return false;
}

void Display::transform_vertex(std::array<real_t, 3>& vertex)
{ }

void Display::transform_vertex_mv(std::array<real_t, 3>& vertex)
{ }

void Display::transform_vertex_mvp(std::array<real_t, 3>& vertex)
{ }


void Display::clear_render()
{ }

void Display::set_colour_mask(bool r, bool g, bool b, bool a)
{ }

void Display::enable_view_projection(bool)
{ }

template<bool write>
void Display::serialize(typename state_stream<write>::state_stream_t& s)
{ }

template
void Display::serialize<false>(typename state_stream<false>::state_stream_t& s);

template
void Display::serialize<true>(typename state_stream<true>::state_stream_t& s);

void Display::set_matrix_pre_model(coord_t x, coord_t y, coord_t xscale, coord_t yscale, real_t angle)
{ }

void Display::set_matrix_pre_model()
{ }

void Display::set_matrix_projection()
{ }

void Display::set_blendmode(int32_t src, int32_t dst)
{ }

void Display::shader_set_alpha_test_enabled(bool enabled)
{ }

void Display::shader_set_alpha_test_threshold(real_t value)
{ }

void Display::set_blending_enabled(bool c)
{ }

void Display::disable_scissor()
{ }

void Display::enable_scissor(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{ }

bool Display::get_joystick_button_down(size_t index, size_t button)
{
    return false;
}

size_t Display::get_joystick_button_count(size_t index)
{
    return 0;
}

bool Display::get_joystick_button_pressed(size_t index, size_t button)
{
    return 0;
}

bool Display::get_joystick_button_released(size_t index, size_t button)
{
    return 0;
}

ogm_keycode_t Display::get_current_key()
{
    return 0;
}

bool Display::get_key_direct(ogm_keycode_t i)
{
    return false;
}

model_id_t Display::model_make()
{
    return 0;
}

void Display::model_add_vertex_buffer(model_id_t, uint32_t buffer, uint32_t render_glenum)
{ }

void Display::model_draw(model_id_t id, TexturePage* image)
{ }

uint32_t Display::model_get_vertex_format(model_id_t id)
{
    return 0;
}

void Display::model_free(model_id_t)
{ }

void Display::bind_and_compile_shader(asset_index_t, const std::string& vertex_source, const std::string& fragment_source)
{ }

void Display::use_shader(asset_index_t)
{ }

int32_t Display::shader_get_uniform_id(asset_index_t asset_index, const std::string& handle)
{
    return 0;
}

void Display::shader_set_uniform_f(int32_t uniform_id, int c, float* v)
{ }

void Display::check_error(const std::string& text)
{ }

void Display::set_multisample(uint32_t n_samples)
{ }

}}

#endif
