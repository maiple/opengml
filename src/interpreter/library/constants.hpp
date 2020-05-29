#pragma once

// we need these include files because several fn_* files reference shared enums.
#include "ogm/asset/AssetObject.hpp"
#include "ogm/interpreter/async/Network.hpp"

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
// variadic function
#define FNDEFN(name)

// writeable variable
#define SETVAR(name)
// readable variable
#define GETVAR(name)
// read-and-write variable
#define VAR(name)

#define SETVARA(name)
#define GETVARA(name)
#define VARA(name)

#define IGNORE_WARN(name)
#define ALIAS(dst, src)

namespace ogm::interpreter
{
    #define CONST(name, value) namespace constant{static const auto name = value;}
    
    namespace fn::constant
    {
        
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

    }
}

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
#undef VAR
#undef SETVARA
#undef GETVARA
#undef VARA
#undef IGNORE_WARN
#undef CONST
#undef ALIAS

        