#pragma once

#if defined(GFX_AVAILABLE) && defined(IMGUI)

#include "private.hpp"
#include <SDL2/SDL.h>

namespace ogm::gui
{
    using namespace project;
    
    typedef int64_t ResourceID;
    
    // ---------------------------------------------------------------------
    // project data
    extern project::Project* g_project;
    extern std::map<std::string, bool> g_dirty;
    
    // list of things that have been dirty
    // may be an overestimate however -- trust g_dirty, not this.
    extern std::set<std::string> g_dirty_list;

    // ---------------------------------------------------------------------
    // global state
    extern std::string g_resource_selected;
    extern double g_time;
    extern bool g_fuzzy_input_open;
    extern std::string g_fuzzy_input;
    
    // transient state
    extern bool g_left_button_pressed;
    
    // window
    extern bool g_refresh_window_title;
    extern geometry::Vector<int> g_window_size;
    
    // ---------------------------------------------------------------------
    // SDL
    extern SDL_Window* g_window;
    extern GLuint g_dummy_texture;
    extern glm::mat4 g_view;
    extern std::map<ResourceID, Texture> g_texmap;
    
    // imgui
    extern ImGuiID g_next_id;
    extern ImGuiID g_main_dockspace_id;
    
    // access ---------------------------------------------------------------
    bool resource_is_dirty(const std::string& resource);
    void set_dirty(const std::string& resource);
    void save_all_dirty();
    
    Texture* get_texture_embedded(const uint8_t* data, size_t len, ResourceID* out_hash=nullptr);
    Texture* get_texture_for_asset_name(std::string asset_name, ResourceID* out_hash);
}

#endif