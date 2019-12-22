

#ifdef GFX_AVAILABLE

#include "ogm/project/Project.hpp"

#include "ogm/geometry/Vector.hpp"
#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/project/resource/ResourceRoom.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "resources/resources.hpp"

#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>

#include <utility>
#include <stdio.h>

namespace ogm::gui
{
    void menu_bar();
    void main_dockspace();
    void resource_windows();
    void resources_pane();
    void set_window_title();
    void create_dummy_texture();
    void handle_hotkeys();
    void fuzzy_finder();
    void open_resource(std::string);

    extern project::Project* g_project;
    extern bool g_refresh_window_title;
    extern SDL_Window* g_window;
    extern std::string g_resource_selected;
    extern geometry::Vector<int> g_window_size;
    extern ImGuiID g_next_id;
    extern ImGuiID g_main_dockspace_id;
    extern GLuint g_dummy_texture;
    extern glm::mat4 g_view;
    extern double g_time;
    extern std::string g_fuzzy_input;
    extern bool g_fuzzy_input_open;
    extern std::map<std::string, bool> g_dirty; // which resources are dirty

    // TODO: get these properly
    static GLuint g_AttribLocationTex = 1;
    static GLuint g_AttribLocationProjMtx = 0;
    static GLuint g_ShaderHandle = 3;

    using namespace project;

    template<typename Resource=project::Resource>
    Resource* get_resource(const std::string& s, ResourceType* rt=nullptr)
    {
        if (!g_project) return nullptr;
        auto iter = g_project->m_resourceTable.find(s);
        if (iter == g_project->m_resourceTable.end())
        {
            return nullptr;
        }
        ResourceTableEntry& rte = g_project->m_resourceTable.at(s);
        if (rt)
        {
            *rt = rte.m_type;
        }
        return dynamic_cast<Resource*>(rte.get());
    }

    // editable framebuffer/texture
    class ImageComponent
    {
    public:
        GLuint m_texture = 0;
        GLuint m_fbo = 0;
        geometry::Vector<int32_t> m_dimensions{ 0, 0 };

        // sets this as the fbo target.
        // supply dimensions to adjust the internal
        // texture.
        void target(geometry::Vector<int32_t> dimensions)
        {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_prev_fbo);
            if (!m_fbo)
            {
                glGenFramebuffers(1, &m_fbo);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

            if (dimensions.x < 1) dimensions.x = 1;
            if (dimensions.y < 1) dimensions.y = 1;
            if (m_texture)
            {
                if (m_dimensions != dimensions)
                {
                    // delete texture
                    std::cout << "recreating texture." << std::endl;
                    glDeleteTextures(1, &m_texture);
                    m_texture = 0;
                }
            }

            if (!m_texture)
            {
                m_dimensions = dimensions;
                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_2D, m_texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
            }

            glViewport(0, 0, dimensions.x, dimensions.y);
            glUseProgram(g_ShaderHandle);
        }

        void button()
        {
            ImGui::ImageButton(
                (void*)(intptr_t)m_texture,
                {
                    static_cast<float>(m_dimensions.x),
                    static_cast<float>(m_dimensions.y)
                },
                { 0, 0 }, {1, 1}, 0
            );
        }

        void untarget()
        {
            // TODO: consider binding just previous draw fbo.
            glBindFramebuffer(GL_FRAMEBUFFER, m_prev_fbo);
            m_prev_fbo = 0;
        }

        ImageComponent()=default;

        ImageComponent(ImageComponent&& other)
            : m_texture(other.m_texture)
            , m_fbo(other.m_fbo)
            , m_dimensions(other.m_dimensions)
        {
            other.m_fbo = 0;
            other.m_texture = 0;
        }

        ImageComponent& operator=(ImageComponent&& other)
        {
            m_texture = other.m_texture;
            m_fbo = other.m_fbo;
            m_dimensions = other.m_dimensions;
            other.m_fbo = 0;
            other.m_texture = 0;

            return *this;
        }

        ~ImageComponent()
        {
            if (m_fbo)
            {
                glDeleteFramebuffers(1, &m_fbo);
            }

            if (m_texture)
            {
                glDeleteTextures(1, &m_texture);
            }

        }

    private:
        GLint m_prev_fbo;
    };


    // state for an open room editor.
    class RoomState
    {
    public:
        float m_splitter_width = 300;
        float m_bg_pane_splitter_height = 256;
        ImageComponent m_gl_component;
        ImageComponent m_pane_component;
        geometry::AABB<coord_t> m_bg_region{ -1, -1, -1, -1 };
        int32_t m_instance_selected = -1;
        int32_t m_tile_selected = -1;
        geometry::Vector<coord_t> m_grid_snap{ 16, 16 };
        enum tab_type {
            INSTANCES,
            TILES,
            OTHER
        } m_tab;

        geometry::Vector<coord_t> m_camera_position{ -32, -32 };
        int32_t m_zoom_amount = 0;

    public:
        double zoom_ratio() const
        {
            return std::exp(m_zoom_amount * 0.15);
        }

        RoomState() = default;

        RoomState(RoomState&& other)
            : m_splitter_width(other.m_splitter_width)
            , m_gl_component(std::move(other.m_gl_component))
            , m_pane_component(std::move(other.m_pane_component))
            , m_bg_region(other.m_bg_region)
        {
        }

        RoomState& operator=(RoomState&& other)
        {
            m_splitter_width = other.m_splitter_width;
            m_gl_component = std::move(other.m_gl_component);
            m_pane_component = std::move(other.m_pane_component);
            m_bg_region = other.m_bg_region;
            return *this;
        }
    };

    // if this is set to INSTANCES or to TILES, changes tab.
    extern RoomState::tab_type g_set_room_tab;
    extern bool g_left_button_pressed;

    // data for an open resource window.
    struct ResourceWindow
    {
        project::ResourceType m_type;
        std::string m_resource_name;
        project::Resource* m_resource;
        bool m_open = true;
        std::string m_id;
        ImGuiWindowClass* m_class;
        RoomState m_room;
        bool m_active=false;

        ResourceWindow(project::ResourceType type, project::Resource* resource, std::string resource_name)
            : m_type(type)
            , m_resource(resource)
            , m_resource_name(resource_name)
            , m_id(std::to_string(reinterpret_cast<uintptr_t>(resource)))
            , m_class(new ImGuiWindowClass())
        {
            m_class->ClassId = g_next_id++;
        }

        ResourceWindow(ResourceWindow&& other)
            : m_type(other.m_type)
            , m_resource(other.m_resource)
            , m_resource_name(std::move(other.m_resource_name))
            , m_open(other.m_open)
            , m_id(other.m_id)
            , m_class(other.m_class)
            , m_room(std::move(other.m_room))
            , m_active(other.m_active)
        {
            other.m_class = nullptr;
        }

        ResourceWindow& operator=(ResourceWindow&& other)
        {
            m_type = other.m_type;
            m_resource = other.m_resource;
            m_open = other.m_open;
            m_id = other.m_id;
            m_class = other.m_class;
            m_room = std::move(other.m_room);
            m_active = other.m_active;
            m_resource_name = std::move(other.m_resource_name);
            other.m_class = nullptr;
            return *this;
        }

        ~ResourceWindow()
        {
            if (m_class)
            {
                delete m_class;
            }
        }
    };

    struct Texture
    {
        asset::Image m_image;

        // used by some textures only:
        geometry::Vector<coord_t> m_offset { 0, 0 };

        Texture(std::string&& path)
            : m_image(std::move(path))
        { }

        Texture(asset::Image&& image)
            : m_image(std::move(image))
        { }

        Texture(const asset::Image& image)
            : m_image(image)
        { }

        Texture(Texture&& other)
            : m_image(std::move(other.m_image))
            , m_gl_tex(other.m_gl_tex)
            , m_offset(other.m_offset)
        {
            other.m_gl_tex = 0;
        }

        geometry::Vector<int32_t> get_dimensions()
        {
            m_image.realize_data();
            return m_image.m_dimensions;
        }

        GLuint get_gl_tex()
        {
            if (m_gl_tex) return m_gl_tex;

            m_image.realize_data();
            glGenTextures(1, &m_gl_tex);
            glBindTexture(GL_TEXTURE_2D, m_gl_tex);
            glTexImage2D(
                GL_TEXTURE_2D,
                0, // mipmap level
                GL_RGBA, // texture format
                m_image.m_dimensions.x,
                m_image.m_dimensions.y,
                0,
                GL_RGBA, // source format
                GL_UNSIGNED_BYTE, // source format
                m_image.m_data // image data
            );
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            return m_gl_tex;
        }

        ~Texture()
        {
            if (m_gl_tex)
            {
                glDeleteTextures(1, &m_gl_tex);
            }
        }

    private:
        GLuint m_gl_tex = 0;
    };

    extern std::vector<ResourceWindow> g_resource_windows;
    typedef int64_t ResourceID;
    extern std::map<ResourceID, Texture> g_texmap;

    void resource_window_room(ResourceWindow&);

    inline uint32_t swapendian(uint32_t a)
    {
        return
              ((a & 0x000000ff) << 24)
            | ((a & 0x0000ff00) << 8)
            | ((a & 0x00ff0000) >> 8)
            | ((a & 0xff000000) << 24);
    }

    inline void colour_int_bgr_to_float3_rgb(int32_t c, float col[3])
    {
        col[0] = static_cast<float>((c & 0xff) / 255.0);
        col[1] = static_cast<float>(((c & 0xff00) >> 8) / 255.0);
        col[2] = static_cast<float>(((c & 0xff0000) >> 16) / 255.0);
    }

    inline int32_t colour_float3_rgb_to_int_bgr(const float col[3])
    {
        uint32_t rz = 255 * col[0];
        if (rz > 255) rz = 255;
        uint32_t gz = 255 * col[1];
        if (gz > 255) gz = 255;
        uint32_t bz = 255 * col[2];
        if (bz > 255) bz = 255;

        return rz | (gz << 8) | (bz << 16);
    }

    void set_dirty(const std::string& resource);

    Texture* get_texture_for_asset_name(std::string asset_name, ResourceID* out_hash=nullptr);
    geometry::Vector<coord_t> get_mouse_position_from_cursor();
    void DrawSplitter(int split_vertically, float thickness, float* size0, float min_size0, float max_size0);
    void set_view_matrix(geometry::AABB<coord_t> view);
    void draw_rect_immediate_tiled(geometry::AABB<coord_t> rect, geometry::AABB<coord_t> fill, GLuint texture=0, int32_t colour=0xffffffff, geometry::AABB<double> uv = {0, 0, 1, 1});
    void draw_rect_immediate(geometry::AABB<coord_t> rect, GLuint texture=0, int32_t colour=0xffffffff, geometry::AABB<double> uv = {0, 0, 1, 1});
}

#endif
