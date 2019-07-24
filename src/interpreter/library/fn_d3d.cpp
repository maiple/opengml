#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame
#define display frame.m_display

void ogm::interpreter::fn::d3d_set_fog(VO out, V enable, V colour, V start, V end)
{
    display->set_fog(enable.castCoerce<bool>(), start.castCoerce<real_t>(), end.castCoerce<real_t>(), colour.castCoerce<int32_t>());
}
