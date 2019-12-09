#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "serialize_g.hpp"

#include <sstream>
#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::ogm_display_create(VO out, V width, V height, V caption)
{
    Display* display = new Display();

    if (!display->start(width.castCoerce<uint32_t>(), height.castCoerce<uint32_t>(), caption.castCoerce<std::string>().c_str()))
    {
        if (display) delete display;
        display = nullptr;
    }

    frame.m_display = display;
    out = static_cast<void*>(display);
}

void ogm::interpreter::fn::ogm_display_destroy(VO out, V display)
{
    Display* d = static_cast<Display*>(display.get<void*>());
    if (frame.m_display == d)
    {
        frame.m_display = nullptr;
    }
    delete d;
}

void ogm::interpreter::fn::ogm_display_render_begin(VO out)
{
    frame.m_display->begin_render();
}

void ogm::interpreter::fn::ogm_display_render_clear(VO out)
{
    frame.m_display->clear_render();
}

void ogm::interpreter::fn::ogm_display_render_end(VO out)
{
    frame.m_display->end_render();
}

void ogm::interpreter::fn::ogm_display_process_input(VO out)
{
    frame.m_display->process_keys();
}

void ogm::interpreter::fn::ogm_display_bind_assets(VO out)
{
    asset_index_t asset_count = frame.m_assets.asset_count();
    for (asset_index_t i = 0; i < asset_count; ++i)
    {
        Asset* asset = frame.m_assets.get_asset(i);
        if (AssetSprite* sprite = dynamic_cast<AssetSprite*>(asset))
        {
            for (size_t j = 0; j < sprite->image_count(); ++j)
            {
                frame.m_display->m_textures.bind_asset_to_path({i, j}, sprite->m_subimage_paths.at(j));
            }
        }
        else if (AssetBackground* background = dynamic_cast<AssetBackground*>(asset))
        {
            frame.m_display->m_textures.bind_asset_to_path({i}, background->m_path);
        }
    }
}

void ogm::interpreter::fn::ogm_display_close_requested(VO out)
{
    out = frame.m_display->window_close_requested();
}

// sets display's view matrix to match the given size.
void ogm::interpreter::fn::ogm_display_set_matrix_view(VO out, V x1, V y1, V x2, V y2, V angle)
{
    frame.m_display->set_matrix_view(x1.castCoerce<coord_t>(), y1.castCoerce<coord_t>(), x2.castCoerce<coord_t>(), y2.castCoerce<coord_t>(), angle.castCoerce<real_t>());
}

void ogm::interpreter::fn::ogm_display_reset_matrix_projection(VO out)
{
    frame.m_display->set_matrix_projection();
}

void ogm::interpreter::fn::ogm_display_reset_matrix_model(VO out)
{
    frame.m_display->set_matrix_model();
}

void ogm::interpreter::fn::ogm_get_prg_end(VO out)
{
    out = frame.m_data.m_prg_end;
}

void ogm::interpreter::fn::ogm_get_prg_reset(VO out)
{
    out = frame.m_data.m_prg_reset;
}

void ogm::interpreter::fn::ogm_set_prg_end(VO out, V i)
{
    frame.m_data.m_prg_end = i.cond();
}

void ogm::interpreter::fn::ogm_set_prg_reset(VO out, V i)
{
    frame.m_data.m_prg_reset = i.cond();
}

void ogm::interpreter::fn::ogm_ptr_is_null(VO out, V vptr)
{
    out = !vptr.get<void*>();
}

namespace
{
    inline void render_tile_layer(const TileLayer& layer, const geometry::AABB<coord_t>& aabb)
    {
        Variable dummy;

        const ogm::collision::Entity<coord_t, tile_id_t> collider
        {
            ogm::collision::ShapeType::rectangle,
            aabb,
            -1
        };

        // OPTIMIZE: accelerated tile layer rendering (assume mostly static)
        layer.m_collision.iterate_entity(collider,
            [&](collision::entity_id_t, const ogm::collision::Entity<coord_t, tile_id_t>& e) -> bool
            {
                const Tile& tile = frame.m_tiles.get_tile(e.m_payload);
                if (tile.m_visible && tile.m_alpha > 0)
                {
                    draw_background_part_ext(
                        dummy,
                        tile.m_background_index,
                        tile.m_bg_position.x, tile.m_bg_position.y,
                        tile.m_dimension.x, tile.m_dimension.y,
                        tile.m_position.x, tile.m_position.y,
                        tile.m_scale.x, tile.m_scale.y,
                        tile.m_blend, tile.m_alpha);
                }

                return true;
            },
            false
        );
    }

    template<StaticEvent event>
    inline void _ogm_event_instance_static(Instance* instance)
    {
        if (instance->m_data.m_frame_active)
        {
            bytecode_index_t bytecode_index = frame.get_static_event_bytecode<event>(instance->m_data.m_object_index);
            if (bytecode_index != k_no_bytecode)
            {
            execute_bytecode:
                Bytecode b = frame.m_bytecode.get_bytecode(bytecode_index);
                frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
                staticExecutor.pushSelfDouble(instance);
                execute_bytecode(b);
                staticExecutor.popSelfDouble();
            }
            else
            {
                // try default bytecode (e.g. default draw event):
                bytecode_index = frame.m_config.m_default_events[static_cast<size_t>(event)];
                if (bytecode_index != k_no_bytecode)
                {
                    goto execute_bytecode;
                }
            }
        }
    }

    void draw_background(const BackgroundLayer& bl)
    {
        const AssetBackground* bg = frame.m_assets.get_asset<AssetBackground*>(bl.m_background_index);
        frame.m_display->set_matrix_pre_model();
        frame.m_display->draw_image_tiled(
            frame.m_display->m_textures.get_texture({ bl.m_background_index }),
            bl.m_tiled_x,
            bl.m_tiled_y,
            bl.m_position.x,
            bl.m_position.y,
            bl.m_position.x + bg->m_dimensions.x,
            bl.m_position.y + bg->m_dimensions.y
        );
    }

    template<StaticEvent event, bool draw, bool draw_tiles=false>
    inline void _ogm_phase_static()
    {
        if (draw)
        {
            if (draw_tiles)
            // draw instances and tiles interleaved. Also draw backgrounds.
            {
                // draw background-layer backgrounds
                for (const BackgroundLayer& bl : frame.m_background_layers)
                {
                    if (bl.m_visible && !bl.m_foreground && bl.m_background_index != k_no_asset)
                    {
                        draw_background(bl);
                    }
                }
                auto iter_inst = frame.m_depth_sorted_instances.begin();
                auto iter_tile = frame.m_tiles.get_tile_layers().rbegin();
                while (true)
                {
                    // determine which to render (instance or tile)
                    bool render_instance;
                    if (iter_inst == frame.m_depth_sorted_instances.end() && iter_tile == frame.m_tiles.get_tile_layers().rend())
                    {
                        break;
                    }
                    else if (iter_inst == frame.m_depth_sorted_instances.end())
                    {
                        render_instance = false;
                    }
                    else if (iter_tile == frame.m_tiles.get_tile_layers().rend())
                    {
                        render_instance = true;
                    }
                    else
                    {
                        render_instance = iter_tile->m_depth < (*iter_inst)->m_data.m_depth;
                    }

                    // render
                    if (render_instance)
                    // render instance
                    {
                        if ((*iter_inst)->m_data.m_visible)
                        {
                             _ogm_event_instance_static<event>(*iter_inst);
                        }
                        ++iter_inst;
                    }
                    else
                    // render tile layer
                    {
                        render_tile_layer(*iter_tile, staticExecutor.m_frame.m_display->get_viewable_aabb());
                        ++iter_tile;
                    }
                }

                // draw foreground-layer backgrounds
                for (const BackgroundLayer& bl : frame.m_background_layers)
                {
                    if (bl.m_visible && bl.m_foreground && bl.m_background_index != k_no_asset)
                    {
                        draw_background(bl);
                    }
                }
            }
            else
            // just draw instances
            {
                for (size_t i = 0; i < frame.m_depth_sorted_instances.size(); ++i)
                {
                    Instance* instance = frame.m_depth_sorted_instances.at(i);
                    if (instance->m_data.m_visible)
                    {
                        _ogm_event_instance_static<event>(instance);
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < frame.get_instance_count(); ++i)
            {
                Instance* instance = frame.get_instance_nth(i);
                _ogm_event_instance_static<event>(instance);
            }
        }
    }

    template<bool draw, bool draw_tiles=false>
    void _ogm_phase_static(StaticEvent se)
    {
        switch (se)
        {
        case StaticEvent::CREATE:
            return _ogm_phase_static<StaticEvent::CREATE, draw, draw_tiles>();
        case StaticEvent::DESTROY:
            return _ogm_phase_static<StaticEvent::DESTROY, draw, draw_tiles>();
        case StaticEvent::STEP_BEGIN:
            return _ogm_phase_static<StaticEvent::STEP_BEGIN, draw, draw_tiles>();
        case StaticEvent::STEP:
            return _ogm_phase_static<StaticEvent::STEP, draw, draw_tiles>();
        case StaticEvent::STEP_END:
            return _ogm_phase_static<StaticEvent::STEP_END, draw, draw_tiles>();
        case StaticEvent::STEP_BUILTIN:
            return _ogm_phase_static<StaticEvent::STEP_BUILTIN, draw, draw_tiles>();
        case StaticEvent::DRAW_BEGIN:
            return _ogm_phase_static<StaticEvent::DRAW_BEGIN, draw, draw_tiles>();
        case StaticEvent::DRAW:
            return _ogm_phase_static<StaticEvent::DRAW, draw, draw_tiles>();
        case StaticEvent::DRAW_END:
            return _ogm_phase_static<StaticEvent::DRAW_END, draw, draw_tiles>();
        }

        ogm_assert(false);
    }

    inline void _ogm_event_instance_dynamic(Instance* instance, DynamicEvent event, DynamicSubEvent subevent)
    {
        if (instance->m_data.m_frame_active)
        {
            bytecode_index_t bytecode_index = frame.get_dynamic_event_bytecode(instance->m_data.m_object_index, event, subevent);
            if (bytecode_index != k_no_bytecode)
            {
                Bytecode b = frame.m_bytecode.get_bytecode(bytecode_index);
                staticExecutor.pushSelfDouble(instance);
                frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
                execute_bytecode(b);
                staticExecutor.popSelfDouble();
            }
        }
    }

    template<bool draw>
    inline void _ogm_phase_dynamic(DynamicEvent event, DynamicSubEvent subevent)
    {
        if (draw)
        {
            for (size_t i = 0; i < frame.m_depth_sorted_instances.size(); ++i)
            {
                Instance* instance = frame.m_depth_sorted_instances.at(i);
                if (instance->m_data.m_visible)
                {
                    _ogm_event_instance_dynamic(instance, event, subevent);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < frame.get_instance_count(); ++i)
            {
                Instance* instance = frame.get_instance_nth(i);
                _ogm_event_instance_dynamic(instance, event, subevent);
            }
        }
    }

    inline void _ogm_phase_input(DynamicEvent event, DynamicSubEvent subevent)
    {
        for (Instance* instance : frame.m_input_listener_instances)
        {
            _ogm_event_instance_dynamic(instance, event, subevent);
        }
    }

    template<bool draw, bool draw_tiles=false>
    void _ogm_phase(V ev, V subev)
    {
        DynamicEvent event = static_cast<DynamicEvent>(ev.castCoerce<uint32_t>());
        DynamicSubEvent sub_event = static_cast<DynamicSubEvent>(subev.castCoerce<uint32_t>());

        frame.m_data.m_event_context.m_event = event;
        frame.m_data.m_event_context.m_sub_event = sub_event;

        StaticEvent se;
        if (event_dynamic_to_static(event, sub_event, se))
        {
            _ogm_phase_static<draw, draw_tiles>(se);
        }
        else
        {
            if (draw_tiles)
            {
                throw NotImplementedError("tiles can only be drawn in-phase in static event phases.");
            }
            _ogm_phase_dynamic<draw>(event, sub_event);
        }
    }
}

const int vk_keyboard_min = 2;
const int vk_keyboard_max = 128;

void ogm::interpreter::fn::ogm_phase_input(VO out)
{
    if (frame.m_input_listener_instances.empty()) return;

    for (size_t i = vk_keyboard_min; i <= vk_keyboard_max; ++i)
    {
        if (frame.m_display->get_key_down(i))
        {
            _ogm_phase_input(DynamicEvent::KEYBOARD, static_cast<DynamicSubEvent>(i));
        }
    }

    // TODO: mouse input

    for (size_t i = vk_keyboard_min; i <= vk_keyboard_max; ++i)
    {
        if (frame.m_display->get_key_pressed(i))
        {
            _ogm_phase_input(DynamicEvent::KEYPRESS, static_cast<DynamicSubEvent>(i));
        }
    }

    for (size_t i = vk_keyboard_min; i <= vk_keyboard_max; ++i)
    {
        if (frame.m_display->get_key_released(i))
        {
            _ogm_phase_input(DynamicEvent::KEYRELEASE, static_cast<DynamicSubEvent>(i));
        }
    }
}

void ogm::interpreter::fn::ogm_phase(VO out, V ev, V subev)
{
    _ogm_phase<false>(ev, subev);
}

void ogm::interpreter::fn::ogm_phase_draw(VO out, V ev, V subev)
{
    _ogm_phase<true>(ev, subev);
}

void ogm::interpreter::fn::ogm_phase_draw_all(VO out, V ev, V subev)
{
    _ogm_phase<true, true>(ev, subev);
}

namespace
{
    int g_async_load_map = -1;
}

void ogm::interpreter::fn::getv::async_load(VO out)
{
    out = g_async_load_map;
}

void ogm::interpreter::fn::ogm_async_network_update(VO out)
{
    Variable dummy;
    Variable s_type = "type";
    Variable s_id = "id";
    Variable s_ip = "ip";
    Variable s_port = "port";
    Variable s_socket = "socket";
    Variable s_buffer = "buffer";
    Variable s_size = "size";

    if (g_async_load_map == -1)
    {
        // TODO: read only..?
        g_async_load_map = staticExecutor.m_frame.m_ds_map.ds_new();
    }
    DynamicEventPair de{ DynamicEvent::OTHER, DynamicSubEvent::OTHER_ASYNC_NETWORK };
    std::vector<SocketEvent> events;
    frame.m_network.receive(events);
    for (const SocketEvent& event : events)
    {
        id_t instance = event.m_listener;
        if (frame.instance_active(instance))
        {
            // TODO: consider creating map here.
            Instance* listener = frame.get_instance(instance);
            int32_t buffer = -1;
            ogm_assert(listener);

            // set async map

            ds_map_replace(dummy, g_async_load_map, s_type, static_cast<real_t>(static_cast<int32_t>(event.m_type)));
            ds_map_replace(dummy, g_async_load_map, s_id, event.m_socket);

            // TODO: set ip and port
            ds_map_replace(dummy, g_async_load_map, s_ip, k_undefined_variable);
            ds_map_replace(dummy, g_async_load_map, s_port, k_undefined_variable);

            // special data
            if (event.m_type == SocketEvent::CONNECTION_ACCEPTED)
            {
                ds_map_replace(dummy, g_async_load_map, s_socket, event.m_connected_socket);
            }

            if (event.m_type == SocketEvent::DATA_RECEIVED)
            {
                buffer = frame.m_buffers.create_buffer(event.m_data_len, Buffer::FIXED, 1);

                // OPTIMIZE: just set buffer to use existing data pointer and be read-only.

                frame.m_buffers.get_buffer(buffer).write_n(event.m_data, event.m_data_len);
                frame.m_buffers.get_buffer(buffer).seek(0);

                ds_map_replace(dummy, g_async_load_map, s_buffer, buffer);
                ds_map_replace(dummy, g_async_load_map, s_size, event.m_data_len);
            }

            _ogm_event_instance_dynamic(listener, de.first, de.second);

            if (buffer != -1)
            {
                frame.m_buffers.delete_buffer(buffer);
            }
        }
    }

    ds_map_clear(dummy, g_async_load_map);
    ds_map_destroy(dummy, g_async_load_map);
    g_async_load_map = -1;
    dummy.cleanup();
    s_type.cleanup();
    s_id.cleanup();
    s_port.cleanup();
    s_ip.cleanup();
}

void ogm::interpreter::fn::ogm_sort_instances(VO out)
{
    // Warning: this invalidates any active with-iterators.
    frame.sort_instances();
}

void ogm::interpreter::fn::ogm_room_goto_immediate(VO out, V r)
{
    // Warning: this invalidates any active with-iterators.
    frame.change_room(r.castCoerce<asset_index_t>());
}

void ogm::interpreter::fn::getv::ogm_room_queued(VO out)
{
    out = (int32_t) frame.m_data.m_room_goto_queued;
}

void ogm::interpreter::fn::setv::ogm_room_queued(V v)
{
    frame.m_data.m_room_goto_queued = v.castCoerce<int32_t>();
}

void ogm::interpreter::fn::ogm_debug_start(VO out)
{
    if (ogm::interpreter::staticExecutor.m_debugger)
    {
        ogm::interpreter::staticExecutor.m_debugger->break_execution();
    }
    else
    {
        std::cout << "Warning: Debugger attached, but not previously in debug mode.\n"
            "Experience likely to be unstable.\n";
        ogm::interpreter::staticExecutor.debugger_attach(new Debugger(true));
    }
}

void ogm::interpreter::fn::ogm_suspend(VO out)
{
    // not intended for use -- special compilation support for this function in StandardLibrary.
    ogm_assert(false);
}

void ogm::interpreter::fn::ogm_ds_info(VO out, V type, V id)
{
    ds_index_t did = id.castCoerce<ds_index_t>();
    size_t dtype = type.castCoerce<size_t>();
    switch (dtype)
    {
    case 0:
        {
            DSMap& m = frame.m_ds_map.ds_get(did);
            if (m.m_data.size() == 0)
            {
                std::cout << "{ }\n";
            }
            else
            {
                std::cout << "{\n";
                for (const std::pair<const Variable, DSMap::MarkedVariable>& v : m.m_data)
                {
                    std::cout << "    " << v.first;
                    std::cout << ": " << v.second.v;

                    // TODO: expand recursively?
                    if (v.second.m_flag == DSMap::MarkedVariable::MAP)
                    {
                        std::cout << " [map]";
                    }
                    if (v.second.m_flag == DSMap::MarkedVariable::LIST)
                    {
                        std::cout << " [list]";
                    }
                    std::cout << ",\n";
                }
                std::cout << "}\n";
            }
        }
        break;
    case 1:
        {
            DSList& l = frame.m_ds_list.ds_get(did);
            if (l.m_size == 0)
            {
                std::cout << "[ ]\n";
            }
            else
            {
                std::cout << "[\n";
                size_t i = 0;
                for (const Variable& v : l.m_data)
                {
                    std::cout << "    " << v;

                    // TODO: expand recursively?
                    if (l.m_nested_ds.find(i) != l.m_nested_ds.end())
                    {
                        if (l.m_nested_ds.at(i) == DSList::MAP)
                        {
                            std::cout << " [map]";
                        }
                        if (l.m_nested_ds.at(i) == DSList::LIST)
                        {
                            std::cout << " [list]";
                        }
                    }
                    std::cout << ",\n";
                    ++i;
                }
                std::cout << "]\n";
            }
        }
        break;
    case 2:
        {
            DSStack& l = frame.m_ds_stack.ds_get(did);
            if (l.m_data.size() == 0)
            {
                std::cout << "[ ]\n";
            }
            else
            {
                std::cout << "[\n";
                size_t i = 0;
                for (const Variable& v : l.m_data)
                {
                    std::cout << "    " << v;
                    std::cout << ",\n";
                    ++i;
                }
                std::cout << "]\n";
            }
        }
        break;
    default:
        std::cout << "Info for that datastructure type not supported yet.";
        break;
    }
}

namespace
{
    std::stringstream g_state_stream;
    bool g_queued_save = false;
    bool g_queued_load = false;
}

void ogm::interpreter::fn::ogm_save_state(VO out)
{
    auto& s = g_state_stream;
    g_queued_save = false;
    s.seekp(std::ios_base::beg);
    s.clear();
    ogm_assert(s.good());
    auto a = s.tellg();
    staticExecutor.serialize<true>(g_state_stream);
    _serialize_canary<true>(s);
    ogm_assert(s.good());
    _serialize_all<true>(s);
    _serialize_canary<true>(s);
    ogm_assert(s.good());
    ogm_assert(s.good());
    // assert no read occurred.
    ogm_assert(a == s.tellg());
}

void ogm::interpreter::fn::ogm_load_state(VO out)
{
    auto& s = g_state_stream;
    g_queued_load = false;
    s.seekg(std::ios_base::beg);
    s.clear();
    ogm_assert(s.good());
    ogm_assert(!s.eof());
    auto a = s.tellp();
    staticExecutor.serialize<false>(g_state_stream);
    _serialize_canary<false>(s);
    ogm_assert(s.good());
    _serialize_all<false>(s);
    _serialize_canary<false>(s);
    ogm_assert(s.good());
    // assert no read occurred.
    ogm_assert(a == s.tellp());
}

void ogm::interpreter::fn::ogm_queue_save_state(VO out)
{
    g_queued_save = true;
}

void ogm::interpreter::fn::ogm_queue_load_state(VO out)
{
    g_queued_load = true;
}

void ogm::interpreter::fn::ogm_save_state_queued(VO out)
{
    out = g_queued_save;
}

void ogm::interpreter::fn::ogm_load_state_queued(VO out)
{
    out = g_queued_load;
}
