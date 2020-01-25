FNDEF0(d3d_model_create)
FNDEF1(d3d_model_destroy, id)
FNDEF5(d3d_model_draw, id, x, y, z, texture)

FNDEF2(d3d_model_primitive_begin, id, glenum)
FNDEF3(d3d_model_vertex, x, y, z)
FNDEF5(d3d_model_vertex_colour, x, y, z, colour, alpha)
FNDEF5(d3d_model_vertex_texture, x, y, z, u, v)
FNDEF7(d3d_model_vertex_texture_colour, x, y, z, u, v, colour, alpha)
FNDEF0(d3d_model_primitive_end)

ALIAS(d3d_model_vertex_colour, d3d_model_vertex_color)
ALIAS(d3d_model_vertex_texture_colour, d3d_model_vertex_texture_color)

// TODO: add normal_ to the vertex functions above
// this will require adding support for normals to the default
// vertex format and to the model's own vertex format.