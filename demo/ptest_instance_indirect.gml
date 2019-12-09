self.instance_variable_z = 3;
show_debug_message("instance variable z is "
    + string(self.instance_variable_z)
    + " (accessed indirectly.)");

show_debug_message("instance variable z is "
    + string(instance_variable_z)
    + " (accessed directly.)");
