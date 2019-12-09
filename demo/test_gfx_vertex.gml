var k_width = 600;
var k_height = 600;

var display = ogm_display_create(k_width, k_height, "Graphics Test");
for (var i = 0; i < 675; ++i)
{
    ogm_display_render_begin();

    // gray background
    draw_set_colour(c_gray);
    draw_clear(c_gray)
    if (i == 0)
    {
        var srf = surface_create(90, 64);
        surface_set_target(srf);
        draw_clear(c_yellow);
        draw_set_color(c_white);
        draw_text(0, 0, "Hello, World!")
        draw_set_color(c_blue);
        draw_text(0, 16, "Hello, World!")
        draw_set_color(c_green);
        draw_text(0, 32, "Hello, World!")
        draw_text_color(0, 48, "Hello, World!", c_blue, c_white, c_green, c_red, 1);
        surface_reset_target();
    }

    if (i < 90)
    {
        // view matrix
        ogm_display_set_matrix_view(0, 0, k_width, k_height, 0);
    }
    else
    {
        if (i == 90)
        {
            d3d_start();
        }

        if (i > 120)
        {
            var theta = (i-120)/100;
            d3d_set_projection(
                k_width/2 + 1000*sin(theta), k_height/2, -1000*cos(theta),
                k_width/2, k_height/2, 0,
                0, -1, 0
            )
        }

        if (i == 360)
        {
            d3d_set_culling(true);
        }

        if (i > 630)
        {
            d3d_set_projection_ortho(
                0, 0,
                k_width, k_height,
                0
            )
        }
    }

    // rectangle
    draw_set_color(c_red);
    draw_rectangle(12, 5, 80, 50, false);
    //draw_set_alpha(0.8)
    // primitive
    d3d_primitive_begin(pr_triangle_strip)
    var expand = clamp((i-120) / 100, 0, 1);
    d3d_vertex(300, 300, 0)
    d3d_vertex(300, 350, 0)
    d3d_vertex(350, 300, 0)
    d3d_vertex(380, 360, 0)
    d3d_vertex(320, 340, 0)
    d3d_vertex(390, 398, -600 * expand)
    d3d_vertex(390, 198, -300 * expand)
    d3d_vertex(190, 148, -700 * expand)
    d3d_primitive_end()

    draw_surface(srf, 500, 5);

    // cube
    draw_set_color(c_ltgray);
    d3d_draw_block(50, 30, 40 + sin(i/10) * 20, 80, 40 + i, 100, -1, 1, 1);


    ogm_display_render_end();
}
ogm_display_destroy(display);
