var k_width = 600;
var k_height = 600;

var display = ogm_display_create(k_width, k_height, "Graphics Test");
var model;
for (var i = 0; i < 275; ++i)
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
        draw_set_color(c_ltgrey);
        draw_text(0, 0, "You should see")
        draw_set_color(c_blue);
        draw_text(0, 16, "a model  in")
        draw_set_color(c_green);
        draw_text(0, 32, "the center of")
        draw_text_color(0, 48, "the screen.", c_blue, c_white, c_green, c_red, 1);
        surface_reset_target();
        
        d3d_start();
        
        // create model.
        model = d3d_model_create();
        
        // define model
        d3d_model_primitive_begin(model, pr_trianglestrip);
        var c = 0;
        var a = 200;
        d3d_model_vertex( 93, 87, 0 + c);
        d3d_model_vertex( 93, 213, 0 + c);
        d3d_model_vertex( 144, 144, a + c);
        d3d_model_vertex( 93, 213, 0 + c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_vertex( 144, 144,a+ c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_vertex( 93, 87, 0+ c);
        d3d_model_vertex( 144, 144, a+ c);
        d3d_model_vertex( 93, 87, 0+ c);
        d3d_model_vertex( 93, 213, 0+ c);
        d3d_model_vertex( 217, 213, 0+ c);
        d3d_model_primitive_end();
    }
    
    draw_set_color(c_white);
    
    //ogm_display_set_matrix_view(0, 0, k_width, k_height, 0);
    
    var theta = (i-120)/100.1;
    d3d_set_projection(
        k_width/2 + 1000*sin(theta), k_height/2, -1000*cos(theta),
        k_width/2, k_height/2, 0,
        0, -1, 0
    )
    
    // draw surface
    draw_surface(srf, 500, 5);
    
    // model
    ogm_display_reset_matrix_model();
    d3d_model_draw(model, 32, 32, 32, -1);

    ogm_display_render_end();
}
ogm_display_destroy(display);
