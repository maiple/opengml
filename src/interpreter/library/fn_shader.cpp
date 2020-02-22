#include "libpre.h"
    #include "fn_shader.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define display frame.m_display

void ogm::interpreter::fn::shader_set(VO out, V id)
{
    display->use_shader(id.castCoerce<asset_index_t>());
}

void ogm::interpreter::fn::shader_reset(VO out)
{
    display->use_shader(std::numeric_limits<asset_index_t>::max());
}

void ogm::interpreter::fn::shaders_are_supported(VO out)
{
    out = true;
}

void ogm::interpreter::fn::shader_get_uniform(VO out, V index, V handle)
{
    out = display->shader_get_uniform_id(
        index.castCoerce<asset_index_t>(),
        handle.castCoerce<std::string>()
    );
}

void ogm::interpreter::fn::shader_set_uniform_f(VO out, uint8_t c, const Variable* v)
{
    if (c <= 1 || c > 5) throw MiscError("wrong number of arguments to shader_set_uniform_f");
    
    float values[4];
    int32_t uniform_id = v[0].castCoerce<int32_t>();
    for (size_t i = 0; i < c - 1 && i < 4; ++i)
    {
        values[i] = v[i + 1].castCoerce<real_t>();
    }
    display->shader_set_uniform_f(uniform_id, c - 1, values);
}