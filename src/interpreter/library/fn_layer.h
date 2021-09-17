#ifdef OGM_LAYERS
FNDEF1(layer_set_target_room, room)
FNDEF0(layer_reset_target_room)
FNDEF0(layer_get_target_room)
#else
IGNORE_WARN(layer_set_target_room)
IGNORE_WARN(layer_reset_target_room)
IGNORE_WARN(get_target_room)
#endif