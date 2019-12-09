// credits to YellowAfterlife for this

var q = 1;
show_debug_message(is_real(q))
show_debug_message(is_int64(q))
show_debug_message(is_bool(q))
show_debug_message("")

q = q << 1;
show_debug_message(is_real(q))
show_debug_message(is_int64(q))
show_debug_message(is_bool(q))
show_debug_message("")

q = q > 1;
show_debug_message(is_real(q))
show_debug_message(is_int64(q))
show_debug_message(is_bool(q))

ogm_expected("
1
0
0

0
1
0

0
0
1
")
