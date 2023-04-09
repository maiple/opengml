#include "libpre.h"
    #include "fn_view.h"
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

#ifdef OGM_2DARRAY
#define OGM_2DARRAY_i V i,
#else
#define OGM_2DARRAY_i
#endif

namespace
{
    // can return nullptr
    Camera* get_view_camera(size_t view_index)
    {
        if (view_index >= frame.k_view_count) return nullptr;
        return &frame.m_cameras.get_camera(frame.m_data.m_view_camera[view_index]);
    }
}

void ogm::interpreter::fn::getv::view_enabled(VO out)
{
    out = frame.m_data.m_views_enabled;
}

void ogm::interpreter::fn::setv::view_enabled(V e)
{
    frame.m_data.m_views_enabled = e.cond();
}

void ogm::interpreter::fn::getv::view_current(VO out)
{
    out = static_cast<real_t>(frame.m_data.m_view_current);
}

void ogm::interpreter::fn::getv::view_visible(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = frame.m_data.m_view_visible[view_index];
}

void ogm::interpreter::fn::setv::view_visible(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogm::interpreter::fn::getv::view_xview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_position.x);
    }
    #endif
}

void ogm::interpreter::fn::getv::view_yview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_position.y);
    }
    #endif
}

void ogm::interpreter::fn::setv::view_xview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_position.x = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::setv::view_yview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_position.x = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_wview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_dimensions.x);
    }
    #endif
}

void ogm::interpreter::fn::getv::view_hview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_dimensions.y);
    }
    #endif
}

void ogm::interpreter::fn::setv::view_wview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_dimensions.x = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::setv::view_hview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_dimensions.y = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_angle(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = camera->m_angle;
    }
    #endif
}

void ogm::interpreter::fn::setv::view_angle(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_angle = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_camera(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(k_no_camera);
    #else
    out = static_cast<real_t>(frame.m_data.m_view_camera[view_index]);
    #endif
}

void ogm::interpreter::fn::setv::view_camera(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    
    #ifdef OGM_CAMERAS
    // TODO: must be able to handle this!
    // frame.m_view_camera[view_index] = out.castCoerce<camera_id_t>();
    #endif
}

void ogm::interpreter::fn::getv::view_visible(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_visible[view_index]);
}

void ogm::interpreter::fn::setv::view_visible(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_visible[view_index] = val.cond();
}

void ogm::interpreter::fn::getv::view_xview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_position.x);
    }
    #endif
}

void ogm::interpreter::fn::getv::view_yview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_position.y);
    }
    #endif
}

void ogm::interpreter::fn::setv::view_xview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_position.x = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::setv::view_yview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_position.y = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_wview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_dimensions.x);
    }
    #endif
}

void ogm::interpreter::fn::getv::view_hview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = static_cast<real_t>(camera->m_dimensions.y);
    }
    #endif
}

void ogm::interpreter::fn::setv::view_wview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_dimensions.x = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::setv::view_hview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_dimensions.y = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_angle(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        out = camera->m_angle;
    }
    #endif
}

void ogm::interpreter::fn::setv::view_angle(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
    #else
    if (Camera* camera = get_view_camera(view_index))
    {
        camera->m_angle = val.castCoerce<real_t>();
    }
    #endif
}

void ogm::interpreter::fn::getv::view_camera(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifndef OGM_CAMERAS
    out = static_cast<real_t>(k_no_camera);
    #else
    out = static_cast<real_t>(frame.m_data.m_view_camera[view_index]);
    #endif
}

void ogm::interpreter::fn::setv::view_camera(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    
    #ifdef OGM_CAMERAS
    // TODO -- handle this
    //frame.m_data.m_view_camera[view_index] = out.castCoerce<camera_id_t>();
    #endif
}