var obj = {
    h: 2
};

show_debug_message("obj1 created");
show_debug_message("obj.h: " + string(obj.h));

var obj2 = obj;
show_debug_message("obj2 linked to obj1");

obj.h = 1;
show_debug_message("obj.h: " + string(obj.h));
show_debug_message("obj2.h: " + string(obj2.h));

with (obj2)
{
    show_debug_message("self(obj2).h: " + string(self.h));
}

ogm_expected("
obj1 created
obj.h: 2
obj2 linked to obj1
obj.h: 1
obj2.h: 1
self(obj2).h: 1
")