FNDEF3(instance_create, x, y, object)
FNDEF0(instance_destroy)
FNDEF1(instance_destroy, instance)
FNDEF1(instance_exists, instance)
FNDEF1(instance_number, object)
FNDEF2(instance_find, object, i)
GETVAR(instance_count)
GETVARA(instance_id)
FNDEF1(instance_id_get, a)
FNDEF3(instance_nearest, x, y, obj)

// instance activation/deactivation

FNDEF0(instance_activate_all)
FNDEF1(instance_activate_object,)
FNDEF6(instance_activate_region, x, y, w, h, inside, notme)
FNDEF1(instance_deactivate_all,)
FNDEF1(instance_deactivate_object,)
FNDEF6(instance_deactivate_region, x, y, w, h, inside, notme)