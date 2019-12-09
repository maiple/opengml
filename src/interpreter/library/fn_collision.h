FNDEF2(place_empty, x, y)
FNDEF3(place_empty, x, y, ex)
FNDEF2(place_free, x, y)
FNDEF3(place_meeting, x, y, ex)
FNDEF2(position_empty, x, y)
FNDEF3(position_empty, x, y, ex)
FNDEF3(position_meeting, x, y, ex)
FNDEF3(instance_position, x, y, object)
FNDEF3(instance_place, x, y, object)

FNDEF7(collision_rectangle, x1, y1, x2, y2, obj, prec, notme)
FNDEF7(collision_ellipse, x1, y1, x2, y2, obj, prec, notme)
FNDEF6(collision_circle, x, y, r, obj, prec, notme)
FNDEF5(collision_point, x, y, obj, prec, notme)
FNDEF7(collision_line, x1, y1, x2, y2, obj, prec, notme)

// requires knowledge of collision bbox.
FNDEF2(distance_to_point, x, y)
FNDEF1(distance_to_object, obj)
