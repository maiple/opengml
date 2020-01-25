var k_width = 600;
var k_height = 600;

var display = ogm_display_create(k_width, k_height, "Graphics Test");
var model;
for (var i = 0; i < 175; ++i)
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
        draw_text(0, 0, "You should see")
        draw_set_color(c_blue);
        draw_text(0, 16, "a model in")
        draw_set_color(c_green);
        draw_text(0, 32, "the center of")
        draw_text_color(0, 48, "the screen.", c_blue, c_white, c_green, c_red, 1);
        surface_reset_target();
        
        // create model.
        model = d3d_model_create();
        
        // define model
        d3d_model_primitive_begin(model, pr_trianglestrip);
        var c = 100;
        d3d_model_vertex( 93, 87, 0 + c);
        d3d_model_vertex( 93, 213, 0 + c);
        d3d_model_vertex( 144, 144, 213 + c);
        d3d_model_vertex( 93, 213, 0 + c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_vertex( 144, 144, 213+ c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_vertex( 93, 87, 0+ c);
        d3d_model_vertex( 144, 144, 213+ c);
        d3d_model_vertex( 93, 87, 0+ c);
        d3d_model_vertex( 93, 213, 0+ c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_primitive_end();
        
        d3d_start();
        ogm_display_set_matrix_view(0, 0, k_width, k_height, 0);
    }
    
    draw_set_color(c_white);
    
    // draw surface
    draw_surface(srf, 500, 5);
    
    // model
    d3d_model_draw(model, 32, 32, 32, surface_get_texture(srf));

    ogm_display_render_end();
}
ogm_display_destroy(display);
