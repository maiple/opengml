#pragma once

#include "TextureStore.hpp"
#include "ogm/common/types.hpp"
#include "ogm/common/serialize.hpp"
#include "ogm/geometry/Vector.hpp"

#include <cstdint>
#include <map>
#include <array>

class GLFWwindow;

namespace ogm {

// forward declarations
namespace asset {
    class AssetFont;
}

namespace interpreter {

using namespace ogm;

typedef uint8_t ogm_keycode_t;

struct Texture;

struct VertexFormatAttribute
{
    enum DataType
    {
        F1,
        F2,
        F3,
        F4,
        RGBA, // colour
        U4, // 4 unsigned bytes
    } m_type;

    enum Destination
    {
        POSITION,
        COLOUR,
        NORMAL,
        TEXCOORD,

        // colour blending
        BW,
        BI,

        // misc
        DEPTH,
        TANGENT,
        BINORMAL,
        FOG,
        SAMPLE
    } m_dest;

    size_t get_size() const;

    size_t get_component_count() const;

    uint32_t get_location() const;
};

class Display
{
public:
    ~Display();
    bool start(uint32_t width, uint32_t height, const char* caption="OpenGML");

    struct Config
    {
        bool m_pixel_precise = true;
    };

    TextureStore m_textures;

public:
    uint32_t make_vertex_format();
    void vertex_format_append_attribute(uint32_t id, VertexFormatAttribute attribute);
    void vertex_format_finish(uint32_t id);

    void free_vertex_format(uint32_t id);

    uint32_t make_vertex_buffer(size_t size = 0);
    void add_vertex_buffer_data(uint32_t id, unsigned char* data, size_t length);
    void freeze_vertex_buffer(uint32_t id);
    size_t vertex_buffer_get_size(uint32_t id);
    size_t vertex_buffer_get_count(uint32_t id);
    void free_vertex_buffer(uint32_t id);

    void associate_vertex_buffer_format(uint32_t vb, uint32_t vf);

    void render_buffer(uint32_t vertex_buffer, TexturePage* image, uint32_t render_glenum);

    void render_array(size_t length, float* vertex_data, TexturePage* texture, uint32_t render_glenum);

    // draws the given image in the given rectangle.
    void draw_image(TextureView*, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1=0, coord_t ty1=0, coord_t tx2=1, coord_t ty2=1);
    void draw_image(TextureView*, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4);
    void draw_image_tiled(TextureView*, bool tiled_x, bool tiled_y, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1=0, coord_t ty1=0, coord_t tx2=1, coord_t ty2=1);

    void draw_filled_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2);
    void draw_filled_circle(coord_t x, coord_t y, coord_t r);
    void draw_outline_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2);
    void draw_outline_circle(coord_t x, coord_t y, coord_t r);

    // TODO: take string_view
    void draw_text(coord_t x, coord_t y, const char* text, real_t halign, real_t valign, bool use_max_width=false, coord_t max_width=0, bool use_sep=false, coord_t sep=0);

    // the number of vertices per circle
    void set_circle_precision(uint32_t prec);
    uint32_t get_circle_precision();

    // fills view with colour
    void draw_fill_colour(uint32_t);

    /////// matrix documentation ///////
    // the following matrices are passed to the vertex shader:
    //
    //   matrix_model (transforms object to be drawn)
    //   matrix_view (rotates and scales world so it is centered on the positive z axis)
    //   matrix_projection (converts view-transformed world into final vertex
    //                      coordinates for rendering in [-1, 1]x[-1, 1])
    //   matrix_model_view (multipled previous for convenience)
    //   matrix_model_view_projection (multipled previous for convenience)
    //
    // there is also a transform stack. None of these are ever applied, they are
    // simply stored faithfully. Some operations can push or pop matrix_model
    // onto/from this stack.
    //
    //   matrix_transform[...]
    //
    // finally, there is a matrix which is applied by the CPU before all other
    // matrices, and it is used only by the basic drawing functions e.g.
    // draw_sprite, draw_rectangle, etc. It was introduced as a quick-fix hack,
    // and should possibly be refactored out for simplicity.
    //
    //   matrix_pre_model

    // sets projection matrix to identity (or flip -1 if drawing to screen buffer)
    void set_matrix_projection();

    // sets camera view
    void set_matrix_view(coord_t x1, coord_t y1, coord_t x2, coord_t y2, real_t angle);

    // sets the projection and view matrix
    void set_camera(coord_t x1, coord_t y1, coord_t z1, coord_t x2, coord_t y2, coord_t z2, coord_t xup, coord_t yup, coord_t zup, real_t fov, real_t aspect, coord_t near, coord_t far);
    void set_camera_ortho(coord_t x, coord_t y, coord_t w, coord_t h, coord_t angle);

    // return value contains all viewable area (and possibly a bit more).
    ogm::geometry::AABB<coord_t> get_viewable_aabb();

    // pre-shader matrix for model
    void set_matrix_pre_model(coord_t x, coord_t y=0, coord_t xcale=1, coord_t yscale=1, real_t angle=0);
    void set_matrix_pre_model();

    // sets the transform for what is to be drawn
    void set_matrix_model(coord_t x=0, coord_t y=0, coord_t xscale=1, coord_t yscale=1, real_t angle=0);

    void set_fog(bool enabled, real_t begin=0, real_t end=1, uint32_t colour=0xffffff);

    std::array<real_t, 16> get_matrix_view();
    std::array<real_t, 16> get_matrix_projection();
    std::array<real_t, 16> get_matrix_model();

    void set_matrix_view(std::array<real_t, 16>);
    void set_matrix_projection(std::array<real_t, 16>);
    void set_matrix_model(std::array<real_t, 16>);

    // sets the colour of the space outside of viewports
    uint32_t get_clear_colour();
    void set_clear_colour(uint32_t);

    uint32_t get_colour();
    void set_colour(uint32_t);

    float get_alpha();
    void set_alpha(float);

    void get_colours4(uint32_t*);
    void set_colours4(uint32_t[4]);

    void set_font(asset::AssetFont* font=nullptr, TextureView* texture=nullptr);

    void set_blendmode(int32_t src, int32_t dst);
    void set_blending_enabled(bool enabled);

    bool window_close_requested();

    // swap back and front buffer, displaying the buffer.
    void flip();

    void clear_keys();

    void process_keys();

    // yields state of key at the time of last process.
    bool get_key_down(ogm_keycode_t);
    bool get_key_pressed(ogm_keycode_t);
    bool get_key_released(ogm_keycode_t);
    bool get_key_direct(ogm_keycode_t);

    ogm_keycode_t get_current_key();

    ogm::geometry::Vector<real_t> get_display_dimensions();
    ogm::geometry::Vector<real_t> get_window_dimensions();
    ogm::geometry::Vector<real_t> get_window_position();
    void set_window_position(real_t x, real_t y);
    void set_window_size(real_t w, real_t h);

    // retrieves mouse coord in window
    ogm::geometry::Vector<real_t> get_mouse_coord();

    // retrieves mouse coord applying inverted view matrix
    ogm::geometry::Vector<real_t> get_mouse_coord_invm();

    // joysticks
    bool get_joysticks_supported();
    size_t get_joystick_max();
    bool get_joystick_connected(size_t index);
    std::string get_joystick_name(size_t index);
    size_t get_joystick_axis_count(size_t index);
    real_t get_joystick_axis_value(size_t index, size_t axis_index);
    size_t get_joystick_button_count(size_t index);
    bool get_joystick_button_down(size_t index, size_t b);
    bool get_joystick_button_pressed(size_t index, size_t b);
    bool get_joystick_button_released(size_t index, size_t b);

    void begin_render();

    void clear_render();

    void end_render();

    // 3d setup
    void set_depth_test(bool);
    void set_culling(bool);
    void set_zwrite(bool);
    void set_colour_mask(bool r, bool g, bool b, bool a);

    // pass in nullptr to reset.
    void set_target(TexturePage*);

    // this is disabled when drawing to surfaces.
    void enable_view_projection(bool);

    // sets the current transformation to identity
    void transform_identity();

    // applies the given matrix to the current transformation
    void transform_apply(std::array<float, 16> matrix);

    // applies a rotation to the current transformation
    void transform_apply_rotation(coord_t angle, coord_t x, coord_t y, coord_t z);

    void transform_stack_clear();
    bool transform_stack_empty();
    bool transform_stack_push();
    bool transform_stack_pop();
    bool transform_stack_top();
    bool transform_stack_discard();

    // applies current transformation to the vertex
    void transform_vertex(std::array<real_t, 3>& vertex);

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s);

private:
    std::array<real_t, 16> get_matrix(int mat);
    asset::AssetFont* m_font = nullptr;
    TextureView* m_font_texture = nullptr;

private:
    void set_matrix(int mat, std::array<real_t, 16> array);

    void update_camera_matrices();

    void blank_image();

    void render_vertices(float* vertices, size_t count, uint32_t texture, uint32_t render_glenum);

    void draw_text_ttf(coord_t x, coord_t y, const char* text, real_t halign, real_t valign, bool use_max_width=false, coord_t max_width=0, bool use_sep=false, coord_t sep=0);
    void draw_text_image(coord_t x, coord_t y, const char* text, real_t halign, real_t valign, bool use_max_width=false, coord_t max_width=0, bool use_sep=false, coord_t sep=0);
};

}}
