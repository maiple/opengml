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

///// map data structure /////

FNDEF0(ds_map_create)
FNDEF1(ds_map_destroy, id)
FNDEF1(ds_map_clear, id)
FNDEF2(ds_map_copy, id, id2)
FNDEF2(ds_map_exists, id, key)
FNDEF3(ds_map_add, id, key, val)
FNDEF3(ds_map_replace, id, key, value)
FNDEF2(ds_map_delete, id, key)
FNDEF1(ds_map_empty, id)
FNDEF1(ds_map_size, id)
FNDEF1(ds_map_find_first, id)
FNDEF1(ds_map_find_last, id)
FNDEF2(ds_map_find_value, id, key)
FNDEF2(ds_map_find_next, id, key)
FNDEF2(ds_map_find_previous, id, key)

///// grid data structure /////

FNDEF2(ds_grid_create, w, h)
FNDEF1(ds_grid_destroy, i)
FNDEF3(ds_grid_get, i, x, y)
FNDEF4(ds_grid_set, i, x, y, value)
FNDEF4(ds_grid_add, i, x, y, value)
FNDEF1(ds_grid_height, i)
FNDEF1(ds_grid_width, i)