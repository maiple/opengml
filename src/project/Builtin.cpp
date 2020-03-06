// default code

namespace ogm { namespace project {

const char* k_default_entrypoint = R"(
ogm_extension_init();

var default_room = room_first;

if (is_undefined(default_room) || !room_exists(default_room))
{
    show_debug_message("At least one room is required, or a custom init script.");
    exit;
}

var default_width = room_get_width(default_room);
var default_height = room_get_height(default_room);

if (room_get_view_enabled(default_room))
{
    for (var i = 0; i < 8; ++i)
    {
        if (room_get_view_visible(default_room, i))
        {
            default_width = room_get_view_wview(default_room, i);
            default_height = room_get_view_hview(default_room, i);
            break;
        }
    }
}

var step_begin_script = asset_get_index("_ogm_pre_simulation_")
var step_end_script = asset_get_index("_ogm_post_simulation_")

application_surface_enable(true);
application_surface_draw_enable(true);

var display = ogm_display_create(default_width, default_height, "OpenGML");
if (ogm_ptr_is_null(display))
{
    show_debug_message("Could not create display.");
}
else
{
    // required before any sprites can be drawn.
    ogm_display_bind_assets();

    // "setup" script
    var pre_entry_script = asset_get_index("_ogm_pre_");
    if (script_exists(pre_entry_script))
    {
        script_execute(pre_entry_script);
    }

    // loop over game resets
    while (!ogm_get_prg_end())
    {
        // set first room
        ogm_room_queued = -1;
        if (!is_undefined(room_first))
        {
            ogm_room_goto_immediate(room_first);
            // TODO: room cc should be run *after* game start, before room start.
            ogm_phase(ev_other, ev_game_start);
            ogm_phase(ev_other, ev_room_start);
        }
        else
        {
            ogm_phase(ev_other, ev_game_start);
        }

        // loop over frames
        while (!ogm_get_prg_end() && !ogm_get_prg_reset())
        {
            // note: this precludes any arrays being on the stack
            // when this is called!
            ogm_garbage_collector_process();

            if (ogm_room_queued != -1)
            {
                var _to = ogm_room_queued;
                ogm_room_queued = -1;
                ogm_room_goto_immediate(_to);
                ogm_phase(ev_other, ev_room_start);
            }

            ogm_display_process_input();

            //// step ////

            if (step_begin_script >= 0) script_execute(step_begin_script);
            
            if (!ogm_block_simulation)
            {
                ogm_sort_instances();
                ogm_phase(ev_step, ev_step_begin);
                ogm_sort_instances();
                ogm_phase(ev_step, ev_step_builtin);
                ogm_sort_instances();
                ogm_phase(ev_step, ev_step_normal);
                ogm_sort_instances();
                ogm_phase_input();
                ogm_flush_tcp_sockets();
                if (!ogm_resimulating)
                {
                    ogm_async_network_update();
                }
                ogm_flush_tcp_sockets();
                ogm_sort_instances();
                ogm_phase(ev_step, ev_step_end);
                ogm_sort_instances();


                // background movement
                for (var i = 0; i < 8; ++i)
                {
                    background_x[i] += background_hspeed[i];
                    background_y[i] += background_vspeed[i];
                }
            }
            
            if (step_end_script >= 0) script_execute(step_end_script);

            ogm_flush_tcp_sockets();
            
            // skip draw phase if resimulating.
            if (ogm_resimulating) continue;

            ///// draw /////
            ogm_display_check_error("pre-begin");
            ogm_display_render_begin();

            ogm_surface_reset_target_all();
            
            // clear backbuffer
            ogm_display_render_clear();
            
            if (application_surface_is_enabled())
            {
                if (!surface_exists(application_surface))
                {
                    ogm_create_application_surface();
                }
                
                surface_set_target(application_surface);
                
                // this is applied to the application surface
                ogm_target_view_projection_enable();
            }

            // set matrices to sensible defaults.
            ogm_display_reset_matrix_model();
            ogm_display_set_matrix_view(0, 0, window_get_width(), window_get_height(), 0);
            ogm_display_reset_matrix_projection();

            ogm_phase_draw(ev_draw, ev_draw_pre);

            ogm_display_render_clear();
            draw_set_colour(c_white);
            if (background_showcolour)
            {
                draw_clear(background_colour)
            }
            
            ogm_display_check_error("pre- main draw loop");

            if (!view_enabled)
            {
                ogm_display_set_matrix_view(0, 0, room_get_width(room), room_get_height(room), 0);
                ogm_display_reset_matrix_projection();

                ogm_phase_draw(ev_draw, ev_draw_begin);

                // draws both instances and tiles
                ogm_phase_draw_all(ev_draw, ev_draw_normal);
                
                ogm_phase_draw(ev_draw, ev_draw_end);
            }
            else
            {
                for (var i = 0; i < 8; ++i)
                {
                    if (view_visible[i])
                    {
                        // TODO: set "current view"
                        // TODO: view angle
                        ogm_display_reset_matrix_model();
                        ogm_display_set_matrix_view(view_xview[i], view_yview[i], view_xview[i] + view_wview[i], view_yview[i] + view_hview[i], 0);
                        ogm_display_reset_matrix_projection();

                        ogm_phase_draw(ev_draw, ev_draw_begin);
                        ogm_phase_draw_all(ev_draw, ev_draw_normal);
                        ogm_phase_draw(ev_draw, ev_draw_end);
                    }
                }
            }
            
            ogm_display_check_error("post- main draw loop");

            // this is unrelated to drawing, but we try to give ample opportunity for this.
            ogm_flush_tcp_sockets();

            // TODO: reset "current view" to 0.

            ogm_surface_reset_target_all();

            // reset for drawing application surface.
            ogm_display_reset_matrix_model();
            ogm_display_set_matrix_view(0, 0, window_get_width(), window_get_height(), 0);
            ogm_display_reset_matrix_projection();
            
            shader_reset();

            ogm_phase_draw(ev_draw, ev_draw_post);
            
            ogm_display_check_error("post post-draw");

            if (ogm_application_surface_is_draw_enabled() && application_surface_is_enabled() && surface_exists(application_surface))
            {
                draw_set_alpha(1);
                draw_set_color(c_white);
                
                var srfarr = application_get_position();
                if (is_array(srfarr) && array_length_1d(srfarr) >= 4)
                {
                    draw_surface_ext(
                        application_surface,
                        srfarr[0],
                        srfarr[1],
                        (srfarr[2] - srfarr[0]) / surface_get_width(application_surface),
                        (srfarr[3] - srfarr[1]) / surface_get_height(application_surface),
                        0, c_white, 1
                    );

                    /*draw_surface_ext(
                        application_surface,
                        0, 0,
                        window_get_width() / surface_get_width(application_surface),
                        window_get_height() / surface_get_height(application_surface),
                        0, c_white, 1
                    );*/
                }
                
                // GC will object to top-level-stack frame arrays.
                srfarr = 0;
            }
            

            ogm_display_render_end();
            ogm_display_check_error("post-end");

            if (ogm_display_close_requested())
            {
                ogm_set_prg_end(true);
            }

            // emscripten requires an execution suspend once per loop.
            if (os_browser != browser_not_a_browser)
            {
                if (!ogm_get_prg_end())
                {
                    ogm_suspend();
                }
            }

            // save/load state if queued
            if (ogm_load_state_queued())
            {
                ogm_load_state();
            }

            if (ogm_save_state_queued())
            {
                ogm_save_state();
            }
        }

        //ogm_phase(ev_other, ev_game_end);
    }

    ogm_display_destroy(display);
}

)";

const char* k_default_draw_normal = R"(
draw_self();
)";

const char* k_default_step_builtin = R"(
// gravity
if (gravity != 0)
{
    hspeed += gravity * dcos(gravity_direction);
    vspeed -= gravity * dsin(gravity_direction);
}

// friction
if (friction != 0)
{
    if (point_distance(0, 0, hspeed, vspeed) < friction)
    {
        hspeed = 0;
        yspeed = 0;
    }
    else
    {
        var _direction = direction;
        hspeed -= dcos(_direction) * friction;
        vspeed += dsin(_direction) * friction;
    }
}

// movement
xprevious = x;
yprevious = y;
if (place_free(x + hspeed, y + vspeed))
{
    x += hspeed;
    y += vspeed;
}
else
{
    hspeed = 0;
    vspeed = 0;
}

// alarms
for (var i = 0; i < 8; ++i)
{
    if (alarm[i] > 0)
    {
        alarm[i] -= 1;
        if (alarm[i] <= 0)
        {
            event_perform(ev_alarm, i);
        }
    }
}

if (image_number > 0)
{
    var anim_end = floor(image_index / image_number) != floor((image_index + image_speed) / image_number);
    image_index += image_speed;
    image_index -= floor(image_index / image_number) * image_number;
    if (anim_end)
    {
        event_perform(ev_other, ev_animation_end);
    }
}
else
{
    image_index = 0;
}
)";

}}
