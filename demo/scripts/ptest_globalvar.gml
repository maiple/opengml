globalvar z = 3;
show_debug_message("globalvar z is " + string(z));
show_debug_message("global.z is " + string(z));

ogm_expected("
globalvar z is 3
global.z is 3
")
