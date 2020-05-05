// ensure number of deletions by GC is as expected.

if (!ogm_garbage_collector_enabled()) exit;

var arr50 = [0, 1, 2]
inst_arr51 = [arr50, 1, 2]

ogm_assert(ogm_garbage_collector_process() == 0)

// unlink
arr50 = 0;

show_debug_message(ogm_garbage_collector_graph())

ogm_assert(ogm_garbage_collector_process() == 0)
inst_arr51[0] = 0 // remove link

show_debug_message("-------")
show_debug_message(ogm_garbage_collector_graph())

ogm_assert(ogm_garbage_collector_process() == 1, "failed to delete array after link removed.")
ogm_assert(ogm_garbage_collector_process() == 0)

show_debug_message("-------")
show_debug_message(ogm_garbage_collector_graph())

inst_arr51 = 0
ogm_assert(ogm_garbage_collector_process() == 1)
ogm_assert(ogm_garbage_collector_process() == 0)

show_debug_message("GC test 2 complete.")
