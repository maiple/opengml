view_xview[0, 0] = 5;
view_xview[0][0] = 3;
show_debug_message(view_xview[0])

self.alarm[0, 0] = 2;
self.alarm[0][0] = 2;
show_debug_message(self.alarm[0])
show_debug_message(self.alarm[0][0])
show_debug_message(self.alarm[0,0])

view_xview[0, 1] = 5;
view_xview[0][1] = 3;
show_debug_message(view_xview[1])

self.alarm[0, 1] = 2;
self.alarm[0][1] = 2;
show_debug_message(self.alarm[1])
show_debug_message(self.alarm[0][1])
show_debug_message(self.alarm[0,1])

ogm_expected("
3
2
2
2
3
2
2
2
")