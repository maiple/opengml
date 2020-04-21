#include "ogm/interpreter/Variable.hpp"

#pragma once

// function with fixed argument count
#define FNDEF0(name)
#define FNDEF1(name, a)
#define FNDEF2(name, a, b)
#define FNDEF3(name, a, b, c)
#define FNDEF4(name, a, b, c, d)
#define FNDEF5(name, a, b, c, d, e)
#define FNDEF6(name, a, b, c, d, e, f)
#define FNDEF7(name, a, b, c, d, e, f, g)
#define FNDEF8(name, a, b, c, d, e, f, g, h)
#define FNDEF9(name, a, b, c, d, e, f, g, h, i)
#define FNDEF10(name, a, b, c, d, e, f, g, h, i, j)
#define FNDEF11(name, a, b, c, d, e, f, g, h, i, j, k)
#define FNDEF12(name, a, b, c, d, e, f, g, h, i, j, k, l)
#define FNDEF13(name, a, b, c, d, e, f, g, h, i, j, k, l, m)
#define FNDEF14(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
#define FNDEF15(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
#define FNDEF16(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)
#define FNDEFN(name)

// writeable variable
#define SETVAR(name)
// readable variable
#define GETVAR(name) namespace getv {void name(VO);}
// read-and-write variable
#define VAR(name) SETVAR(name) GETVAR(name)

// writeable variable array [VO parameter should be ignored or set to undefined.]
// i, j: the indices of the array to write to
// value: the value to write into the array
#ifdef OGM_2DARRAY
#define SETVARA(name) namespace setv {void name(VO dummy, V i, V j, V value);}
// readable variable array
// i, j: the indices to read from
#define GETVARA(name) namespace getv {void name(VO, V i, V j);}
#else
#define SETVARA(name) namespace setv {void name(VO dummy, V i, V value);}
// readable variable array
// i, j: the indices to read from
#define GETVARA(name) namespace getv {void name(VO, V i);}
#endif
// read-and-write variable array
#define VARA(name) SETVARA(name) GETVARA(name)

#define IGNORE_WARN(name)
#define CONST(name, ...)
// looks up "src" as "dst"
#define ALIAS(dst, src)

namespace ogm { namespace interpreter
{
    typedef const Variable& V;
    typedef Variable& VO;
    namespace fn
    {
        // declares all the library functions.
        // they are then referenced in StandardLibrary.cpp
        #include "all.h"
    }
}}

#undef FNDEF0
#undef FNDEF1
#undef FNDEF2
#undef FNDEF3
#undef FNDEF4
#undef FNDEF5
#undef FNDEF6
#undef FNDEF7
#undef FNDEF8
#undef FNDEF9
#undef FNDEF10
#undef FNDEF11
#undef FNDEF12
#undef FNDEF13
#undef FNDEF14
#undef FNDEF15
#undef FNDEF16
#undef FNDEFN
#undef SETVAR
#undef GETVAR
#undef SETVARA
#undef GETVARA
#undef IGNORE_WARN
#undef CONST
#undef ALIAS
