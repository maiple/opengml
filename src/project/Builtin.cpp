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

application_surface_enable(false);
application_surface_draw_enable(false);

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
    do
    {
        ogm_room_queued = -1;
        if (!is_undefined(room_first))
        {
            ogm_room_goto_immediate(room_first);
            // TODO: room cc should be run *after* game start, before room start.
            ogm_phase(ev_other, ev_game_start);
            ogm_phase(ev_other, ev_room_start);
        }
        //ogm_phase(ev_other, ev_game_start);
        //ogm_phase(ev_other, ev_room_start);

        // loop over frames
        while (!ogm_get_prg_end() && !ogm_get_prg_reset())
        {
            if (ogm_room_queued != -1)
            {
                var _to = ogm_room_queued;
                ogm_room_queued = -1;
                ogm_room_goto_immediate(_to);
                ogm_phase(ev_other, ev_room_start);
            }

            ogm_display_process_input();

            //// step ////

            ogm_sort_instances();
            ogm_phase(ev_step, ev_step_begin);
            ogm_sort_instances();
            ogm_phase(ev_step, ev_step_builtin);
            ogm_sort_instances();
            ogm_phase(ev_step, ev_step_normal);
            ogm_sort_instances();
            ogm_phase_input();
            ogm_async_network_update();
            ogm_sort_instances();
            ogm_phase(ev_step, ev_step_end);
            ogm_sort_instances();

            // background movement
            for (var i = 0; i < 8; ++i)
            {
                background_x[i] += background_hspeed[i];
                background_y[i] += background_vspeed[i];
            }

            if (ogm_resimulating) continue;

            ///// draw /////

            ogm_display_render_begin();

            ogm_surface_reset_target_all();
            if (application_surface_is_enabled())
            {
                if (!surface_exists(application_surface))
                {
                    ogm_create_application_surface();
                }
                surface_set_target(application_surface);
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
                        ogm_display_set_matrix_view(view_xview[i], view_yview[i], view_xview[i] + view_wview[i], view_yview[i] + view_hview[i], 0);
                        ogm_display_reset_matrix_projection();

                        ogm_phase_draw(ev_draw, ev_draw_begin);
                        ogm_phase_draw_all(ev_draw, ev_draw_normal);
                        ogm_phase_draw(ev_draw, ev_draw_end);
                    }
                }
            }

            // TODO: reset "current view" to 0.

            ogm_surface_reset_target_all();

            ogm_display_set_matrix_view(0, 0, window_get_width(), window_get_height(), 0);
            ogm_display_reset_matrix_projection();

            ogm_phase_draw(ev_draw, ev_draw_post);

            if (ogm_application_surface_is_draw_enabled() && surface_exists(application_surface))
            {
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

                    //draw_surface_ext(application_surface, 0, 0, window_get_width() / surface_get_width(application_surface), window_get_height() / surface_get_height(application_surface), 0, c_white, 1);
                }
            }

            ogm_display_render_end();

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

    } until (ogm_get_prg_end());

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

var anim_end = floor(image_index / image_number) != floor((image_index + image_speed) / image_number);
image_index += image_speed;
image_index -= floor(image_index / image_number) * image_number;
if (anim_end)
{
    event_perform(ev_other, ev_animation_end);
}
)";

}}
