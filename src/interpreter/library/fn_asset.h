FNDEF1(asset_get_type, asset)
FNDEF1(object_exists, obj)
FNDEF1(object_get_parent, obj)
FNDEF1(object_get_name, obj)
FNDEF1(object_get_sprite, obj)
FNDEF1(object_get_mask, obj)
FNDEF1(object_get_depth, obj)
FNDEF1(object_get_persistent, obj)
FNDEF1(object_get_solid, obj)
FNDEF1(object_get_visible, obj)
FNDEF2(object_is_ancestor, obj, ancestor)

FNDEF2(object_set_sprite, obj, spr)
FNDEF2(object_set_mask, obj, msk)
FNDEF2(object_set_persistent, obj, prs)
FNDEF2(object_set_solid, obj, sol)
FNDEF2(object_set_depth, obj, spr)
FNDEF2(object_set_visible, obj, vis)

FNDEF1(room_exists, rm)
FNDEF1(room_get_name, rm)
FNDEF0(room_add)
FNDEF1(room_duplicate, rm)
FNDEF1(room_instance_clear, rm)
FNDEF4(room_instance_add, rm, x, y, obj)
FNDEF1(room_tile_clear, rm)
FNDEF9(room_tile_add, rm, bg, left, top, w, h, x, y, depth)
FNDEF12(room_tile_add_ext, rm, bg, left, top, w, h, x, y, depth, xscale, yscale, alpha)
FNDEF12(room_set_background, rm, index, visible, fg, id, x, y, htile, vtile, hspeed, vspeed, alpha)
FNDEF3(room_set_background_colour, rm, col, show)
ALIAS(room_set_background_colour, room_set_background_color)
FNDEF2(room_set_width, rm, w)
FNDEF2(room_set_height, rm, h)
FNDEF2(room_set_view_enabled, rm, enable)

FNDEF1(asset_get_index, a)
FNDEF1(script_exists, n)
FNDEF1(script_get_name, n)
FNDEFN(script_execute)

FNDEF1(sprite_exists, s)
FNDEF1(sprite_get_number, s)
FNDEF1(sprite_get_xoffset, s)
FNDEF1(sprite_get_yoffset, s)
FNDEF1(sprite_get_width, s)
FNDEF1(sprite_get_height, s)
FNDEF1(sprite_get_bbox_left, s)
FNDEF1(sprite_get_bbox_top, s)
FNDEF1(sprite_get_bbox_right, s)
FNDEF1(sprite_get_bbox_bottom, s)
FNDEF1(sprite_get_speed, s)
FNDEF1(sprite_get_speed_type, s)
FNDEF3(sprite_set_speed, s, spd, spdtype)
FNDEF9(sprite_create_from_surface, surface, x, y, w, h, removebg, smooth, xo, yo)
FNDEF8(sprite_add_from_surface, sprite, surface, x, y, w, h, removebg, smooth)
FNDEF6(sprite_add, sprite, imgnum, removebg, smooth, xo, yo)

CONST(spritespeed_framespersecond, 0)
CONST(spritespeed_framespergameframe, 1)

FNDEF4(font_add_sprite, sprite, first, prop, sep)
FNDEF4(font_add_sprite_ext, sprite, str, prop, sep)

FNDEF1(background_exists, b)
FNDEF1(background_get_width, b)
FNDEF1(background_get_height, b)
FNDEF1(background_get_name, b)
FNDEF1(background_duplicate, b)

// ogm-only functions:
FNDEF1(room_get_view_enabled, r)
FNDEF2(room_get_view_visible, r, i)
FNDEF2(room_get_view_wview, r, i)
FNDEF2(room_get_view_hview, r, i)
FNDEF1(room_get_width, r)
FNDEF1(room_get_height, r)

// ACCURACY: check these for accuracy
// (if modified, also edit the implementation for asset_get_type!)
CONST(asset_sprite, 0)
CONST(asset_object, 1)
CONST(asset_background, 2)
CONST(asset_tiles, 2) // same thing as background
CONST(asset_sound, 3)
CONST(asset_room, 4)
CONST(asset_path, 5)
CONST(asset_timeline, 6)
CONST(asset_font, 7)
CONST(asset_shader, 8)
CONST(asset_script, 9)
CONST(asset_unknown, -1)
