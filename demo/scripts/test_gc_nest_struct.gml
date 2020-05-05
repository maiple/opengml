if (!ogm_garbage_collector_enabled()) exit;

o = {
    arr: [0, 1, 2, 3],
}

show_debug_message(typeof(o))

show_debug_message(ogm_garbage_collector_graph())

ogm_assert(ogm_garbage_collector_count() == 2);
ogm_assert(ogm_garbage_collector_process() == 0);
ogm_assert(ogm_garbage_collector_node_outgoing_count(o) == 1);

o.s = o;
o.arr[2] = o;
o.arr[3] = o;
o.arr[2] = o;
ogm_assert(ogm_garbage_collector_node_outgoing_count(o) == 2);

o = 0;

show_debug_message("--------")
show_debug_message(ogm_garbage_collector_graph())

ogm_assert(ogm_garbage_collector_count() == 2);
ogm_assert(ogm_garbage_collector_process() == 2);
ogm_assert(ogm_garbage_collector_count() == 0);

show_debug_message("--------")
show_debug_message(ogm_garbage_collector_graph())

ogm_expected(
"
object
Node 0 (root)
  -> 1

Node 1

--------
Node 0
  -> 1
  -> 0

Node 1
  -> 0 (x2)

--------
"
)