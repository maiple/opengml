#include "private.hpp"

#ifdef GFX_AVAILABLE

namespace ogm::gui
{

using namespace project;

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

}

#endif
