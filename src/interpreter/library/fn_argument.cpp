#include "libpre.h"
    #include "fn_argument.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

// retrieve number of arguments provided to script
void ogm::interpreter::fn::getv::argument_count(VO out)
{
    out = static_cast<real_t>(staticExecutor.prelocal(1).get<int32_t>());
}

// retrieve jth argument
void ogm::interpreter::fn::getv::argument(VO out, V vi, V vj)
{
    ogm_assert(vi == 0);
    size_t j = vj.castCoerce<size_t>();

    // ogm_assert argument number is less than argument count.
    size_t argc = staticExecutor.prelocal(1).get<int32_t>();
    ogm_assert(j < argc);

    out.copy(staticExecutor.prelocal(argc - j + 1));
}

#define argumentdef(i)\
void ogm::interpreter::fn::getv::argument##i(VO out) \
{  \
    size_t argc = staticExecutor.prelocal(1).get<int32_t>(); \
    ogm_assert(i < argc); \
    out.copy(staticExecutor.prelocal(argc - i + 1));\
} \
 \
void ogm::interpreter::fn::setv::argument##i(V v) \
{  \
    size_t argc = staticExecutor.prelocal(1).get<int32_t>(); \
    ogm_assert(i < argc); \
    Variable& arg = staticExecutor.prelocal(argc - i + 1);\
    arg.cleanup(); \
    arg.copy(v); \
}

argumentdef(0)
argumentdef(1)
argumentdef(2)
argumentdef(3)
argumentdef(4)
argumentdef(5)
argumentdef(6)
argumentdef(7)
argumentdef(8)
argumentdef(9)
argumentdef(10)
argumentdef(11)
argumentdef(12)
argumentdef(13)
argumentdef(14)
argumentdef(15)
