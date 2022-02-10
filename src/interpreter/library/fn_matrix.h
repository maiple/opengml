///// trigonometric functions /////

FNDEF1(matrix_get, index)
FNDEF2(matrix_set, index, matrix)

FNDEF9(matrix_build, x, y, z, xa, ya, za, xs, ys, zs)
FNDEF0(matrix_build_identity)
FNDEF4(matrix_build_projection_ortho, w, h, zclose, zfar)
FNDEF4(matrix_build_projection_perspective, w, h, zclose, zfar)
FNDEF4(matrix_transform_vertex, matrix, x, y, z)

FNDEF2(matrix_multiply, mata, matb)

CONST(matrix_view, 0)
CONST(matrix_projection, 1)
CONST(matrix_world, 2)
