if (!ogm_garbage_collector_enabled()) exit;

// remove link
var arr50 = [0, 1, 2]
inst_arr51 = [arr50, 1, 2]

ogm_assert(ogm_garbage_collector_process() == 0)

arr50 = 0;

ogm_assert(ogm_garbage_collector_process() == 0)
inst_arr51[0] = 0 // remove link
ogm_assert(ogm_garbage_collector_process() == 1, "failed to delete array after link removed.")
inst_arr51 = 0
ogm_assert(ogm_garbage_collector_process() == 1)

show_debug_message("GC remove link test complete.")
