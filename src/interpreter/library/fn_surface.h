FNDEF2(surface_create, w, h)
FNDEF1(surface_exists, id)
FNDEF3(surface_resize, id, w, h)
FNDEF1(surface_set_target, id)
FNDEF0(surface_reset_target)
FNDEF1(surface_free, id)
FNDEF1(surface_get_texture, id)
FNDEF1(surface_get_width, id)
FNDEF1(surface_get_height, id)
FNDEF3(surface_getpixel, id, x, y)
FNDEF3(surface_getpixel_ext, id, x, y)

GETVAR(application_surface)
FNDEF1(application_surface_enable, enable)
FNDEF0(application_surface_is_enabled)
FNDEF1(application_surface_draw_enable, enable)

// x1, y1, x2, y2 of where to draw the surface on-screen to retain aspect ratio
FNDEF0(application_get_position)
