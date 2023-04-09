#ifdef OGM_LAYERS
FNDEF4(instance_create_layer, x, y, object, layer)

FNDEF1(layer_set_target_room, room)
FNDEF0(layer_reset_target_room)
FNDEF0(layer_get_target_room)
FNDEF1(layer_exists, lid)
FNDEF1(layer_get_id, lid)
FNDEF1(layer_get_depth, lid)
#else
IGNORE_WARN(instance_create_layer)
IGNORE_WARN(layer_get_id)
IGNORE_WARN(layer_get_depth)
IGNORE_WARN(layer_exists)
IGNORE_WARN(layer_set_target_room)
IGNORE_WARN(layer_reset_target_room)
IGNORE_WARN(get_target_room)
#endif