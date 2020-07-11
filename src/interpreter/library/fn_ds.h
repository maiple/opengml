FNDEF2(ds_exists, id, type)

CONST(ds_type_map, 0)
CONST(ds_type_list, 1)
CONST(ds_type_stack, 2)
CONST(ds_type_grid, 3)
CONST(ds_type_queue, 4)
CONST(ds_type_priority, 5)

///// list data structure /////

//! returns list id, guaranteed to be positive
FNDEF0(ds_list_create)
FNDEF1(ds_list_destroy, id)
FNDEF1(ds_list_clear, id)
FNDEF1(ds_list_empty, id)
FNDEF1(ds_list_size, id)
FNDEF2(ds_list_add, id, value)
FNDEFN(ds_list_add)
FNDEF3(ds_list_set, id, pos, value)
FNDEF2(ds_list_delete, id, pos)
FNDEF2(ds_list_find_index, id, val)
FNDEF2(ds_list_find_value, id, val)
FNDEF3(ds_list_insert, id, pos, value)
FNDEF3(ds_list_replace, id, pos, value)
FNDEF1(ds_list_shuffle, id)
FNDEF1(ds_list_sort, id)
FNDEF2(ds_list_copy, dst, src)
FNDEF2(ds_list_mark_as_map, id, index)
FNDEF2(ds_list_mark_as_list, id, index)

///// map data structure /////

FNDEF0(ds_map_create)
FNDEF1(ds_map_destroy, id)
FNDEF1(ds_map_clear, id)
FNDEF2(ds_map_copy, id, id2)
FNDEF2(ds_map_exists, id, key)
FNDEF3(ds_map_add, id, key, val)
FNDEF3(ds_map_replace, id, key, value)
ALIAS(ds_map_replace, ds_map_set)
FNDEF2(ds_map_delete, id, key)
FNDEF1(ds_map_empty, id)
FNDEF1(ds_map_size, id)
FNDEF1(ds_map_find_first, id)
FNDEF1(ds_map_find_last, id)
FNDEF2(ds_map_find_value, id, key)
FNDEF2(ds_map_find_next, id, key)
FNDEF2(ds_map_find_previous, id, key)
FNDEF3(ds_map_add_list, id, key, value)
FNDEF3(ds_map_add_map, id, key, value)
FNDEF3(ds_map_replace_list, id, key, value)
FNDEF3(ds_map_replace_map, id, key, value)
FNDEF1(ds_map_write, id)
FNDEF1(ds_map_read, str)
FNDEF2(ds_map_read, str, fmt)

FNDEF2(ds_map_secure_save, id, file)
FNDEF2(ds_map_secure_save_buffer, id, buffer)
FNDEF1(ds_map_secure_load, file)
FNDEF1(ds_map_secure_load_buffer, buffer)

///// stack data structure /////

FNDEF0(ds_stack_create)
FNDEF1(ds_stack_destroy, id)
FNDEF1(ds_stack_clear, id)
FNDEF1(ds_stack_empty, id)
FNDEF1(ds_stack_size, id)
FNDEF1(ds_stack_top, id)
FNDEF1(ds_stack_pop, id)
FNDEF2(ds_stack_push, id, value)

///// grid data structure /////

FNDEF2(ds_grid_create, w, h)
FNDEF1(ds_grid_destroy, i)
FNDEF3(ds_grid_get, i, x, y)
FNDEF4(ds_grid_set, i, x, y, value)
FNDEF4(ds_grid_add, i, x, y, value)
FNDEF1(ds_grid_height, i)
FNDEF1(ds_grid_width, i)

///// priority queue data structure /////

FNDEF0(ds_priority_create)
FNDEF1(ds_priority_empty, id)
FNDEF3(ds_priority_add, id, value, priority)
FNDEF1(ds_priority_delete_min, id)
FNDEF1(ds_priority_delete_max, id)
FNDEF1(ds_priority_destroy, id)