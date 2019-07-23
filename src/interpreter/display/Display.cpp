#include "ogm/interpreter/display/Display.hpp"

#ifdef GFX_AVAILABLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace ogm;

namespace ogmi {

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

    size_t glfw_reference_count=0;
    Display* g_active_display = nullptr;
    uint32_t g_window_width=0;
    uint32_t g_window_height=0;
    GLFWwindow* g_window=nullptr;
    uint32_t g_square_vbo;
    uint32_t g_square_vao;
    uint32_t g_vertex_shader;
    uint32_t g_geometry_shader;
    uint32_t g_fragment_shader;
    uint32_t g_shader_program;
    uint32_t g_blank_texture;
    uint32_t g_circle_precision=32;
    colour4 g_clear_colour{0, 0, 0, 1};
    colour4 g_draw_colour[4] = {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};

    bool init_glfw = false;
    bool init_glew = false;
    bool init_buffers = false;
    bool init_il = false;


    colour3 bgrz_to_colour3(uint32_t bgr)
    {
        return
        {
            (bgr & 0xff) / 255.0,
            ((bgr & 0xff00) >> 8) / 255.0,
            ((bgr & 0xff0000) >> 16) / 255.0
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
            ((bgr & 0xff00) >> 8) / 255.0,
            ((bgr & 0xff0000) >> 16) / 255.0,
            ((bgr & 0xff000000) >> 24) / 255.0,
            ((bgr & 0xff)) / 255.0
        };
    }

    inline void colour4_to_floats(float* dst, colour4 c)
    {
        dst[0] = c.r;
        dst[1] = c.g;
        dst[2] = c.b;
        dst[3] = c.a;
    }

    // fills the given pointer with 3 floats
    inline void floats3(float* dst, coord_t x=0, coord_t y=0, coord_t z=0)
    {
        dst[0] = x;
        dst[1] = y;
        dst[2] = z;
    }

    #define CONST(x, y) constexpr ogm_keycode_t x = y;
    #include "../library/fn_keycodes.h"

    ogm_keycode_t glfw_to_ogm_keycode(int key)
    {
        // A-Z
        if (key >= 65 && key <= 90)
        {
            return key;
        }

        // 0-9
        if (key >= 48 && key <= 57)
        {
            return key;
        }

        // Fn keys
        if (key >= 290 && key <= 301)
        {
            return key + 112 - 290;
        }

        switch (key)
        // TODO: finish this
        // keycodes: https://www.glfw.org/docs/latest/group__keys.html
        {
        case 32:
            return 32;
        case 256:
            return vk_escape;
        case 257:
            return vk_enter;
        case 259:
            return vk_backspace;
        case 262:
            return vk_right;
        case 263:
            return vk_left;
        case 264:
            return vk_down;
        case 265:
            return vk_up;
        case 340:
        case 344:
            return vk_shift;
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

    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        ogm_keycode_t ogm_keycode = glfw_to_ogm_keycode(key);
        if (action == GLFW_PRESS)
        {
            g_key_pressed[ogm_keycode] = true;
            g_key_down[ogm_keycode] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_key_released[ogm_keycode] = true;
            g_key_down[ogm_keycode] = false;
        }
    }

    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        g_window_width=width;
        g_window_height=height;
    }

    // view matrices
    const int MATRIX_VIEW = 0;
    const int MATRIX_PROJECTION = 1;
    const int MATRIX_MODEL = 2;
    const int MATRIX_MODEL_VIEW = 3;
    const int MATRIX_MODEL_VIEW_PROJECTION = 4;
    const int MATRIX_MAX = 5;
    glm::mat4 g_matrices[MATRIX_MAX];
    
    // inverse of matrices
    // as the use of these are limited, some are not used.
    glm::mat4 g_imatrices[MATRIX_MAX];
}

// defined in shader_def.cpp
extern const char* k_default_vertex_shader_source;
extern const char* k_default_geometry_shader_source;
extern const char* k_default_fragment_shader_source;

// number of floats per vertex
// 3 xyz coordinates, 4 colour coordinates, 2 texture coordinates
const size_t k_vertex_data_size = (3 + 4 + 2);

bool Display::start(uint32_t width, uint32_t height, const char* caption)
{
    if (g_active_display != nullptr)
    {
        throw MiscError("Multiple displays not supported");
    }

    if (!init_glfw)
    {
        if (!glfwInit())
        {
            std::cout << "could not initialize glfw.\n";
            return false;
        }

        init_glfw = true;
    }

    g_active_display = this;

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    g_window = glfwCreateWindow(width, height, caption, nullptr, nullptr);
    if (!g_window)
    {
        std::cout << "could not create window.\n";
        return false;
    }

    auto display_dimensions = get_display_dimensions();
    if (display_dimensions.x != 0)
    {
        set_window_position(display_dimensions.x/2 - width/2, display_dimensions.y/2 - height/2);
    }

    glfwSetKeyCallback(g_window, key_callback);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
    glfwSwapInterval(0);
    glfwMakeContextCurrent(g_window);

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

    g_window_width = width;
    g_window_height = height;
    begin_render();

    // compile shaders
    int success;
    char infoLog[512];
    g_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(g_vertex_shader, 1, &k_default_vertex_shader_source, nullptr);
    glCompileShader(g_vertex_shader);
    glGetShaderiv(g_vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(g_vertex_shader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    g_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_fragment_shader, 1, &k_default_fragment_shader_source, nullptr);
    glCompileShader(g_fragment_shader);
    glGetShaderiv(g_fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(g_fragment_shader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // set shader program
    g_shader_program = glCreateProgram();
    glAttachShader(g_shader_program, g_vertex_shader);
    glAttachShader(g_shader_program, g_fragment_shader);
    glLinkProgram(g_shader_program);
    glGetProgramiv(g_shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(g_shader_program, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
        return false;
    }
    glUseProgram(g_shader_program);

    // clean up shaders
    glDeleteShader(g_vertex_shader);
    glDeleteShader(g_fragment_shader);

    // set up buffer
    {
        glGenVertexArrays(1, &g_square_vao);
        glBindVertexArray(g_square_vao);

        glGenBuffers(1, &g_square_vbo);

        glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
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

    // enable alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );

    // generate the basic "blank" texture
    blank_image();

    // disable distance fog
    set_fog(false);

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

    if (init_glfw)
    {
        glfwDestroyWindow(g_window);
        glfwTerminate();

        init_glfw = false;
    }

    g_active_display = nullptr;
}

void Display::bind_asset(ImageDescriptor descriptor, std::string image_path)
{
    m_descriptor_path[descriptor] = native_path(image_path);
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

bool Display::cache_image(ImageDescriptor descriptor)
{
    if (m_descriptor_texture.find(descriptor) == m_descriptor_texture.end() || m_descriptor_texture.at(descriptor) == 0)
    {
        std::string image_path = m_descriptor_path[descriptor];
        //std::cout << "Caching image \"" + image_path + "\"\n";
        // TODO
        int width, height, channel_count;
        unsigned char* data = stbi_load(image_path.c_str(), &width, &height, &channel_count, 4);

        if (!data)
        {
            std::cout << "  - Failed to cache image \"" + image_path + "\"\n";
            return false;
        }

        //std::cout << "    loaded image " << width << "x" << height << ", " << channel_count << " channels.\n";
        uint32_t texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_descriptor_texture[descriptor] = texture;
        glTexImage2D(
            GL_TEXTURE_2D,
            0, // mipmap level
            GL_RGBA, // texture format
            width, height,
            0,
            (channel_count == 4) ? GL_RGBA : GL_RGB, // source format
            GL_UNSIGNED_BYTE, // source format
            data // image data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        // TODO: LRU cache free.
    }
    return true;
}

void Display::render_vertices(float* vertices, size_t count, uint32_t texture, uint32_t render_glenum)
{
    glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * k_vertex_data_size, vertices, GL_STREAM_DRAW);
    glBindVertexArray(g_square_vao);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(render_glenum, 0, count);
}

void Display::draw_image_tiled(ImageDescriptor descriptor, bool tiled_x, bool tiled_y, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2)
{
    double width = x2 - x1;
    double height = y2 - y1;
    if (cache_image(descriptor))
    {
        // construct a mesh of sufficient size to cover the view.
        
        // determine extent of view
        size_t offset_minx;
        size_t offset_maxx;
        size_t offset_miny;
        size_t offset_maxy;
        {
            glm::vec4 p[4] = {
                { 1, 1, 0, 0 },
                { -1, 1, 0, 0 },
                { -1, -1, 0, 0 },
                { -1, 1, 0, 0 }
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
        
        const size_t vertex_count = (offset_maxx - offset_minx) * (offset_maxy - offset_miny);
        float* vertices = new float[vertex_count];
        for (int32_t i = offset_minx; i < offset_maxx; ++i)
        {
            for (int32_t j = offset_miny; j < offset_maxy; ++j)
            {
                auto p = offset_to_world(i, j);
                floats3(vertices + 0*k_vertex_data_size, p.x, p.y);
                floats3(vertices + 1*k_vertex_data_size, p.x, p.y + height);
                floats3(vertices + 2*k_vertex_data_size, p.x + width, p.y);
                floats3(vertices + 3*k_vertex_data_size, p.x, p.y + height);
                floats3(vertices + 4*k_vertex_data_size, p.x + width, p.y);
                floats3(vertices + 5*k_vertex_data_size, p.x + width, p.y + height);
                
                colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
                colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
                colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
                colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[1]);
                colour4_to_floats(vertices + 4*k_vertex_data_size + 3, g_draw_colour[2]);
                colour4_to_floats(vertices + 5*k_vertex_data_size + 3, g_draw_colour[3]);
                
                vertices[0*k_vertex_data_size + 7] = tx1;
                vertices[0*k_vertex_data_size + 8] = ty1;

                vertices[1*k_vertex_data_size + 7] = tx1;
                vertices[1*k_vertex_data_size + 8] = ty2;

                vertices[2*k_vertex_data_size + 7] = tx2;
                vertices[2*k_vertex_data_size + 8] = ty1;
                
                vertices[3*k_vertex_data_size + 7] = tx1;
                vertices[3*k_vertex_data_size + 8] = ty2;

                vertices[4*k_vertex_data_size + 7] = tx2;
                vertices[4*k_vertex_data_size + 8] = ty1;

                vertices[5*k_vertex_data_size + 7] = tx2;
                vertices[5*k_vertex_data_size + 8] = ty2;
                
                // TODO: switch to trianglestrip for more efficient vertex packing.
                vertices += k_vertex_data_size * 6;
            }
        }
        
        render_vertices(vertices, 4, m_descriptor_texture.at(descriptor), GL_TRIANGLES);
        delete[] vertices;
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::draw_image(ImageDescriptor descriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2)
{
    if (cache_image(descriptor))
    {
        // set render data

        float vertices[k_vertex_data_size * 4];
        floats3(vertices + 0*k_vertex_data_size, x1, y1);
        floats3(vertices + 1*k_vertex_data_size, x2, y1);
        floats3(vertices + 2*k_vertex_data_size, x1, y2);
        floats3(vertices + 3*k_vertex_data_size, x2, y2);

        // colour data
        colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
        colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
        colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
        colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[3]);

        // texture coordinates
        vertices[0*k_vertex_data_size + 7] = tx1;
        vertices[0*k_vertex_data_size + 8] = ty1;

        vertices[1*k_vertex_data_size + 7] = tx2;
        vertices[1*k_vertex_data_size + 8] = ty1;

        vertices[2*k_vertex_data_size + 7] = tx1;
        vertices[2*k_vertex_data_size + 8] = ty2;

        vertices[3*k_vertex_data_size + 7] = tx2;
        vertices[3*k_vertex_data_size + 8] = ty2;

        // how to interpret `vertices`
        render_vertices(vertices, 4, m_descriptor_texture.at(descriptor), GL_TRIANGLE_STRIP);
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::draw_image(ImageDescriptor descriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4)
{
    if (cache_image(descriptor))
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
        vertices[0*k_vertex_data_size + 7] = tx1;
        vertices[0*k_vertex_data_size + 8] = ty1;

        vertices[1*k_vertex_data_size + 7] = tx2;
        vertices[1*k_vertex_data_size + 8] = ty2;

        vertices[2*k_vertex_data_size + 7] = tx3;
        vertices[2*k_vertex_data_size + 8] = ty3;

        vertices[3*k_vertex_data_size + 7] = tx4;
        vertices[3*k_vertex_data_size + 8] = ty4;

        // how to interpret `vertices`
        render_vertices(vertices, 4, m_descriptor_texture.at(descriptor), GL_TRIANGLE_FAN);
    }
    else
    {
        throw MiscError("Failed to cache image in time for rendering.");
    }
}

void Display::flip()
{
    end_render();
    begin_render();
}

void Display::begin_render()
{
    glViewport(0, 0, g_window_width, g_window_height);

    glClearColor(g_clear_colour.r, g_clear_colour.g, g_clear_colour.b, g_clear_colour.a);
    glClear( GL_COLOR_BUFFER_BIT );

    // quad test
    glBegin( GL_QUADS);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f(0.5f, -0.5f);
        glVertex2f(0.5f, 0.5f);
        glVertex2f(-0.5f, 0.5f);
    glEnd();

    glfwPollEvents();
}

void Display::end_render()
{
    glfwSwapBuffers(g_window);
}

bool Display::window_close_requested()
{
    return glfwWindowShouldClose(g_window);
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

void Display::draw_filled_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{
    // set render data
    float vertices[k_vertex_data_size * 4];
    floats3(vertices + 0*k_vertex_data_size, x1, y1);
    floats3(vertices + 1*k_vertex_data_size, x2, y1);
    floats3(vertices + 2*k_vertex_data_size, x1, y2);
    floats3(vertices + 3*k_vertex_data_size, x2, y2);

    // colour data
    colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[0]);
    colour4_to_floats(vertices + 1*k_vertex_data_size + 3, g_draw_colour[1]);
    colour4_to_floats(vertices + 2*k_vertex_data_size + 3, g_draw_colour[2]);
    colour4_to_floats(vertices + 3*k_vertex_data_size + 3, g_draw_colour[3]);

    // texture coordinates
    vertices[0*k_vertex_data_size + 7] = 0;
    vertices[0*k_vertex_data_size + 8] = 0;

    vertices[1*k_vertex_data_size + 7] = 1;
    vertices[1*k_vertex_data_size + 8] = 0;

    vertices[2*k_vertex_data_size + 7] = 0;
    vertices[2*k_vertex_data_size + 8] = 1;

    vertices[3*k_vertex_data_size + 7] = 1;
    vertices[3*k_vertex_data_size + 8] = 1;

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
    floats3(vertices + 0, x, y);
    colour4_to_floats(vertices + 0*k_vertex_data_size + 3, g_draw_colour[1]);
    vertices[0*k_vertex_data_size + 7] = 0.5;
    vertices[0*k_vertex_data_size + 8] = 0.5;

    for (size_t i = 1; i <= g_circle_precision + 1; ++i)
    {
        float p = static_cast<float>(i)/g_circle_precision;
        float theta = p * 6.283185307;
        float cost = -std::cos(theta);
        float sint = std::sin(theta);
        floats3(vertices + i*k_vertex_data_size, x + r * sint, y + r*cost);
        colour4_to_floats(vertices + i*k_vertex_data_size + 3, g_draw_colour[0]);
        vertices[i*k_vertex_data_size + 7] = sint;
        vertices[i*k_vertex_data_size + 8] = cost;
    }

    render_vertices(vertices, vertex_count, g_blank_texture, GL_TRIANGLE_FAN);
    delete vertices;
}

void Display::draw_outline_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{
    // set render data
    float vertices[k_vertex_data_size * 5];
    floats3(vertices + 0*k_vertex_data_size, x1, y1);
    floats3(vertices + 1*k_vertex_data_size, x2, y1);
    floats3(vertices + 2*k_vertex_data_size, x2, y2);
    floats3(vertices + 3*k_vertex_data_size, x1, y2);
    floats3(vertices + 4*k_vertex_data_size, x1, y1);

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
        floats3(vertices + i*k_vertex_data_size, x + r * sint, y + r*cost);
        colour4_to_floats(vertices + i*k_vertex_data_size + 3, g_draw_colour[0]);
        vertices[i*k_vertex_data_size + 7] = sint;
        vertices[i*k_vertex_data_size + 8] = cost;
    }

    render_vertices(vertices, vertex_count, g_blank_texture, GL_LINE_LOOP);
    delete vertices;
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

void Display::process_keys()
{
    // copy over to new buffer

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

void Display::set_matrix_view(coord_t x1, coord_t y1, coord_t x2, coord_t y2, real_t angle)
{
    glm::mat4& view = g_matrices[MATRIX_VIEW];
    view = glm::mat4(1);
    view = glm::rotate(view, static_cast<float>(angle), glm::vec3(0, 0, -1));
    view = glm::translate(view, glm::vec3(-1, 1, 1));
    view = glm::scale(view, glm::vec3(2, -2, 1));
    view = glm::scale(view, glm::vec3(1/(x2 - x1), 1/(y2 - y1), 1));
    view = glm::translate(view, glm::vec3(-x1, -y1, 0));
    
    // inverse matrix
    glm::mat4& iview = g_imatrices[MATRIX_VIEW];
    iview = glm::mat4(1);
    iview = glm::rotate(iview, -static_cast<float>(angle), glm::vec3(0, 0, -1));
    iview = glm::translate(iview, glm::vec3(1, -1, 1));
    iview = glm::scale(iview, glm::vec3(0.5, -0.5, 1));
    iview = glm::scale(iview, glm::vec3((x2 - x1), (y2 - y1), 1));
    iview = glm::translate(iview, glm::vec3(x1, y1, 0));
    
    update_camera_matrices();
}

void Display::set_matrix_model(coord_t x, coord_t y, coord_t xscale, coord_t yscale, real_t angle)
{
    glm::mat4& view = g_matrices[MATRIX_MODEL];
    view = glm::mat4(1);
    view = glm::translate(view, glm::vec3(x, y, 0));
    view = glm::rotate(view, static_cast<float>(angle), glm::vec3(0, 0, -1));
    view = glm::scale(view, glm::vec3(xscale, yscale, 1));
    
    // inverse matrix?
    // Not actually needed presently.
    
    update_camera_matrices();
}

void Display::update_camera_matrices()
{
    // calculate combined matrices
    g_matrices[MATRIX_MODEL_VIEW] = g_matrices[MATRIX_VIEW] * g_matrices[MATRIX_MODEL];
    g_matrices[MATRIX_MODEL_VIEW_PROJECTION] = g_matrices[MATRIX_PROJECTION] * g_matrices[MATRIX_MODEL_VIEW];
    
    // inverse matrices
    #if 0
    // not actually used anywhere.
    g_imatrices[MATRIX_MODEL_VIEW] = g_imatrices[MATRIX_MODEL] * g_imatrices[MATRIX_VIEW];
    g_imatrices[MATRIX_MODEL_VIEW_PROJECTION] = g_imatrices[MATRIX_MODEL_VIEW] * g_imatrices[MATRIX_PROJECTION];
    #endif
    
    // set uniforms
    for (size_t i = 0; i < 5; ++i)
    {
        uint32_t matvloc = glGetUniformLocation(g_shader_program, "gm_Matrices");
        glUniformMatrix4fv(matvloc + i, 1, GL_FALSE, glm::value_ptr(g_matrices[i]));
    }
}

void Display::set_fog(bool enabled, real_t start, real_t end, uint32_t col)
{
    uint32_t fog_enabledv_loc = glGetUniformLocation(g_shader_program, "gm_VS_FogEnabled");
    uint32_t fog_enabledf_loc = glGetUniformLocation(g_shader_program, "gm_PS_FogEnabled");
    uint32_t fog_start_loc = glGetUniformLocation(g_shader_program, "gm_FogStart");
    uint32_t fog_rcp_loc = glGetUniformLocation(g_shader_program, "gm_RcpFogRange");
    uint32_t fog_col_loc = glGetUniformLocation(g_shader_program, "gm_FogColour");

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

ogm::geometry::Vector<real_t> Display::get_display_dimensions()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        return{ mode->width, mode->height };
    }
    else
    {
        return{ 0, 0 };
    }
}

void Display::set_window_position(real_t x, real_t y)
{
    glfwSetWindowPos(g_window, x, y);
}

void Display::set_window_size(real_t w, real_t h)
{
    glfwSetWindowSize(g_window, w, h);
}

bool Display::get_joysticks_supported()
{
    return true;
}

size_t Display::get_joystick_max()
{
    return GLFW_JOYSTICK_LAST;
}

bool Display::get_joystick_connected(size_t index)
{
    if (index < GLFW_JOYSTICK_LAST)
    {
        return glfwJoystickPresent(index);
    }
    else
    {
        return false;
    }
}

std::string Display::get_joystick_name(size_t index)
{
    if (index < GLFW_JOYSTICK_LAST)
    {
        return glfwGetJoystickName(index);
    }
    else
    {
        return "";
    }
}

size_t Display::get_joystick_axis_count(size_t index)
{
    if (index < GLFW_JOYSTICK_LAST)
    {
        int32_t count;
        glfwGetJoystickAxes(index, &count);
        return count;
    }
    else
    {
        return 0;
    }
}

real_t Display::get_joystick_axis_value(size_t index, size_t axis_index)
{
    if (index < GLFW_JOYSTICK_LAST)
    {
        int32_t count;
        const float* axes = glfwGetJoystickAxes(index, &count);
        if (axis_index < count)
        {
            return axes[axis_index];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

}

#else

namespace ogmi {

bool Display::start(uint32_t width, uint32_t height, const char* caption)
{
    return false;
}

Display::~Display()
{ }

void Display::bind_asset(ImageDescriptor descriptor, std::string image_path)
{ }

bool Display::cache_image(ImageDescriptor descriptor)
{
    return false;
}

void Display::draw_image(ImageDescriptor descriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2)
{ }

void Display::draw_image(ImageDescriptor descriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4)
{ }

void Display::draw_image_tiled(ImageDescriptor, bool tiled_x, bool tiled_y, coord_t x1, coord_t y1, coord_t x2, coord_t y2)
{ }

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
}
#endif
