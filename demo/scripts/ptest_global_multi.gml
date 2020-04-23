a = 3;
global.c = 5;
global.d = 2
global.z = [];
global.z[0, 0] = 3
show_debug_message(string(global.z[0, 0]) + string((global).z[0, 0]) + string((-5).z[0, 0]));
ogm_expected("333")