FNDEF3(make_colour_rgb, r, g, b)
FNDEF3(make_colour_hsv, h, s, v)
FNDEF3(merge_colour, a, b, x)

FNDEF1(colour_get_red, c)
FNDEF1(colour_get_blue, c)
FNDEF1(colour_get_green, c)
FNDEF1(colour_get_hue, c)
FNDEF1(colour_get_saturation, c)
FNDEF1(colour_get_value, c)


ALIAS(make_colour_rgb, make_color_rgb)
ALIAS(make_colour_hsv, make_color_hsv)
ALIAS(merge_colour, merge_color)
ALIAS(colour_get_red, color_get_red)
ALIAS(colour_get_blue, color_get_blue)
ALIAS(colour_get_green, color_get_green)
ALIAS(colour_get_hue, color_get_hue)
ALIAS(colour_get_saturation, color_get_saturation)
ALIAS(colour_get_value, color_get_value)

// greyscale
CONST(c_white, 0xffffff)
CONST(c_black, 0)
CONST(c_dkgrey, 0x404040)
CONST(c_dkgray, 0x404040)
CONST(c_grey, 0x808080)
CONST(c_gray, 0x808080)
CONST(c_ltgrey, 0xC0C0C0)
CONST(c_ltgray, 0xC0C0C0)
CONST(c_silver, 0xC0C0C0)

// primary colours
CONST(c_blue, 0xff0000)
CONST(c_lime, 0x00ff00)
CONST(c_red, 0x0000ff)

// dark primary
CONST(c_navy, 0x800000)
CONST(c_green, 0x008000)
CONST(c_maroon, 0x000080)

// secondary
CONST(c_aqua, 0xffff00)
CONST(c_fuchsia, 0xff00ff)
CONST(c_yellow, 0x00ffff)

// dark secondary
CONST(c_olive, 0x008080)
CONST(c_purple, 0x800080)
CONST(c_teal, 0x808000)

// misc
CONST(c_orange, 0x40A0ff)
