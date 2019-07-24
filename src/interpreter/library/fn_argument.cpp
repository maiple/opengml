#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

// retrieve number of arguments provided to script
void ogm::interpreter::fn::getv::argument_count(VO out)
{
    out = staticExecutor.prelocal(1).get<int32_t>();
}

// retrieve jth argument
void ogm::interpreter::fn::getv::argument(VO out, V vi, V vj)
{
    assert(vi == 0);
    size_t j = vj.castCoerce<size_t>();
    
    // assert argument number is less than argument count.
    size_t argc = staticExecutor.prelocal(1).get<int32_t>();
    assert(j < argc);
    
    out.copy(staticExecutor.prelocal(argc - j + 1));
}

#define argumentdef(i)\
void ogm::interpreter::fn::getv::argument##i(VO out) \
{  \
    size_t argc = staticExecutor.prelocal(1).get<int32_t>(); \
    assert(i < argc); \
    out.copy(staticExecutor.prelocal(argc - i + 1));\
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