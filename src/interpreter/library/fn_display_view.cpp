#include "libpre.h"
    #include "fn_display.h"
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
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
}

void ogm::interpreter::fn::getv::view_yview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
}

void ogm::interpreter::fn::setv::view_xview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_yview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_wview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
}

void ogm::interpreter::fn::getv::view_hview(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
}

void ogm::interpreter::fn::setv::view_wview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_hview(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
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
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].x);
}

void ogm::interpreter::fn::getv::view_yview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_position[view_index].y);
}

void ogm::interpreter::fn::setv::view_xview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_yview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_position[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_wview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].x);
}

void ogm::interpreter::fn::getv::view_hview(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_dimension[view_index].y);
}

void ogm::interpreter::fn::setv::view_wview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].x = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::setv::view_hview(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_dimension[view_index].y = val.castCoerce<coord_t>();
}

void ogm::interpreter::fn::getv::view_angle(VO out, OGM_2DARRAY_i V j)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
}

void ogm::interpreter::fn::setv::view_angle(VO out, OGM_2DARRAY_i V j, V val)
{
    size_t view_index = j.castCoerce<size_t>();
    if (view_index >= frame.k_view_count)
    {
        throw MiscError("No view numbered " + std::to_string(view_index));
    }
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}

void ogm::interpreter::fn::getv::view_angle(VO out)
{
    size_t view_index = frame.m_data.m_view_current;
    out = static_cast<real_t>(frame.m_data.m_view_angle[view_index]);
}

void ogm::interpreter::fn::setv::view_angle(V val)
{
    size_t view_index = frame.m_data.m_view_current;
    frame.m_data.m_view_angle[view_index] = val.castCoerce<real_t>();
}