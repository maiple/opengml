// gravity is a built-in variable required by ogme
self.gravity = 3;
show_debug_message("builtin variable gravity is "
    + string(self.gravity)
    + " (accessed indirectly.)");

ogm_expected("builtin variable gravity is 3 (accessed indirectly.)")
