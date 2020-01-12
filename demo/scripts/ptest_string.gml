ogm_expected("
9
0
0
")

var s = "test abc<>";

show_debug_message(string_pos("<", s));
show_debug_message(string_pos("x", s));
show_debug_message(string_pos("<a", s));
