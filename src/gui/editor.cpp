//// source file

#include "ogm/gui/editor.hpp"

#ifdef GFX_AVAILABLE
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

    using namespace project;

    namespace
    {
        // global declarations
        project::Project* g_project = nullptr;
        bool g_refresh_window_title = false;
        SDL_Window* g_window;
        std::string g_resource_selected = "";
        geometry::Vector<int> g_window_size;
        ImGuiID g_next_id = 0;
        ImGuiID g_main_dockspace_id;
        GLuint g_dummy_texture;
        glm::mat4 g_view;
        double g_time = 0.0;

        // TODO: get these properly
        static GLuint g_AttribLocationTex = 1;
        static GLuint g_AttribLocationProjMtx = 0;
        static GLuint g_ShaderHandle = 3;

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
        RoomState::tab_type g_set_room_tab = RoomState::OTHER;
        bool g_left_button_pressed = false;

        // data for an open resource window.
        struct ResourceWindow
        {
            project::ResourceType m_type;
            project::Resource* m_resource;
            bool m_open = true;
            std::string m_id;
            ImGuiWindowClass* m_class;
            RoomState m_room;
            bool m_active=false;

            ResourceWindow(project::ResourceType type, project::Resource* resource)
                : m_type(type)
                , m_resource(resource)
                , m_id(std::to_string(reinterpret_cast<uintptr_t>(resource)))
                , m_class(new ImGuiWindowClass())
            {
                m_class->ClassId = g_next_id++;
            }

            ResourceWindow(ResourceWindow&& other)
                : m_type(other.m_type)
                , m_resource(other.m_resource)
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

        std::vector<ResourceWindow> g_resource_windows;
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

    typedef int64_t ResourceID;
    std::map<ResourceID, Texture> g_texmap;

    Texture* get_texture_embedded(const uint8_t* data, size_t len, ResourceID* out_hash=nullptr)
    {
        ResourceID id = reinterpret_cast<intptr_t>(data);
        if (out_hash)
        {
            *out_hash = id;
        }

        auto iter = g_texmap.find(id);
        if (iter != g_texmap.end())
        {
            // return cached result.
            return &iter->second;
        }
        else
        {
            // load texture from provided pointer.
            asset::Image image;
            image.load_from_memory(data, len);

            auto [iter, success] = g_texmap.emplace(id, std::move(image));
            ogm_assert(success);
            return &iter->second;
        }
    }

    Texture* get_texture_for_asset_name(std::string asset_name, ResourceID* out_hash=nullptr)
    {
        ResourceID id = std::hash<std::string>{}(asset_name);
        if (out_hash)
        {
            *out_hash = id;
        }
        auto iter = g_texmap.find(id);
        if (iter != g_texmap.end())
        {
            return &iter->second;
        }
        else
        {
            Resource* r = get_resource(asset_name);
            if (!r) return nullptr;
            r->load_file();
            bool success;
            if (ResourceBackground* bg = dynamic_cast<ResourceBackground*>(r))
            {
                // associate texture
                std::tie(iter, success) = g_texmap.emplace(id, bg->m_image);
            }
            else if (ResourceSprite* spr = dynamic_cast<ResourceSprite*>(r))
            {
                // associate texture
                ogm_assert(!spr->m_subimages.empty());
                std::tie(iter, success) = g_texmap.emplace(id, spr->m_subimages.front());
                if (!success) return nullptr;
                iter->second.m_offset = spr->m_offset;
            }
            else if (ResourceObject* obj = dynamic_cast<ResourceObject*>(r))
            {
                if (obj->m_sprite_name.length() > 0 && obj->m_sprite_name != "<undefined>")
                {
                    // FIXME: this doesn't cache the result for the object string.
                    return get_texture_for_asset_name(obj->m_sprite_name, out_hash);
                }
                else
                {
                    // FIXME: this doesn't cache the result for the object string.
                    return get_texture_embedded(
                        resource::object_resource_png,
                        resource::object_resource_png_len,
                        out_hash
                    );
                }
            }
            else
            {
                success = false;
            }

            if (!success) return nullptr;

            // cache texture
            Texture& tex = iter->second;
            tex.get_gl_tex();

            // return texture.
            return &tex;
        }
    }

    geometry::Vector<coord_t> get_mouse_position_from_cursor()
    {
        ImVec2 item_position = ImGui::GetCursorScreenPos();
        return
        {
            ImGui::GetMousePos().x - item_position.x,
            ImGui::GetMousePos().y - item_position.y
        };
    }

    // from https://github.com/ocornut/imgui/issues/319#issuecomment-147364392
    void DrawSplitter(int split_vertically, float thickness, float* size0, float min_size0, float max_size0)
    {
        ImVec2 backup_pos = ImGui::GetCursorPos();
        if (split_vertically)
            ImGui::SetCursorPosY(backup_pos.y + *size0);
        else
            ImGui::SetCursorPosX(backup_pos.x + *size0);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));          // We don't draw while active/pressed because as we move the panes the splitter button will be 1 frame late
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f,0.6f,0.6f,0.10f));
        ImGui::Button("##Splitter", ImVec2(!split_vertically ? thickness : -1.0f, split_vertically ? thickness : -1.0f));
        ImGui::PopStyleColor(3);

        ImGui::SetItemAllowOverlap(); // This is to allow having other buttons OVER our splitter.

        if (ImGui::IsItemActive())
        {
            float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;

            // Minimum pane size
            if (mouse_delta < min_size0 - *size0)
                mouse_delta = min_size0 - *size0;
            if (mouse_delta > max_size0 - *size0)
                mouse_delta = max_size0 - *size0;

            // Apply resize
            *size0 += mouse_delta;
        }
        else
        {
            if (*size0 > max_size0)
            {
                *size0 = *size0*0.9 + (max_size0) * 0.1 - 1;
            }
            if (*size0 < min_size0)
            {
                *size0 = min_size0;
            }
        }
        ImGui::SetCursorPos(backup_pos);
    }

    int run(project::Project* project)
    {
        ogm_assert(g_project == nullptr);
        g_refresh_window_title = true;
        g_project = project;
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            return -1;
        }

        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        g_window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        SDL_GLContext gl_context = SDL_GL_CreateContext(g_window);
        SDL_GL_MakeCurrent(g_window, gl_context);
        SDL_GL_SetSwapInterval(1); // Enable vsync

        bool err = glewInit() != GLEW_OK;

        if (err)
        {
           fprintf(stderr, "Failed to initialize OpenGL loader!\n");
           return 1;
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplSDL2_InitForOpenGL(g_window, gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // Our state
        bool show_demo_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        create_dummy_texture();

        // Main loop
        bool done = false;
        while (!done)
        {
            g_set_room_tab = RoomState::OTHER;
            g_left_button_pressed = false;

            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    done = true;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(g_window))
                    done = true;
                if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        g_left_button_pressed = true;
                    }
                }
            }

            set_window_title();

            // get window size
            SDL_GetWindowSize(
                g_window,
                &g_window_size.x,
                &g_window_size.y
            );


            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(g_window);
            ImGui::NewFrame();

            main_dockspace();

            menu_bar();

            resources_pane();

            resource_windows();

            if (show_demo_window)
            {
                ImGui::ShowDemoWindow();
            }

            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(g_window);
            g_time += 1/60.0;
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(g_window);
        SDL_Quit();

        return 0;
    }

    void main_dockspace()
    {
        g_main_dockspace_id = ImGui::GetID("MainDockspace");
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        bool p_open = true;
        ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        ImGui::PopStyleVar(3);

        ImGui::DockSpace(g_main_dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Docking"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

                if (ImGui::MenuItem("Flag: NoSplit",                "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
                if (ImGui::MenuItem("Flag: NoResize",               "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
                if (ImGui::MenuItem("Flag: PassthruCentralNode",    "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                if (ImGui::MenuItem("Flag: AutoHideTabBar",         "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    static GLuint g_vao = 0;
    static GLuint g_vbo = 0;
    static const size_t k_vertex_data_size = 5;

    const int ATTR_LOC_POSITION = 0;
    const int ATTR_LOC_TEXCOORD = 1;
    const int ATTR_LOC_COLOUR = 2;

    void set_view_matrix(geometry::AABB<coord_t> view)
    {
        float
            x1 = view.left(),
            y1 = view.top(),
            x2 = view.right(),
            y2 = view.bottom();

        g_view = glm::ortho<float>(x1, x2, y1, y2, -10, 10);
        /*view = glm::rotate(view, static_cast<float>(angle), glm::vec3(0, 0, -1));
        view = glm::translate(view, glm::vec3(-1, 1, 0));
        view = glm::scale(view, glm::vec3(2, -2, 1));
        view = glm::scale(view, glm::vec3(1.0/(x2 - x1), 1.0/(y2 - y1), 1));
        view = glm::translate(view, glm::vec3(-x1, -y1, 0));*/

        glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &g_view[0][0]);
    }

    void draw_rect_immediate_tiled(geometry::AABB<coord_t> rect, geometry::AABB<coord_t> fill, GLuint texture=0, int32_t colour=0xffffffff, geometry::AABB<double> uv = {0, 0, 1, 1})
    {
        if (g_vao == 0)
        {
            glGenVertexArrays(1, &g_vao);
            glGenBuffers(1, &g_vbo);
        }
        coord_t wnf = fill.width() / rect.width();
        coord_t hnf = fill.height() / rect.height();
        size_t wn = std::ceil(wnf);
        size_t hn = std::ceil(hnf);
        const size_t count = 6 * wn * hn;
        float* vertices = new float[count * k_vertex_data_size];

        for (size_t x = 0; x < wn; ++x)
        {
            for (size_t y = 0; y < hn; ++y)
            {
                geometry::Vector<coord_t> offset
                {
                    fill.left() + x * rect.width(),
                    fill.top() + y * rect.height()
                };
                geometry::Vector<coord_t> crop
                {
                    rect.width(),
                    rect.height()
                };
                if (x == wn - 1)
                {
                    crop.x *= 1 - (wn - wnf);
                }
                if (y == hn - 1)
                {
                    crop.y *= 1 - (hn - hnf);
                }
                geometry::AABB<coord_t> cropr{ {0, 0}, crop };
                cropr.m_start += offset;
                cropr.m_end += offset;
                for (size_t i = 0; i < 6; ++i)
                {
                    geometry::Vector<coord_t> v;
                    switch (i)
                    {
                    case 0:
                        v = cropr.top_left();
                        break;
                    case 1:
                    case 4:
                        v = cropr.top_right();
                        break;
                    case 2:
                    case 3:
                        v = cropr.bottom_left();
                        break;
                    case 5:
                        v = cropr.bottom_right();
                        break;
                    }
                    size_t j = i + 6 * (y * wn + x);
                    size_t z = j * k_vertex_data_size;
                    vertices[0 + z] = v.x;
                    vertices[1 + z] = v.y;
                    geometry::Vector<coord_t> uvv = uv.apply_normalized(
                        rect.get_normalized_coordinates(v)
                    );
                    vertices[2 + z] = uvv.x * (crop.x / rect.width());
                    vertices[3 + z] = uvv.y * (crop.y / rect.height());
                    vertices[4 + z] = *reinterpret_cast<float*>(&colour);
                }
            }
        }

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

        // position
        glVertexAttribPointer(ATTR_LOC_POSITION, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)0);
        glEnableVertexAttribArray(ATTR_LOC_POSITION);

        // texture coordinate
        glVertexAttribPointer(ATTR_LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_TEXCOORD);

        // colour
        glVertexAttribPointer(ATTR_LOC_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, k_vertex_data_size * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_COLOUR);

        glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * k_vertex_data_size, vertices, GL_STREAM_DRAW);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(g_AttribLocationTex, 0);
        glDrawArrays(GL_TRIANGLES, 0, count);
        delete[] vertices;
    }

    void draw_rect_immediate(geometry::AABB<coord_t> rect, GLuint texture=0, int32_t colour=0xffffffff, geometry::AABB<double> uv = {0, 0, 1, 1})
    {
        if (g_vao == 0)
        {
            glGenVertexArrays(1, &g_vao);
            glGenBuffers(1, &g_vbo);
        }
        const size_t count = 6;
        float vertices[count * k_vertex_data_size];

        for (size_t i = 0; i < 6; ++i)
        {
            geometry::Vector<coord_t> v;
            switch (i)
            {
            case 0:
                v = rect.top_left();
                break;
            case 1:
            case 4:
                v = rect.top_right();
                break;
            case 2:
            case 3:
                v = rect.bottom_left();
                break;
            case 5:
                v = rect.bottom_right();
                break;
            }
            glm::vec4 glm_in_view = g_view * glm::vec4{ v.x, v.y, 0, 1 };

            // check bounds
            if (i == 0 || i == 3) if (glm_in_view.x >= 1) return;
            if (i == 1 || i == 5) if (glm_in_view.x <= -1) return;
            if (i == 0 || i == 1) if (glm_in_view.y >= 1) return;
            if (i == 3 || i == 5) if (glm_in_view.y <= -1) return;

            // set vertex buffer data
            size_t z = i * k_vertex_data_size;
            vertices[0 + z] = v.x;
            vertices[1 + z] = v.y;
            geometry::Vector<coord_t> uvv = uv.apply_normalized(
                rect.get_normalized_coordinates(v)
            );
            vertices[2 + z] = uvv.x;
            vertices[3 + z] = uvv.y;
            vertices[4 + z] = *reinterpret_cast<float*>(&colour);
        }

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

        // position
        glVertexAttribPointer(ATTR_LOC_POSITION, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)0);
        glEnableVertexAttribArray(ATTR_LOC_POSITION);

        // texture coordinate
        glVertexAttribPointer(ATTR_LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, k_vertex_data_size * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_TEXCOORD);

        // colour
        glVertexAttribPointer(ATTR_LOC_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, k_vertex_data_size * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(ATTR_LOC_COLOUR);

        glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * k_vertex_data_size, vertices, GL_STREAM_DRAW);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(g_AttribLocationTex, 0);
        glDrawArrays(GL_TRIANGLES, 0, count);
    }

    // draws selection bubble.
    // invoke with pre=true first, then draw the selectee,
    // then invoke with pre=false.
    inline void draw_selection_rect(geometry::AABB<coord_t> bounds, bool pre)
    {
        int32_t p;
        uint32_t col;
        if (pre)
        {
            p = 0xff * (0.5 + 0.2 * std::sin(g_time * 3));
            col = (p << 24) | (p << 16) | (p << 8) | p;
        }
        else
        {
            p = 0xff * (0.4 + 0.2 * std::sin(g_time * 3));
            uint32_t d = 0xff * (0.8 + 0.2 * std::sin(g_time * 4));
            col = (p << 24) | ( d << 16) | (d << 8) | d;
        }
        draw_rect_immediate(
            bounds,
            g_dummy_texture,
            col
        );
    }

    void menu_bar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open...", "CTRL+O"))
                {
                    std::cout << "Open" << std::endl;
                }

                if (ImGui::MenuItem("Save", "CTRL+S", false, g_project == nullptr))
                {

                }

                if (ImGui::MenuItem("Save As...", "CTRL+SHIFT+S", false, g_project == nullptr))
                {

                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void open_resource(std::string name)
    {
        ResourceType type;
        Resource* resource = get_resource<Resource>(name, &type);

        if (!resource) return;

        if (type == project::OBJECT)
        // set open rooms to instance tab.
        {
            g_set_room_tab = RoomState::INSTANCES;
        }

        if (type == project::BACKGROUND)
        // set open rooms to instance tab.
        {
            g_set_room_tab = RoomState::TILES;
        }

        // currently, only some resources can be opened.
        if (type != project::ROOM)
        {
            return;
        }

        // check for existing open window
        for (size_t i = 0; i < g_resource_windows.size(); ++i)
        {
            if (g_resource_windows.at(i).m_resource == resource)
            {
                // already open.
                return;
            }
        }
        g_resource_windows.emplace_back(
            type, resource
        );
    }

    void resource_leaf(project::ResourceTree& leaf)
    {
        static int selected = -1;
        if (ImGui::Selectable(
            leaf.rtkey.c_str(), // name
            g_resource_selected == leaf.rtkey, // is selected?
            ImGuiSelectableFlags_AllowDoubleClick
        ))
        {
            g_resource_selected = leaf.rtkey;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                open_resource(leaf.rtkey);
            }
        }
    }

    void resource_tree(project::ResourceTree& tree)
    {
        for (project::ResourceTree& elt : tree.list)
        {
            if (elt.m_type != project::CONSTANT && !elt.is_hidden)
            {
                if (!elt.is_leaf && elt.m_name != "")
                {
                    if (ImGui::TreeNode(elt.m_name.c_str()))
                    {
                        resource_tree(elt);
                        ImGui::TreePop();
                    }
                }
                else if (elt.is_leaf)
                {
                    resource_leaf(elt);
                }
            }
        }
    }

    void resources_pane()
    {
        int32_t k_resource_window_width = 50;
        int32_t k_top = 19;
        ImGui::SetNextWindowDockID(g_main_dockspace_id, ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_Once);
        if (ImGui::Begin("Resources"))
        {
            resource_tree(g_project->m_resourceTree);
        }
        ImGui::End();
    }

    namespace
    {
        uint32_t swapendian(uint32_t a)
        {
            return
                  ((a & 0x000000ff) << 24)
                | ((a & 0x0000ff00) << 8)
                | ((a & 0x00ff0000) >> 8)
                | ((a & 0xff000000) << 24);
        }

        void colour_int_bgr_to_float3_rgb(int32_t c, float col[3])
        {
            col[0] = static_cast<float>((c & 0xff) / 255.0);
            col[1] = static_cast<float>(((c & 0xff00) >> 8) / 255.0);
            col[2] = static_cast<float>(((c & 0xff0000) >> 16) / 255.0);
        }

        int32_t colour_float3_rgb_to_int_bgr(const float col[3])
        {
            uint32_t rz = 255 * col[0];
            if (rz > 255) rz = 255;
            uint32_t gz = 255 * col[1];
            if (gz > 255) gz = 255;
            uint32_t bz = 255 * col[2];
            if (bz > 255) bz = 255;

            return rz | (gz << 8) | (bz << 16);
        }
    }

    void resource_window_room_pane_properties(ResourceWindow& rw)
    {
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        // caption
        {
            size_t buffl = room->m_data.m_caption.length() + 32;
            char* buf = new char[buffl];
            strcpy(buf, room->m_data.m_caption.c_str());
            ImGui::InputText("Caption", buf, buffl);
            room->m_data.m_caption = buf;
            delete[] buf;
        }

        // speed
        {
            ImGui::InputDouble("Speed", &room->m_data.m_speed);
        }

        // colour
        {
            float col[3];
            colour_int_bgr_to_float3_rgb(room->m_data.m_colour, col);
            ImGui::ColorEdit3("Colour", col);
            room->m_data.m_colour = colour_float3_rgb_to_int_bgr(col);
        }
    }

    // select region of background to use for tile.
    void resource_window_room_pane_background_region(ResourceWindow& rw)
    {
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        RoomState& state = rw.m_room;

        ResourceBackground* bg = get_resource<ResourceBackground>(g_resource_selected);

        if (bg)
        {
            Texture* tex = get_texture_for_asset_name(g_resource_selected);
            if (tex)
            {
                const geometry::Vector<coord_t> mouse_pos = get_mouse_position_from_cursor();

                state.m_pane_component.target(bg->m_dimensions);
                state.m_pane_component.button();

                set_view_matrix({{0, 0}, bg->m_dimensions});

                // clear
                glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                glClear( GL_COLOR_BUFFER_BIT );

                // draw background
                draw_rect_immediate(
                    { { 0, 0 }, tex->get_dimensions() },
                    tex->get_gl_tex()
                );

                // bg region rect
                const geometry::Vector<int32_t> position = mouse_pos;
                geometry::Vector<int32_t> snap_position_floor{ position };

                const ImGuiIO& io = ImGui::GetIO();

                // snap
                bool snap = false;
                if (!io.KeysDown[SDL_SCANCODE_LCTRL] && !io.KeysDown[SDL_SCANCODE_RCTRL])
                {
                    snap = true;
                    snap_position_floor -= bg->m_offset;
                    snap_position_floor.floor_to_apply(bg->m_tileset_dimensions + bg->m_sep);
                    snap_position_floor += bg->m_offset;
                }

                if (state.m_bg_region.right() < 0 || state.m_bg_region.bottom() < 0)
                // default to fit.
                {
                    state.m_bg_region = { {0, 0}, bg->m_dimensions};
                }

                if (ImGui::IsItemHovered())
                {
                    if (io.MouseDown[0] && io.MouseDownDuration[0] == 0.0f)
                    // select with mouse
                    {
                        state.m_bg_region.m_start = snap_position_floor;
                    }
                    if (io.MouseDown[0])
                    // drag
                    {
                        state.m_bg_region.m_end = snap_position_floor;
                        if (snap)
                        {
                            state.m_bg_region.m_end += bg->m_tileset_dimensions;
                        }
                    }
                }

                // clamp
                state.m_bg_region = state.m_bg_region.intersection(
                    { {0, 0}, bg->m_dimensions }
                );

                // clamp
                if (state.m_bg_region.width() < 1)
                {
                    state.m_bg_region.m_end.x = state.m_bg_region.m_start.x + 1;
                }
                if (state.m_bg_region.height() < 1)
                {
                    state.m_bg_region.m_end.y = state.m_bg_region.m_start.y + 1;
                }

                draw_selection_rect(state.m_bg_region, false);

                state.m_pane_component.untarget();
            }
        }
    }

    void resource_window_room_pane(ResourceWindow& rw)
    {
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        room->load_file();

        rw.m_room.m_tab = RoomState::OTHER;

        // hotkeys
        RoomState::tab_type set_room_tab = g_set_room_tab;

        if (rw.m_active)
        {
            const ImGuiIO& io = ImGui::GetIO();
            if (io.KeysDown[SDL_SCANCODE_I])
            {
                set_room_tab = RoomState::INSTANCES;
            }
            else if (io.KeysDown[SDL_SCANCODE_T])
            {
                set_room_tab = RoomState::TILES;
            }
            // TODO continue this for other hotkeys
        }

        if (set_room_tab != RoomState::OTHER)
        {
            rw.m_room.m_instance_selected = -1;
            rw.m_room.m_tile_selected = -1;
            rw.m_room.m_bg_region = { -1, -1, -1, -1 };
        }

        if (ImGui::BeginTabBar("Pane Items", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
        {
            if (ImGui::BeginTabItem("Properties"))
            {
                resource_window_room_pane_properties(rw);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Instances",
                nullptr,
                set_room_tab == RoomState::INSTANCES
                    ? ImGuiTabItemFlags_SetSelected
                    : ImGuiTabItemFlags_None
            ))
            {
                rw.m_room.m_tab = RoomState::INSTANCES;
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Tiles",
                nullptr,
                set_room_tab == RoomState::TILES
                    ? ImGuiTabItemFlags_SetSelected
                    : ImGuiTabItemFlags_None
            ))
            {
                rw.m_room.m_tab = RoomState::TILES;

                // content
                const float k_margin = 128;
                const ImVec2 content_dimensions = ImGui::GetContentRegionAvail();

                DrawSplitter(
                    true,
                    12,
                    &rw.m_room.m_bg_pane_splitter_height,
                    k_margin,
                    content_dimensions.y - k_margin
                );

                if (ImGui::BeginChild(
                    "BackgroundRegionContainer",
                    ImVec2(content_dimensions.x, rw.m_room.m_bg_pane_splitter_height),
                    false,
                    ImGuiWindowFlags_HorizontalScrollbar
                ));
                {
                    // show selection tab
                    resource_window_room_pane_background_region(rw);
                }
                ImGui::EndChild();
                ImGui::Separator();

                // TODO: additional stuff

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Backgrounds"))
            {

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Views"))
            {

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    void draw_instance(ResourceWindow& rw, const project::ResourceRoom::InstanceDefinition& instance, bool selected)
    {
        Texture& tex = *get_texture_for_asset_name(instance.m_object_name);
        geometry::AABB<coord_t> bounds {
            {instance.m_position - tex.m_offset.scale_copy(instance.m_scale)},
            {instance.m_position - (tex.m_offset - tex.get_dimensions()).scale_copy(instance.m_scale)}
        };

        if (selected)
        {
            draw_selection_rect(bounds, true);
        }

        draw_rect_immediate(
            bounds,
            tex.get_gl_tex(),
            instance.m_colour | 0xff000000,
            { 0, 0, 1, 1 }
        );

        if (selected)
        {
            draw_selection_rect(bounds, false);
        }
    }

    void draw_tile(ResourceWindow& rw, const project::ResourceRoom::TileDefinition& tile, bool selected)
    {
        Texture& tex = *get_texture_for_asset_name(tile.m_background_name);
        uint32_t alpha = 0xff * tile.m_alpha;

        geometry::AABB<coord_t> bounds {
            {tile.m_position},
            {tile.m_position + tile.m_dimensions.scale_copy(tile.m_scale)}
        };

        if (selected)
        {
            draw_selection_rect(bounds, true);
        }

        draw_rect_immediate(
            bounds,
            tex.get_gl_tex(),
            tile.m_blend | (alpha << 24),
            {
                tile.m_bg_position.descale_copy(tex.get_dimensions()),
                (tile.m_bg_position + tile.m_dimensions).descale_copy(tex.get_dimensions())
            }
        );

        if (selected)
        {
            draw_selection_rect(bounds, false);
        }
    }

    void draw_background_layer(ResourceWindow& rw, const project::ResourceRoom::BackgroundLayerDefinition& layer)
    {
        if (!layer.m_visible) return;
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        RoomState& state = rw.m_room;
        const std::string& name = layer.m_background_name;
        if (name.length() > 0 && name != "<undefined>")
        {
            Texture& tex = *get_texture_for_asset_name(name);

            // draw room background
            draw_rect_immediate_tiled(
                {
                    0.0,
                    0.0,
                    static_cast<coord_t>(tex.get_dimensions().x),
                    static_cast<coord_t>(tex.get_dimensions().y)
                },
                {
                    0.0,
                    0.0,
                    layer.m_tiled_x ? room->m_data.m_dimensions.x : tex.get_dimensions().x,
                    layer.m_tiled_y ? room->m_data.m_dimensions.y : tex.get_dimensions().y
                },
                tex.get_gl_tex()
            );
        }
    }

    void resource_window_room_main(ResourceWindow& rw)
    {
        ImVec2 winsize = ImGui::GetContentRegionAvail();
        int32_t winw = winsize.x;
        int32_t winh = winsize.y;

        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        RoomState& state = rw.m_room;

        const geometry::Vector<coord_t> mouse_pos = get_mouse_position_from_cursor();

        state.m_gl_component.target({winw, winh});
        state.m_gl_component.button();

        // clear with gray
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear( GL_COLOR_BUFFER_BIT );

        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::IsItemFocused() || ImGui::IsItemActive() || ImGui::IsItemHovered())
        {
            state.m_camera_position += mouse_pos * state.zoom_ratio();
            state.m_zoom_amount -= io.MouseWheel;
            state.m_camera_position -= mouse_pos * state.zoom_ratio();
        }

        // set view
        {
            real_t x1 = state.m_camera_position.x;
            real_t y1 = state.m_camera_position.y;
            real_t x2 = state.m_camera_position.x + state.zoom_ratio() * winsize.x;
            real_t y2 = state.m_camera_position.y + state.zoom_ratio() * winsize.y;
            set_view_matrix({ x1, y1, x2, y2 });
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable( GL_BLEND );

        // draw room background
        draw_rect_immediate(
            { 0.0, 0.0, room->m_data.m_dimensions.x, room->m_data.m_dimensions.y },
            g_dummy_texture,
            room->m_data.m_colour | (0xff << 24)
        );

        // draw background proper
        for (
            const project::ResourceRoom::BackgroundLayerDefinition& layer
            : room->m_backgrounds
        )
        {
            if (!layer.m_foreground)
            {
                draw_background_layer(rw, layer);
            }
        }

        /*std::vector<size_t> instance_indicies = sort_indices(
            room->m_instances,
            [](const project::ResourceRoom::InstanceDefinition& a,
                const project::ResourceRoom::InstanceDefinition& b)
            {

                // OPTIMIZE: cache the depths
                ResourceTableEntry& rte_a = g_project->m_resourceTable.at(a.m_object_name);
                ResourceObject* oa = dynamic_cast<ResourceObject*>(rte_a.get());

                ResourceTableEntry& rte_b = g_project->m_resourceTable.at(b.m_object_name);
                ResourceObject* ob = dynamic_cast<ResourceObject*>(rte_b.get());

                return oa->m_depth > ob->m_depth;
            }
        );*/

        // deselect if tab has changed
        if (state.m_tab != RoomState::INSTANCES)
        {
            state.m_instance_selected = -1;
        }
        if (state.m_tab != RoomState::TILES)
        {
            state.m_tile_selected = -1;
        }

        // TODO: depth sorting (instances + tiles).
        int32_t i = 0;
        for (project::ResourceRoom::TileDefinition& tile : room->m_tiles)
        {
            draw_tile(rw, tile, i == state.m_tile_selected);
            ++i;
        }

        i = 0;
        for (project::ResourceRoom::InstanceDefinition& instance : room->m_instances)
        {
            draw_instance(
                rw,
                instance,
                i == state.m_instance_selected
            );
            i++;
        }

        // draw foregrounds
        for (
            const project::ResourceRoom::BackgroundLayerDefinition& layer
            : room->m_backgrounds
        )
        {
            if (layer.m_foreground)
            {
                draw_background_layer(rw, layer);
            }
        }

        // where to place:
        const geometry::Vector<coord_t> position = state.m_camera_position + mouse_pos * state.zoom_ratio();
        geometry::Vector<coord_t> snap_position{ position };
        geometry::Vector<coord_t> snap_position_floor{ position };
        // snap
        if (!io.KeysDown[SDL_SCANCODE_LCTRL] && !io.KeysDown[SDL_SCANCODE_RCTRL])
        {
            snap_position.round_to_apply(state.m_grid_snap);
            snap_position_floor.floor_to_apply(state.m_grid_snap);
        }

        // current placement -- instances
        if (state.m_tab == RoomState::INSTANCES && ImGui::IsItemHovered())
        {
            ResourceObject* obj = get_resource<ResourceObject>(g_resource_selected);

            if (obj)
            {
                // ghost
                Texture* tex = get_texture_for_asset_name(g_resource_selected);
                if (tex)
                {
                    draw_rect_immediate(
                        {
                            snap_position - tex->m_offset,
                            snap_position - tex->m_offset + tex->get_dimensions()
                        },
                        tex->get_gl_tex(),
                        0xa0ffd0d0
                    );

                    if (io.MouseDown[0] && io.MouseDownDuration[0] == 0.0)
                    // place instance
                    {
                        state.m_instance_selected = room->m_instances.size();
                        project::ResourceRoom::InstanceDefinition& instance =
                            room->m_instances.emplace_back();

                        instance.m_object_name = g_resource_selected;
                        instance.m_position = snap_position;
                    }

                    if (io.MouseDown[1] && io.MouseDownDuration[1] == 0.0)
                    // deselect placement
                    {
                        g_resource_selected = "";
                    }
                }
            }
            else
            // select an instance
            {
                if (g_left_button_pressed)
                {
                    // deselect by default
                    state.m_instance_selected = -1;

                    // find instance overlapping mouse.
                    for (size_t i = room->m_instances.size(); i --> 0; )
                    {
                        const project::ResourceRoom::InstanceDefinition& instance =
                            room->m_instances.at(i);
                        Texture* tex = get_texture_for_asset_name(instance.m_object_name);
                        if (tex)
                        {
                            geometry::AABB<coord_t> bounds {
                                {instance.m_position - tex->m_offset.scale_copy(instance.m_scale)},
                                {instance.m_position - (tex->m_offset - tex->get_dimensions()).scale_copy(instance.m_scale)}
                            };

                            if (bounds.contains(position))
                            // instance overlaps
                            {
                                state.m_instance_selected = i;
                                break;
                            }
                        }
                    }
                }
                else if (io.MouseDown[0])
                // drag instance
                {
                    if (state.m_instance_selected != -1)
                    {
                        ogm_assert(state.m_instance_selected < room->m_instances.size());
                        project::ResourceRoom::InstanceDefinition& instance =
                            room->m_instances.at(state.m_instance_selected);
                        instance.m_position = snap_position;
                    }
                }
            }

            if (io.KeysDown[SDL_SCANCODE_DELETE] && state.m_instance_selected >= 0)
            // delete instance
            {
                room->m_instances.erase(room->m_instances.begin() + state.m_instance_selected);
                state.m_instance_selected = -1;
            }
        }
        else if (state.m_tab == RoomState::TILES && ImGui::IsItemHovered())
        // current placement -- tiles
        {
            ResourceBackground* bg = get_resource<ResourceBackground>(g_resource_selected);

            if (bg)
            {
                // ghost
                Texture* tex = get_texture_for_asset_name(g_resource_selected);
                if (tex)
                {
                    draw_rect_immediate(
                        {
                            snap_position_floor,
                            snap_position_floor + state.m_bg_region.diagonal()
                        },
                        tex->get_gl_tex(),
                        0xa0ffd0d0,
                        {
                            state.m_bg_region.m_start.descale_copy(tex->get_dimensions()),
                            state.m_bg_region.m_end.descale_copy(tex->get_dimensions())
                        }
                    );

                    if (io.MouseDown[0] && io.MouseDownDuration[0] == 0.0)
                    // place tile
                    {
                        // determine id.
                        instance_id_t id = 10000000;
                        for (project::ResourceRoom::TileDefinition& tile : room->m_tiles)
                        {
                            id = std::max(id, tile.m_id);
                        }

                        // add tile
                        state.m_tile_selected = room->m_tiles.size();
                        project::ResourceRoom::TileDefinition& tile =
                            room->m_tiles.emplace_back();

                        // configure tile
                        tile.m_background_name = g_resource_selected;
                        tile.m_id = id;
                        tile.m_position = snap_position_floor;
                        tile.m_bg_position = state.m_bg_region.m_start;
                        tile.m_dimensions = state.m_bg_region.diagonal();
                        tile.m_scale = {1, 1};
                        tile.m_blend = 0xffffff;

                        // TODO: depth
                        tile.m_depth = 10000;
                    }

                    // deselect
                    if (io.MouseDown[1] && io.MouseDownDuration[1] == 0.0)
                    {
                        g_resource_selected = "";
                    }
                }
            }
            else
            // select
            {
                if (io.MouseDown[0] && io.MouseDownDuration[0] == 0.0f)
                {
                    // deselect by default
                    state.m_tile_selected = -1;

                    // find tile overlapping mouse.
                    for (size_t i = room->m_tiles.size(); i --> 0; )
                    {
                        const project::ResourceRoom::TileDefinition& tile =
                            room->m_tiles.at(i);
                        geometry::AABB<coord_t> bounds {
                            tile.m_position,
                            tile.m_position + tile.m_dimensions.scale_copy(tile.m_scale)
                        };

                        if (bounds.contains(position))
                        {
                            state.m_tile_selected = i;
                            break;
                        }
                    }
                }
                else if (io.MouseDown[0])
                // drag tile
                {
                    if (state.m_tile_selected != -1)
                    {
                        ogm_assert(state.m_tile_selected < room->m_tiles.size());
                        project::ResourceRoom::TileDefinition& tile =
                            room->m_tiles.at(state.m_tile_selected);
                        tile.m_position = snap_position_floor;
                    }
                }
            }

            if (io.KeysDown[SDL_SCANCODE_DELETE] && state.m_tile_selected >= 0)
            // delete instance
            {
                room->m_tiles.erase(room->m_tiles.begin() + state.m_tile_selected);
                state.m_tile_selected = -1;
            }
        }

        state.m_gl_component.untarget();
    }

    void resource_window_room(ResourceWindow& rw)
    {
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        ImVec2 winsize = ImGui::GetContentRegionAvail();
        const int k_margin = 32;
        DrawSplitter(false, 12, &rw.m_room.m_splitter_width, k_margin, winsize.x - k_margin);
        if (ImGui::BeginChild(
            "SidePane",
            ImVec2(rw.m_room.m_splitter_width, winsize.y),
            true
        ))
        {
            resource_window_room_pane(rw);
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if(ImGui::BeginChild(
            "MainView",
            ImVec2(winsize.x - rw.m_room.m_splitter_width, winsize.y),
            true
        ))
        {
            resource_window_room_main(rw);
        }
        ImGui::EndChild();
    }

    void resource_windows()
    {
        for (ResourceWindow& window : g_resource_windows)
        {
            std::string name = window.m_resource->get_name()
                // unique ID for window
                + std::string("##")
                + window.m_id;

            // FIXME: why doesn't this work?
            ImGui::SetNextWindowDockID(g_main_dockspace_id, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(300, 32), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(name.c_str(), &window.m_open))
            {
                window.m_active = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
                switch (window.m_type)
                {
                case project::ROOM:
                    resource_window_room(window);
                    break;
                }
            }
            ImGui::End();
        }

        auto it = std::remove_if(
            g_resource_windows.begin(),
            g_resource_windows.end(),
            [](ResourceWindow& rw) -> bool
            { return !rw.m_open; }
        );
        g_resource_windows.erase(it, g_resource_windows.end());
    }

    void set_window_title()
    {
        if (g_refresh_window_title)
        {
            if (g_project)
            {
                SDL_SetWindowTitle(g_window, g_project->get_project_file_path().c_str());
            }
            else
            {
                SDL_SetWindowTitle(g_window, "OpenGML");
            }
            g_refresh_window_title = false;
        }
    }

    void create_dummy_texture()
    {
        glGenTextures(1, &g_dummy_texture);
        glBindTexture(GL_TEXTURE_2D, g_dummy_texture);
        int32_t z = 0xffffffff;
        glTexImage2D(
            GL_TEXTURE_2D,
            0, // mipmap level
            GL_RGBA, // texture format
            1,
            1,
            0,
            GL_RGBA, // source format
            GL_UNSIGNED_BYTE, // source format
            &z // image data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}
#else
namespace ogm::gui
{
    int run(project::Project*)
    {
        std::cerr << "Graphics must be enabled to run editor.\n";
        return -1;
    }
}
#endif
