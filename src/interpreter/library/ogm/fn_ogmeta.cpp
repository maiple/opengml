#include "interpreter/library/libpre.h"
    #include "fn_ogmeta.h"
    #include "interpreter/library/fn_draw.h"
    #include "interpreter/library/fn_ds.h"
#include "interpreter/library/libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"
#include "ogm/interpreter/Garbage.hpp"
#include "interpreter/library/serialize_g.hpp"

#include <sstream>
#include <string>

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
                asset::AssetSprite::SubImage& image = sprite->m_subimages.at(j);
                ogm_assert(image.m_path != "");
                frame.m_display->m_textures.bind_asset_to_callback(
                    {i, j},
                    [sprite, j]() { return &sprite->m_subimages.at(j); }
                );
            }
        }
        else if (AssetBackground* background = dynamic_cast<AssetBackground*>(asset))
        {
            frame.m_display->m_textures.bind_asset_to_callback(
                {i},
                [background]() { return &background->m_image; }
            );
        }
        else if (AssetShader* shader = dynamic_cast<AssetShader*>(asset))
        {
            frame.m_display->bind_and_compile_shader(
                i,
                shader->m_vertex_source,
                shader->m_pixel_source
            );
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

void ogm::interpreter::fn::ogm_display_check_error(VO out, V str)
{
    frame.m_display->check_error(str.castCoerce<std::string>());
}

void ogm::interpreter::fn::ogm_display_set_multisample(VO out, V c)
{
    frame.m_display->set_multisample(c.castCoerce<uint32_t>());
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
                const size_t count = frame.m_depth_sorted_instances.size();
                for (size_t i = 0; i < count; ++i)
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
            const size_t count = frame.get_instance_count();
            for (size_t i = 0; i < count; ++i)
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
            const size_t count = frame.m_depth_sorted_instances.size();
            for (size_t i = 0; i < count; ++i)
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
            // we intentionally do not iterate over any instances created as a result of this iteration.
            const size_t count = frame.get_instance_count();
            for (size_t i = 0; i < count; ++i)
            {
                Instance* instance = frame.get_instance_nth(i);
                _ogm_event_instance_dynamic(instance, event, subevent);
            }
        }
    }

    // objects listen to keyboard and mouse input.
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

namespace
{
    #define CONST(x, y) constexpr size_t x = y;
    #include "interpreter/library/fn_keycodes.h"
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

    for (size_t i = mb_left; i <= mb_middle; ++i)
    {
        size_t event = 4 + (i - mb_left);
        if (frame.m_display->get_key_pressed(i))
        {
            _ogm_phase_input(DynamicEvent::MOUSE, static_cast<DynamicSubEvent>(event));
        }
    }

    for (size_t i = mb_left; i <= mb_middle; ++i)
    {
        size_t event = (i - mb_left);
        if (frame.m_display->get_key_down(i))
        {
            _ogm_phase_input(DynamicEvent::MOUSE, static_cast<DynamicSubEvent>(event));
        }
    }

    for (size_t i = mb_left; i <= mb_middle; ++i)
    {
        size_t event = 7 + (i - mb_left);
        if (frame.m_display->get_key_released(i))
        {
            _ogm_phase_input(DynamicEvent::MOUSE, static_cast<DynamicSubEvent>(event));
        }
    }

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

void ogm::interpreter::fn::ogm_flush_tcp_sockets(VO out)
{
    staticExecutor.m_frame.m_network.flush_send_all();
}

static void execute_async_events(std::vector<std::unique_ptr<AsyncEvent>>& events)
{
    Variable dummy;
    if (g_async_load_map == -1)
    {
        // TODO: read only..?
        g_async_load_map = staticExecutor.m_frame.m_ds_map.ds_new();
    }
    
    for (std::unique_ptr<AsyncEvent>& eventptr : events)
    {
        AsyncEvent& event = *eventptr;
        ogm::id_t instance = event.m_listener_id;
        if (frame.instance_active(instance))
        {
            // TODO: consider creating map here.
            Instance* listener = frame.get_instance(instance);
            ogm_assert(listener);

            event.produce_info(g_async_load_map);

            // run event
            _ogm_event_instance_dynamic(
                listener,
                DynamicEvent::OTHER,
                event.m_async_subevent
            );

            event.cleanup();

            ds_map_clear(dummy, g_async_load_map);
        }
    }
    
    ds_map_destroy(dummy, g_async_load_map);
    g_async_load_map = -1;
    dummy.cleanup();
}

void ogm::interpreter::fn::ogm_async_network_update(VO out)
{
    std::vector<std::unique_ptr<AsyncEvent>> events;
    frame.m_network.receive(events);
    if (!events.empty())
    {
        execute_async_events(events);
    }
}

void ogm::interpreter::fn::ogm_async_update(VO out)
{
    std::vector<std::unique_ptr<AsyncEvent>> events;
    frame.m_network.receive(events);
    frame.m_http.receive(events);
    
    if (!events.empty())
    {
        execute_async_events(events);
    }
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

void ogm::interpreter::fn::ogm_buffer_info(VO out, V vid)
{
    buffer_id_t id = vid.castCoerce<buffer_id_t>();
    if (!frame.m_buffers.buffer_exists(id))
    {
        std::cout << "Buffer " << id << " does not exist." << std::endl;
        return;
    }
    Buffer& b = frame.m_buffers.get_buffer(id);
    const bool owned = frame.m_buffers.buffer_is_owned(id);
    const size_t position = b.tell();

    std::cout << "Buffer " << id << ":\n";
    std::cout << "  Size: " << b.size() << " bytes\n";
    std::cout << "  Type: ";
    switch(b.get_type())
    {
    case Buffer::FIXED:
        std::cout << "buffer_fixed\n";
        break;
    case Buffer::GROW:
        std::cout << "buffer_grow\n";
        break;
    case Buffer::WRAP:
        std::cout << "buffer_wrap\n";
        break;
    case Buffer::FAST:
        std::cout << "buffer_fast\n";
        break;
    default:
        std::cout << "unknown.\n";
    }
    std::cout << "  Size: " << b.size() << " bytes\n";
    std::cout << "  Alignment: " << b.get_align() << " bytes\n";
    std::cout << "  Position: " << position << " bytes\n";

    const size_t k_frame_width = 32;

    // display 3 lines for the buffer.
    for (size_t j = 0; j < 3; ++j)
    {
        size_t i = b.tell();
        if (i < k_frame_width / 2)
        {
            i = 0;
        }
        else
        {
            i -= k_frame_width;
        }
        for (; i < b.tell() + k_frame_width / 2 && i < b.size(); ++i)
        {
            uint8_t byte = b.get_data()[i];

            switch(j)
            {
            case 0:
                if (i == position)
                {
                    std::cout << "v  ";
                }
                else
                {
                    std::cout << "   ";
                }
                break;
            case 1:
                {
                    std::string b = std::to_string(static_cast<uint32_t>(byte));
                    while (b.length() < 3) b += " ";
                    std::cout << std::hex << b << std::dec << " ";
                }
                break;
            case 2:
                {
                    std::string e = escape(byte);
                    while (e.length() < 3) e += " ";
                    std::cout << e;
                }
                break;
            }
        }
        std::cout << "\n";
    }

    // return buffer to original position.
    b.seek(position);
}

void ogm::interpreter::fn::_ogm_assert(VO out, V v)
{
    if (!v.cond())
    {
        throw MiscError("Assertion failed.");
    }
}

void ogm::interpreter::fn::_ogm_assert(VO out, V v, V m)
{
    if (!v.cond())
    {
        throw MiscError("Assertion failed: " + m.castCoerce<std::string>());
    }
}

void ogm::interpreter::fn::ogm_garbage_collector_process(VO out)
{
    #ifdef OGM_GARBAGE_COLLECTOR
    out = static_cast<real_t>(g_gc.process());
    #else
    out = 0;
    #endif
}

void ogm::interpreter::fn::ogm_garbage_collector_count(VO out)
{
    #ifdef OGM_GARBAGE_COLLECTOR
    out = static_cast<real_t>(g_gc.get_heap_count());
    #else
    out = 0;
    #endif
}

void ogm::interpreter::fn::ogm_garbage_collector_graph(VO out)
{
    out = g_gc.graph();
}

void ogm::interpreter::fn::ogm_garbage_collector_node_outgoing_count(VO out, V v)
{
    #ifdef OGM_GARBAGE_COLLECTOR
    if (GCNode* node = v.get_gc_node())
    {
        out = static_cast<real_t>(node->m_references.size());
    }
    else
    {
        out = -1.0;
    }
    #else
    out = false;
    #endif
}

void ogm::interpreter::fn::ogm_garbage_collector_enabled(VO out)
{
    #ifdef OGM_GARBAGE_COLLECTOR
    out = true;
    #else
    out = false;
    #endif
}

namespace
{
    bool g_resimulating = false;
    bool g_block_simulation = false;
}

void ogm::interpreter::fn::getv::ogm_resimulating(VO out)
{
    out = g_resimulating;
}

void ogm::interpreter::fn::setv::ogm_resimulating(V v)
{
    g_resimulating = v.cond();
}

void ogm::interpreter::fn::getv::ogm_block_simulation(VO out)
{
    out = g_block_simulation;
}

void ogm::interpreter::fn::setv::ogm_block_simulation(V v)
{
    g_block_simulation = v.cond();
}

void ogm::interpreter::fn::ogm_garbage_collector_integrity_check(VO out)
{
    #ifdef OGM_GARBAGE_COLLECTOR
    g_gc.integrity_check_begin();
    frame.gc_integrity_check();
    g_gc.integrity_check_end();
    #endif
}

void ogm::interpreter::fn::setv::ogm_is_host(V v)
{
    // ignore.
}

void ogm::interpreter::fn::getv::ogm_is_host(VO out)
{
    out = true;
}
