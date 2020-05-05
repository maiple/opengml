if (!ogm_garbage_collector_enabled()) exit;

o = {}
b = {}
o.ref = b;
b.ref = o;

show_debug_message(ogm_garbage_collector_graph());
ogm_assert(ogm_garbage_collector_count() == 2);
ogm_assert(ogm_garbage_collector_process() == 0);

// unlink
o = 0;
b = 0;

show_debug_message("-----------");
show_debug_message(ogm_garbage_collector_graph());
ogm_assert(ogm_garbage_collector_process() == 2);
ogm_assert(ogm_garbage_collector_count() == 0);