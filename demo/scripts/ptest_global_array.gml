global.z[2] = 1;
global.z[5] = 4;
global.z[2, 3] = "a"
show_debug_message(global.z[2])
show_debug_message(++global.z[5])
show_debug_message(global.z[2, 3])
show_debug_message(global.z[2, 1])