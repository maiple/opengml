FNDEF3(instance_create, x, y, object)
FNDEF4(instance_create_depth, x, y, object, depth)
// instance_create_layer: see fn_layer
FNDEF1(instance_create, object)
FNDEF2(instance_change, object, events)
FNDEF1(instance_copy, events)
FNDEF0(instance_destroy)
FNDEF1(instance_destroy, instance)
FNDEF1(instance_exists, instance)
FNDEF1(instance_number, object)
FNDEF2(instance_find, object, i)
GETVAR(instance_count)
GETVARA(instance_id)
FNDEF1(instance_id_get, a)
FNDEF3(instance_nearest, x, y, obj)
FNDEF2(place_snapped, h, v)

// instance activation/deactivation

FNDEF0(instance_activate_all)
FNDEF1(instance_activate_object,)
FNDEF5(instance_activate_region, x, y, w, h, inside)
FNDEF1(instance_deactivate_all,)
FNDEF1(instance_deactivate_object,)
FNDEF6(instance_deactivate_region, x, y, w, h, inside, notme)

// data access

FNDEF1(alarm_get, index)
FNDEF2(alarm_set, index, value)