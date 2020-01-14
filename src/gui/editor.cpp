//// source file

#include "ogm/gui/editor.hpp"

#if defined(GFX_AVAILABLE) && defined(IMGUI)

#include "private.hpp"

namespace ogm::gui
{
    using namespace project;

    // global definitions
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
    std::string g_fuzzy_input;
    bool g_fuzzy_input_open = false;

    RoomState::tab_type g_set_room_tab = RoomState::OTHER;
    bool g_left_button_pressed = false;
    std::vector<ResourceWindow> g_resource_windows;
    std::map<ResourceID, Texture> g_texmap;

    std::map<std::string, bool> g_dirty;

    // list of things that have been dirty
    // may be an overestimate however -- trust g_dirty, not this.
    std::set<std::string> g_dirty_list;

    bool resource_is_dirty(const std::string& resource)
    {
        auto iter = g_dirty.find(resource);
        if (iter == g_dirty.end())
        {
            return false;
        }
        return iter->second;
    }

    void set_dirty(const std::string& resource)
    {
        g_dirty[resource] = true;
        g_dirty_list.insert(resource);
    }

    void save_all_dirty()
    {
        for (const std::string& resource : g_dirty_list)
        {
            save_resource(resource);
        }
        g_dirty_list.clear();
    }

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

    Texture* get_texture_for_asset_name(std::string asset_name, ResourceID* out_hash)
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

            handle_hotkeys();

            main_dockspace();

            menu_bar();

            resources_pane();

            resource_windows();

            if (show_demo_window)
            {
                ImGui::ShowDemoWindow();
            }

            fuzzy_finder();

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

    void draw_rect_immediate_tiled(geometry::AABB<coord_t> rect, geometry::AABB<coord_t> fill, GLuint texture, int32_t colour, geometry::AABB<double> uv)
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

    void draw_rect_immediate(geometry::AABB<coord_t> rect, GLuint texture, int32_t colour, geometry::AABB<double> uv)
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

    bool key_pressed(uint32_t scancode)
    {
        const ImGuiIO& io = ImGui::GetIO();
        return io.KeysDown[scancode] && io.KeysDownDuration[scancode] == 0.0f;
    }

    void handle_hotkeys()
    {
        const ImGuiIO& io = ImGui::GetIO();

        bool accelerator_ctrl = (io.KeysDown[SDL_SCANCODE_LCTRL] || io.KeysDown[SDL_SCANCODE_RCTRL]);

        // save
        if (accelerator_ctrl && (key_pressed(SDL_SCANCODE_S)))
        {
            save_all_dirty();
            return;
        }

        // fuzzy-finder
        if (accelerator_ctrl && (key_pressed(SDL_SCANCODE_T) || key_pressed(SDL_SCANCODE_R)))
        {
            if (g_fuzzy_input_open)
            {
                g_fuzzy_input_open = false;
            }
            else
            {
                ImGui::OpenPopup("FuzzyFinder");
                g_fuzzy_input = "";
                g_fuzzy_input_open = true;
                return;
            }
        }
    }

    void fuzzy_finder_string_matches(std::string prefix, size_t count, std::vector<std::string>& out)
    {
        trim(prefix);
        for (const auto& pair : g_project->m_resourceTable)
        {
            const std::string& name = pair.first;
            if (starts_with(name, prefix))
            {
                out.push_back(name);
            }
        }

        std::sort(out.begin(), out.end());
        if (out.size() > count) out.resize(count);
    }

    void fuzzy_finder()
    {
        const ImGuiIO& io = ImGui::GetIO();

        // opened in handle_hotkeys()
        if (ImGui::BeginPopupModal("FuzzyFinder"))
        {
            if (g_fuzzy_input_open)
            {
                if (ImGui::BeginChild(
                    "FuzzyFinderContent",
                    { 350, 128 },
                    false
                ))
                {
                    const size_t buff_len = g_fuzzy_input.size() + 32;
                    char* buff = new char[buff_len];
                    strcpy(buff, g_fuzzy_input.c_str());
                    if (ImGui::IsWindowAppearing())
                    {
                        ImGui::SetKeyboardFocusHere();
                    }
                    if (ImGui::InputText("", buff, buff_len, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoHorizontalScroll))
                    {
                        g_fuzzy_input_open = false;

                        std::vector<std::string> resources;

                        // open best match resource
                        fuzzy_finder_string_matches(g_fuzzy_input, 1, resources);
                        if (!resources.empty())
                        {
                            g_resource_selected = resources.front();
                            open_resource(resources.front());
                        }
                    }

                    g_fuzzy_input = buff;

                    if (g_fuzzy_input.size() > 0)
                    {
                        // selectables
                        std::vector<std::string> resources;
                        fuzzy_finder_string_matches(g_fuzzy_input, 5, resources);

                        for (const std::string& resource : resources)
                        {
                            if (ImGui::Selectable(resource.c_str()))
                            {
                                g_resource_selected = resource;
                                g_fuzzy_input_open = false;
                                open_resource(resource);
                            }
                        }
                    }

                    delete[] buff;
                }

                ImGui::EndChild();

                // close on click outside
                if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup)
                    && ImGui::IsMouseDown(0) && io.MouseDownDuration[0] == 0.0)
                {
                    g_fuzzy_input_open = false;
                }
            }

            if (!g_fuzzy_input_open)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
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

                if (ImGui::MenuItem("Save", "CTRL+S", false, g_project != nullptr))
                {
                    save_all_dirty();
                }

                if (ImGui::MenuItem("Save As...", "CTRL+SHIFT+S", false, g_project != nullptr))
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
            type, resource, name
        );
    }

    bool save_resource(const std::string& name)
    {
        ResourceType type;
        Resource* resource = get_resource<Resource>(name, &type);
        std::cout << "saving resource \"" << name << "\"" << std::endl;
        if (resource->save_file())
        {
            g_dirty_list.erase(name);
            g_dirty.erase(name);
            return false;
        }
        return true;
    }

    uint32_t colour_default = 0xffffffff;
    uint32_t colour_edited  = 0xffa0d0ff;

    void resource_leaf(project::ResourceTree& leaf)
    {
        static int selected = -1;

        uint32_t colour = colour_default;
        if (resource_is_dirty(leaf.rtkey.c_str()))
        {
            colour = colour_edited;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        if (ImGui::Selectable(
            leaf.rtkey.c_str(), // name
            g_resource_selected == leaf.rtkey, // is selected?
            ImGuiSelectableFlags_AllowDoubleClick
        ))
        {
            // set this resource as the selected one.
            g_resource_selected = leaf.rtkey;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                open_resource(leaf.rtkey);
            }
        }
        ImGui::PopStyleColor();
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
