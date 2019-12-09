var k_width = 600;
var k_height = 600;

var display = ogm_display_create(k_width, k_height, "Joystick Test");
while (!ogm_display_close_requested())
{
    ogm_display_process_input();
    ogm_display_render_begin();
    ogm_display_set_matrix_view(0, 0, k_width, k_height, 0);

    // gray background
    draw_set_colour(c_gray);

    draw_clear(c_grey);

    draw_set_color(c_white);

    draw_set_colour(c_blue);

    // axes
    for (var i = 0; i <= gp_shoulderrb - gp_axislh; ++i)
    {
        var w = gamepad_axis_value(0, i + gp_axislh)*k_width/2;
        if (w == 0) w = 1;
        draw_rectangle(k_width/2, i*8, k_width/2 + w, i*8 + 6, false);
    }

    // buttons
    for (var i = 0; i <= gp_padd; ++i)
    {
        draw_set_colour(c_grey);
        if (gamepad_button_check(0, i))
        {
            draw_set_colour(c_blue);
        }
        if (gamepad_button_check_pressed(0, i))
        {
            draw_set_colour(c_red);
        }
        if (gamepad_button_check_released(0, i))
        {
            draw_set_colour(c_yellow);
        }
        draw_rectangle(i * k_width/30, 100, (i + 1) * k_width/30, 120, false);
    }

    ogm_display_render_end();
}
ogm_display_destroy(display);
