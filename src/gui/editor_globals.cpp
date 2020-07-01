#if defined(GFX_AVAILABLE) && defined(IMGUI)

#include "editor_globals.hpp"

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
    
    Texture* get_texture_embedded(const uint8_t* data, size_t len, ResourceID* out_hash)
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
}

#endif