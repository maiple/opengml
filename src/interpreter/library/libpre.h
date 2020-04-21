#include "ogm/interpreter/Variable.hpp"

// we need these include files because several fn_* files reference shared enums.
#include "ogm/asset/AssetObject.hpp"
#include "ogm/interpreter/Network.hpp"

// function with fixed argument count
#define FNDEF0(name) void name(VO);
#define FNDEF1(name, a) void name(VO, V);
#define FNDEF2(name, a, b) void name(VO, V, V);
#define FNDEF3(name, a, b, c) void name(VO, V, V, V);
#define FNDEF4(name, a, b, c, d) void name(VO, V, V, V, V);
#define FNDEF5(name, a, b, c, d, e) void name(VO, V, V, V, V, V);
#define FNDEF6(name, a, b, c, d, e, f) void name(VO, V, V, V, V, V, V);
#define FNDEF7(name, a, b, c, d, e, f, g) void name(VO, V, V, V, V, V, V, V);
#define FNDEF8(name, a, b, c, d, e, f, g, h) void name(VO, V, V, V, V, V, V, V, V);
#define FNDEF9(name, a, b, c, d, e, f, g, h, i) void name(VO, V, V, V, V, V, V, V, V, V);
#define FNDEF10(name, a, b, c, d, e, f, g, h, i, j) void name(VO, V, V, V, V, V, V, V, V, V, V);
#define FNDEF11(name, a, b, c, d, e, f, g, h, i, j, k) void name(VO, V, V, V, V, V, V, V, V, V, V, V);
#define FNDEF12(name, a, b, c, d, e, f, g, h, i, j, k, l) void name(VO, V, V, V, V, V, V, V, V, V, V, V, V);
#define FNDEF13(name, a, b, c, d, e, f, g, h, i, j, k, l, m) void name(VO, V, V, V, V, V, V, V, V, V, V, V, V, V);
#define FNDEF14(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n) void name(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V);
#define FNDEF15(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) void name(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V);
#define FNDEF16(name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) void name(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V);
// variadic function
#define FNDEFN(name) void name(VO, unsigned char argc, const Variable*);

// writeable variable
#define SETVAR(name) namespace setv {void name(V);}
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
#define CONST(name, value) namespace constant{static const auto name = value;}
// looks up "src" as "dst"
#define ALIAS(dst, src)

namespace ogm { namespace interpreter
{
    typedef const Variable& V;
    typedef Variable& VO;
    namespace fn
    {
        
// ---------------------------------------------------------------------------
        