v = ds_map_create();

v[? "hi"] = 9;
v[? 8] = 3;

show_debug_message(v[? 8])
show_debug_message(v[? "hi"])
