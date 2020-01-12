var z;
z[2] = 1;
z[5] = 4;
z[2, 3] = "a"
show_debug_message("expect: 1, 5, a, 0")
show_debug_message(z[2])
show_debug_message(++z[5])
show_debug_message(z[2, 3])
show_debug_message(z[2, 1])

var w = z;
w[@2] = 3
show_debug_message("expect: 3, 3")
show_debug_message(w[2])
show_debug_message(z[2])

w[2] = 4;
show_debug_message("expect: 4, 3")
show_debug_message(w[2])
show_debug_message(z[2])

w[@2] = 5;
show_debug_message("expect: 5, 3")
show_debug_message(w[2])
show_debug_message(z[2])