#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::ogm_display_create(VO out, V width, V height, V caption)
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

void ogmi::fn::ogm_display_destroy(VO out, V display)
{
    Display* d = static_cast<Display*>(display.get<void*>());
    if (frame.m_display == d)
    {
        frame.m_display = nullptr;
    }
    delete d;
}

void ogmi::fn::ogm_display_update(VO out)
{
    frame.m_display->flip();
    frame.m_display->process_keys();
}

void ogmi::fn::ogm_display_bind_assets(VO out)
{
    asset_index_t asset_count = frame.m_assets.asset_count();
    for (asset_index_t i = 0; i < asset_count; ++i)
    {
        Asset* asset = frame.m_assets.get_asset(i);
        if (AssetSprite* sprite = dynamic_cast<AssetSprite*>(asset))
        {
            for (size_t j = 0; j < sprite->image_count(); ++j)
            {
                frame.m_display->bind_asset({i, j}, sprite->m_subimage_paths.at(j));
            }
        }
        else if (AssetBackground* background = dynamic_cast<AssetBackground*>(asset))
        {
            frame.m_display->bind_asset({i}, background->m_path);
        }
    }
}

void ogmi::fn::ogm_display_close_requested(VO out)
{
    out = frame.m_display->window_close_requested();
}

// sets display's view matrix to match room size.
void ogmi::fn::ogm_display_set_matrix_view(VO out, V x1, V y1, V x2, V y2, V angle)
{
    frame.m_display->set_matrix_view(x1.castCoerce<coord_t>(), y1.castCoerce<coord_t>(), x2.castCoerce<coord_t>(), y2.castCoerce<coord_t>(), angle.castCoerce<real_t>());
}

void ogmi::fn::ogm_get_prg_end(VO out)
{
    out = frame.m_data.m_prg_end;
}

void ogmi::fn::ogm_get_prg_reset(VO out)
{
    out = frame.m_data.m_prg_reset;
}

void ogmi::fn::ogm_set_prg_end(VO out, V i)
{
    frame.m_data.m_prg_end = i.cond();
}

void ogmi::fn::ogm_set_prg_reset(VO out, V i)
{
    frame.m_data.m_prg_reset = i.cond();
}

void ogmi::fn::ogm_ptr_is_null(VO out, V vptr)
{
    out = !vptr.get<void*>();
}

namespace
{
    inline void render_tile_layer(const TileLayer& layer)
    {
        Variable dummy;

        // OPTIMIZE: accelerated tile layer rendering
        for (tile_id_t id: layer.m_contents)
        {
            Tile& tile = frame.m_tiles.get_tile(id);
            if (tile.m_visible)
            {
                draw_background_part_ext(
                    dummy,
                    tile.m_background_index,
                    tile.m_bg_position.x, tile.m_bg_position.y,
                    tile.m_dimension.x, tile.m_dimension.y,
                    tile.m_position.x, tile.m_position.y,
                    tile.m_scale.x, tile.m_scale.y,
                    tile.m_blend, 1);
            }
        }
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
                staticExecutor.pushSelf(instance);
                execute_bytecode(b);
                staticExecutor.popSelf();
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

    template<StaticEvent event, bool draw, bool draw_tiles=false>
    inline void _ogm_phase_static()
    {
        if (draw)
        {
            if (draw_tiles)
            // draw instances and tiles interleaved.
            {
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
                        render_tile_layer(*iter_tile);
                        ++iter_tile;
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

        assert(false);
    }

    inline void _ogm_event_instance_dynamic(Instance* instance, DynamicEvent event, DynamicSubEvent subevent)
    {
        if (instance->m_data.m_frame_active)
        {
            bytecode_index_t bytecode_index = frame.get_dynamic_event_bytecode(instance->m_data.m_object_index, event, subevent);
            if (bytecode_index != k_no_bytecode)
            {
                Bytecode b = frame.m_bytecode.get_bytecode(bytecode_index);
                staticExecutor.pushSelf(instance);
                frame.m_data.m_event_context.m_object = instance->m_data.m_object_index;
                execute_bytecode(b);
                staticExecutor.popSelf();
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

void ogmi::fn::ogm_phase(VO out, V ev, V subev)
{
    _ogm_phase<false>(ev, subev);
}

void ogmi::fn::ogm_phase_draw(VO out, V ev, V subev)
{
    _ogm_phase<true>(ev, subev);
}

void ogmi::fn::ogm_phase_draw_all(VO out, V ev, V subev)
{
    _ogm_phase<true, true>(ev, subev);
}

void ogmi::fn::ogm_sort_instances(VO out)
{
    // Warning: this invalidates any active with-iterators.
    frame.sort_instances();
}

void ogmi::fn::ogm_room_goto_immediate(VO out, V r)
{
    // Warning: this invalidates any active with-iterators.
    frame.change_room(r.castCoerce<asset_index_t>());
}

void ogmi::fn::getv::ogm_room_queued(VO out)
{
    out = (int32_t) frame.m_data.m_room_goto_queued;
}

void ogmi::fn::setv::ogm_room_queued(V v)
{
    frame.m_data.m_room_goto_queued = v.castCoerce<int32_t>();
}

void ogmi::fn::ogm_debug_start(VO out)
{
    if (ogmi::staticExecutor.m_debugger)
    {
        ogmi::staticExecutor.m_debugger->break_execution();
    }
    else
    {
        std::cout << "Warning: Debugger attached, but not previously in debug mode.\n"
            "Experience likely to be unstable.\n";
        ogmi::staticExecutor.debugger_attach(new Debugger(true));
    }
}
