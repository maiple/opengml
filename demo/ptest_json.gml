var map = ds_map_create();
var list = ds_list_create();
map[? "test"] = 4;
map[? "bc"] = "hello"
list[|5] = 2;
list[|7] = "test";

ds_map_add_list(map, "list", list);

var s = json_encode(map);

show_debug_message(s);

var _map = json_decode(s);

ogm_ds_info(ds_type_map, _map);
ogm_ds_info(ds_type_list, _map[? "list"]);
