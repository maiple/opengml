#include "libpre.h"
    #include "fn_camera.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define cameras frame.m_cameras

#define CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam) \
camera_id_t camera_id = vcam.castCoerce<camera_id_t>(); \
if (!cameras.camera_exists(camera_id)) \
{ \
    throw MiscError("no camera exists with id" + std::to_string(camera_id)); \
} \
Camera& camera = cameras.get_camera(camera_id);

void ogm::interpreter::fn::camera_create(VO out)
{
    camera_id_t camera_id = cameras.new_camera();
    Camera& camera = cameras.get_camera(camera_id);
    
    camera.m_manual = true;
    
    out = static_cast<real_t>(camera_id);
}

void ogm::interpreter::fn::camera_create_view(VO out, unsigned char n, const Variable* v)
{
    ogm_assert(n >= 4); //, "too few arguments for camera_create_view");
    ogm_assert(n <= 10); //, "too many arguments for camera_create_view");    
    
    camera_id_t camera_id = cameras.new_camera();
    Camera& camera = cameras.get_camera(camera_id);
    
    camera.m_manual = false;
    
    camera.m_position = {
        v[0].castCoerce<coord_t>(),
        v[1].castCoerce<coord_t>()
    };
    camera.m_dimensions = {
        v[2].castCoerce<coord_t>(),
        v[3].castCoerce<coord_t>()
    };
    camera.m_angle = (n > 4) ? v[4].castCoerce<real_t>() : 0;
    camera.m_instance_target = (n > 5) ? v[5].castCoerce<instance_id_t>() : -1;
    camera.m_velocity = {
        (n > 6) ? v[6].castCoerce<coord_t>() : 0.0,
        (n > 7) ? v[7].castCoerce<coord_t>() : 0.0
    };
    camera.m_margins = {
        (n > 8) ? v[8].castCoerce<coord_t>() : 0.0,
        (n > 9) ? v[9].castCoerce<coord_t>() : 0.0
    };
    
    out = static_cast<real_t>(camera_id);
}

void ogm::interpreter::fn::camera_destroy(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    cameras.free_camera(camera_id);
}

void ogm::interpreter::fn::camera_set_view_mat(VO out, V vcam, V vmat)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    matrix_t matrix;
    variable_to_matrix(vmat, matrix);
    
    camera.m_matrix_view = matrix;
}

void ogm::interpreter::fn::camera_set_projection_mat(VO out, V vcam, V vmat)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    matrix_t matrix;
    variable_to_matrix(vmat, matrix);
    
    camera.m_matrix_projection = matrix;
}

void ogm::interpreter::fn::camera_set_view_pos(VO out, V vcam, V vx, V vy)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_position = {
        vx.castCoerce<coord_t>(),
        vy.castCoerce<coord_t>()
    };
}

void ogm::interpreter::fn::camera_set_view_size(VO out, V vcam, V vw, V vh)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_dimensions = {
        vw.castCoerce<coord_t>(),
        vh.castCoerce<coord_t>()
    };
}

void ogm::interpreter::fn::camera_set_view_angle(VO out, V vcam, V angle)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_angle = angle.castCoerce<real_t>();
}

void ogm::interpreter::fn::camera_set_view_speed(VO out, V vcam, V vx, V vy)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_velocity = {
        vx.castCoerce<coord_t>(),
        vy.castCoerce<coord_t>()
    };
}

void ogm::interpreter::fn::camera_set_view_target(VO out, V vcam, V id)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_instance_target = id.castCoerce<instance_id_t>();
}

void ogm::interpreter::fn::camera_set_view_border(VO out, V vcam, V vw, V vh)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_margins = {
        vw.castCoerce<coord_t>(),
        vh.castCoerce<coord_t>()
    };
}

void ogm::interpreter::fn::camera_set_update_script(VO out, V vcam, V script)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_script_update = script.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::camera_set_begin_script(VO out, V vcam, V script)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_script_begin = script.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::camera_set_end_script(VO out, V vcam, V script)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    camera.m_script_end = script.castCoerce<asset_index_t>();
}

void ogm::interpreter::fn::camera_get_view_mat(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    matrix_to_variable(camera.m_matrix_view, out);
}

void ogm::interpreter::fn::camera_get_projection_mat(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    matrix_to_variable(camera.m_matrix_projection, out);
}

void ogm::interpreter::fn::camera_get_view_x(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_position.x);
}

void ogm::interpreter::fn::camera_get_view_y(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_position.y);
}

void ogm::interpreter::fn::camera_get_view_width(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_dimensions.x);
}

void ogm::interpreter::fn::camera_get_view_height(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_dimensions.y);
}

void ogm::interpreter::fn::camera_get_view_angle(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = camera.m_angle;
}

void ogm::interpreter::fn::camera_get_view_speed_x(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_velocity.x);
}

void ogm::interpreter::fn::camera_get_view_speed_y(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_velocity.y);
}

void ogm::interpreter::fn::camera_get_view_target(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_instance_target);
}

void ogm::interpreter::fn::camera_get_view_border_x(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_margins.x);
}

void ogm::interpreter::fn::camera_get_view_border_y(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_margins.y);
}

void ogm::interpreter::fn::camera_get_update_script(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_script_update);
}

void ogm::interpreter::fn::camera_get_begin_script(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_script_begin);
}

void ogm::interpreter::fn::camera_get_end_script(VO out, V vcam)
{
    CAMERA_ACCESS_PROLOGUE(camera, camera_id, vcam);
    out = static_cast<real_t>(camera.m_script_end);
}