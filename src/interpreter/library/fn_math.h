///// trigonometric functions /////

FNDEF1(arccos, theta)
FNDEF1(arcsin, theta)
FNDEF1(arctan, theta)
FNDEF2(arctan2, x, y)
FNDEF1(sin, theta)
FNDEF1(tan, theta)
FNDEF1(cos, theta)
FNDEF1(darccos, theta)
FNDEF1(darcsin, theta)
FNDEF1(darctan, theta)
FNDEF2(darctan2, x, y)
FNDEF1(dsin, theta)
FNDEF1(dtan, theta)
FNDEF1(dcos, theta)
FNDEF1(degtorad, theta)
FNDEF1(radtodeg, theta)
FNDEF2(lengthdir_x, len, dir)
FNDEF2(lengthdir_y, len, dir)


///// rounding functions /////

FNDEF1(round, v)
FNDEF1(floor, v)
FNDEF1(frac, v)
FNDEF1(abs, v)
FNDEF1(sign, v)
FNDEF1(ceil, v)
FNDEFN(max)
FNDEFN(mean)
FNDEFN(median)
FNDEFN(min)
FNDEF3(lerp, a, b, amt)
FNDEF3(clamp, val, min, max)


///// misc functions /////

FNDEF1(exp, v)
FNDEF1(ln, v)
FNDEF2(power, base, exponent)
FNDEF1(sqr, v)
FNDEF1(sqrt, v)
FNDEF1(log2, v)
FNDEF1(log10, v)
FNDEF2(logn, v, n)


///// geometry functions /////
FNDEF6(point_in_rectangle, px, py, x1, y1, x2, y1)


///// randomness functions /////

FNDEFN(choose)
FNDEF1(irandom, v)
FNDEF1(random, v)
FNDEF2(irandom_range, a, b)
FNDEF2(random_range, a, b)
FNDEF0(randomize)
FNDEF1(random_set_seed, r)
FNDEF0(random_get_seed)
ALIAS(randomize, randomise)


///// vector functions /////

FNDEF4(point_direction, x1, y1, x2, y2)
FNDEF4(point_distance, x1, y1, x2, y2)
FNDEF6(point_distance_3d, x1, y1, z1, x2, y2, z2)
FNDEF4(dot_product, x1, y1, x2, y2)
FNDEF6(dot_product_3d, x1, y1, z1, x2, y2, z2)
FNDEF4(dot_product_normalised, x1, y1, x2, y2)
FNDEF6(dot_product_normalised_3d, x1, y1, z1, x2, y2, z2)
FNDEF2(angle_difference, a1, a2)

CONST(pi, 3.1415926535897932)
