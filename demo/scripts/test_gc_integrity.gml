if (!ogm_garbage_collector_enabled()) exit;

arrA = [0, 2]
arrB = [arrA]
arrA[@ 0] = arrB
arrA[@ 1] = 3;
ogm_assert(arrB[0][1] == 3)

show_debug_message(ogm_garbage_collector_graph());
ogm_assert(ogm_garbage_collector_count() == 2);
ogm_assert(ogm_garbage_collector_process() == 0);

// unlink
arrA = 0;
arrB = 0;

show_debug_message("------");
show_debug_message(ogm_garbage_collector_graph());
ogm_assert(ogm_garbage_collector_process() == 2);
ogm_assert(ogm_garbage_collector_count() == 0);

ogm_expected(
"
Node 0 (root)
  -> 1

Node 1 (root)
  -> 0

------
Node 0
  -> 1

Node 1
  -> 0
"
)