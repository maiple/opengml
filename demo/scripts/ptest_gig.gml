// will be swapped out for gig.so on linux automatically.

var hello = external_define(program_directory + "/gig.dll", "gig_hello", dll_cdecl, ty_string, 0);

show_debug_message(external_call(hello));