#include "libpre.h"
    #include "fn_function.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Frame.hpp"
#include "ogm/interpreter/Executor.hpp"

#define frame (staticExecutor.m_frame)

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void ogm::interpreter::fn::method(VO out, V bind, V fn)
{
    #ifdef OGM_FUNCTION_SUPPORT
    if (!fn.is_function()) throw MiscError("called method() without providing function.");
    
    Variable d;
    d.copy(bind);
    out.set_function_binding(std::move(d), fn.get_bytecode_index());
    #else
    throw MiscError("function support is required for method(). Please recompile with -DOGM_FUNCTION_SUPPORT");
    #endif
}