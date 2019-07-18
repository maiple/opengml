FNDEF0(draw_get_alpha)
FNDEF0(draw_get_colour)
FNDEF1(draw_set_alpha, a)
FNDEF1(draw_set_colour, c)
ALIAS(draw_set_colour, draw_set_color)
ALIAS(draw_get_colour, draw_get_color)

FNDEF5(draw_rectangle, x1, y1, x2, y2, outline)
FNDEF4(draw_circle, x, y, r, outline)
FNDEF6(draw_circle_colour, x, y, r, c1, c2, outline)
FNDEF1(draw_set_circle_precision, prec)
ALIAS(draw_circle_colour, draw_circle_color)

// sprites
FNDEF0(draw_self)
FNDEF4(draw_sprite, sprite, subimg, x, y)
FNDEF9(draw_sprite_ext, sprite, subimg, x, y, xscale, yscale, angle, c, alpha)
FNDEF8(draw_sprite_part, sprite, subimg, left, top, width, height, x, y)
FNDEF12(draw_sprite_part_ext, sprite, subimg, left, top, width, height, x, y, xscale, yscale, c, alpha)
FNDEF16(draw_sprite_general, sprite, subimg, left, top, width, height, x, y, xscale, yscale, angle, c1, c2, c3, c4, alpha)
FNDEF6(draw_sprite_stretched, sprite, subimg, x, y, w, h)
FNDEF8(draw_sprite_stretched_ext, sprite, subimg, x, y, w, h, c, alpha)
FNDEF11(draw_sprite_pos, sprite, subimg, x1, y1, x2, y2, x3, y3, x4, y4, alpha)

// backgrounds
FNDEF3(draw_background, background, x, y)
FNDEF8(draw_background_ext, background, x, y, xscale, yscale, angle, c, alpha)
FNDEF7(draw_background_ext, background, x, y, xscale, yscale, c, alpha)
FNDEF7(draw_background_part, background, left, top, width, height, x, y)
FNDEF11(draw_background_part_ext, background, left, top, width, height, x, y, xscale, yscale, c, alpha)
FNDEF14(draw_background_general, background, left, top, width, height, x, y, xscale, yscale, c1, c2, c3, c4, alpha)
FNDEF5(draw_background_stretched, background, x, y, w, h)
FNDEF7(draw_background_stretched_ext, background, x, y, w, h, c, alpha)
FNDEF10(draw_background_pos, background, x1, y1, x2, y2, x3, y3, x4, y4, alpha)

// text
FNDEF3(draw_text, x, y, text)
FNDEF1(draw_set_halign, a)
FNDEF1(draw_set_valign, a)

FNDEF1(draw_clear, c)
FNDEF2(draw_clear_alpha, c, a)
FNDEF1(draw_enable_alphablend, y)

CONST(fa_left, 0)
CONST(fa_center, 1)
CONST(fa_right, 2)

CONST(fa_top, 0)
CONST(fa_middle, 1)
CONST(fa_bottom, 2)
