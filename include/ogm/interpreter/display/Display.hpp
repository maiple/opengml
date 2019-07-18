#pragma once

#include "ogm/asset/Asset.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"

#include <cstdint>
#include <map>

class GLFWwindow;

namespace ogmi {

using namespace ogm;

typedef uint8_t ogm_keycode_t;

class Display
{
public:
    ~Display();
    bool start(uint32_t width, uint32_t height, const char* caption="OpenGML");

    struct Config
    {
        bool m_pixel_precise = true;
    };

    struct ImageDescriptor
    {
        asset_index_t m_asset_index;
        size_t m_image_index;

        bool operator<(const ImageDescriptor& other) const
        {
            if (m_asset_index == other.m_asset_index)
            {
                return m_image_index < other.m_image_index;
            }
            return m_asset_index < other.m_asset_index;
        }

        ImageDescriptor(const ImageDescriptor& other)
            : m_asset_index(other.m_asset_index)
            , m_image_index(other.m_image_index)
        { }

        ImageDescriptor(asset_index_t index)
            : m_asset_index(index)
            , m_image_index(0)
        { }

        ImageDescriptor(asset_index_t asset_index, size_t image_index)
            : m_asset_index(asset_index)
            , m_image_index(image_index)
        { }
    };
private:
    // this map is used to reconstruct image data associated with an asset
    // if it is ever freed from memory.
    // maps "asset, optional subimage" -> *
    std::map<ImageDescriptor, std::string> m_descriptor_path;
    std::map<ImageDescriptor, uint32_t> m_descriptor_texture;

public:
    // informs the display where to find the asset's image data.
    // asset should be index of a sprite or background asset.
    void bind_asset(ImageDescriptor, std::string image_path);

    // preloads the given image if needed. Requires that the asset is bound with bind_asset.
    // returns true on success
    bool cache_image(ImageDescriptor);

    // draws the given image in the given rectangle.
    void draw_image(ImageDescriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t tx1=0, coord_t ty1=0, coord_t tx2=1, coord_t ty2=1);
    void draw_image(ImageDescriptor, coord_t x1, coord_t y1, coord_t x2, coord_t y2, coord_t x3, coord_t y3, coord_t x4, coord_t y4, coord_t tx1, coord_t ty1, coord_t tx2, coord_t ty2, coord_t tx3, coord_t ty3, coord_t tx4, coord_t ty4);

    void draw_filled_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2);
    void draw_filled_circle(coord_t x, coord_t y, coord_t r);
    void draw_outline_rectangle(coord_t x1, coord_t y1, coord_t x2, coord_t y2);
    void draw_outline_circle(coord_t x, coord_t y, coord_t r);

    // the number of vertices per circle
    void set_circle_precision(uint32_t prec);
    uint32_t get_circle_precision();

    // fills view with colour
    void draw_fill_colour(uint32_t);

    // sets camera view
    void set_matrix_view(coord_t x1, coord_t y1, coord_t x2, coord_t y2, real_t angle);
    void set_fog(bool enabled, real_t begin=0, real_t end=1, uint32_t colour=0xffffff);

    // sets the transform for what is to be drawn
    void set_matrix_model(coord_t x=0, coord_t y=0, coord_t xscale=1, coord_t yscale=1, real_t angle=0);

    // sets the colour of the space outside of viewports
    uint32_t get_clear_colour();
    void set_clear_colour(uint32_t);

    uint32_t get_colour();
    void set_colour(uint32_t);

    float get_alpha();
    void set_alpha(float);

    void get_colours4(uint32_t*);
    void set_colours4(uint32_t[4]);

    bool window_close_requested();

    // swap back and front buffer, displaying the buffer.
    void flip();

    void clear_keys();

    void process_keys();

    // yields state of key at the time of last process.
    bool get_key_down(ogm_keycode_t);
    bool get_key_pressed(ogm_keycode_t);
    bool get_key_released(ogm_keycode_t);

    ogm::geometry::Vector<real_t> get_display_dimensions();
    void set_window_position(real_t x, real_t y);
    void set_window_size(real_t w, real_t h);
    
    // joysticks
    bool get_joysticks_supported();
    size_t get_joystick_max();
    bool get_joystick_connected(size_t index);
    std::string get_joystick_name(size_t index);
    size_t get_joystick_axis_count(size_t index);
    real_t get_joystick_axis_value(size_t index, size_t axis_index);
    
    
private:
    void update_camera_matrices();

    void begin_render();

    void end_render();

    void blank_image();
    
    void render_vertices(float* vertices, size_t count, uint32_t texture, uint32_t render_glenum);
};

}
