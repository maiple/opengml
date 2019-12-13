//// source file

#include "ogm/gui/editor.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/project/resource/ResourceRoom.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>

#include <GL/glew.h>

namespace ogm::gui
{
    void menu_bar();
    void main_dockspace();
    void resource_windows();
    void resources_pane();
    void set_window_title();

    namespace
    {
        project::Project* g_project = nullptr;
        bool g_refresh_window_title = false;
        SDL_Window* g_window;
        std::string g_resource_selected = "";
        geometry::Vector<int> g_window_size;
        ImGuiID g_next_id = 0;
        ImGuiID g_main_dockspace_id;

        // data for an open resource window.
        struct ResourceWindow
        {
            project::ResourceType m_type;
            project::Resource* m_resource;
            bool m_open = true;
            std::string m_id;
            ImGuiWindowClass* m_class;
            float m_splitter_width = 300;

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
                , m_splitter_width(other.m_splitter_width)
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
                m_splitter_width = other.m_splitter_width;
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
        bool show_demo_window = true;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Main loop
        bool done = false;
        while (!done)
        {
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
        project::ResourceTableEntry& rte = g_project->m_resourceTable.at(name);
        // check for existing open window
        for (size_t i = 0; i < g_resource_windows.size(); ++i)
        {
            if (g_resource_windows.at(i).m_resource == rte.get())
            {
                // already open.
                return;
            }
        }
        g_resource_windows.emplace_back(
            rte.m_type, rte.get()
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

    void resource_window_room_pane(ResourceWindow& rw)
    {
        if (ImGui::BeginTabBar("Pane Items", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
        {
            if (ImGui::BeginTabItem("Properties"))
            {

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

    void resource_window_room_main(ResourceWindow& rw)
    {
    }

    void resource_window_room(ResourceWindow& rw)
    {
        project::ResourceRoom* room =
            dynamic_cast<project::ResourceRoom*>(rw.m_resource);

        ImVec2 winsize = ImGui::GetContentRegionAvail();
        const int k_margin = 32;
        DrawSplitter(false, 12, &rw.m_splitter_width, k_margin, winsize.x - k_margin);
        ImGui::BeginChild(
            "SidePane",
            ImVec2(rw.m_splitter_width, winsize.y),
            true
        );
        resource_window_room_pane(rw);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild(
            "MainView",
            ImVec2(winsize.x - rw.m_splitter_width, winsize.y),
            true
        );
        resource_window_room_main(rw);
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
}
