// ensure number of deletions by GC is as expected.

ogm_assert(ogm_garbage_collector_process() == 0);

var a = 4;

ogm_assert(ogm_garbage_collector_process() == 0);

// stack references to arrays are weak (non-root).
// This is because the GC only runs once per frame and
// from a context where it is assumed that there are *NO*
// stack-allocated arrays.
// Indeed, this is a very unsafe usage of the garbage collector (!)
// attempting to access or change the array after this will likely result in a crash.
var arr = [1, 2, 3, 4];

ogm_assert(ogm_garbage_collector_process() == 1);
ogm_assert(ogm_garbage_collector_process() == 0);

// instance gc test
instance_arr = [1, 2, 3, 4];

ogm_assert(ogm_garbage_collector_process() == 0);

instance_arr = 0;

ogm_assert(ogm_garbage_collector_process() == 1);
ogm_assert(ogm_garbage_collector_process() == 0);

// global gc test

global.global_arr = [1, 2, 3];

ogm_assert(ogm_garbage_collector_process() == 0);

global.global_arr = 1;

ogm_assert(ogm_garbage_collector_process() == 1);
ogm_assert(ogm_garbage_collector_process() == 0);

// more complicated scenario

var arr2 = [1, 2, 3]
instance_arr2 = arr2;
ogm_assert(ogm_garbage_collector_process() == 0);
var arr3 = arr2;
ogm_assert(ogm_garbage_collector_process() == 0);
arr2 = 0;
ogm_assert(ogm_garbage_collector_process() == 0);
ogm_assert(ogm_garbage_collector_process() == 0);
arr3 = 0;
ogm_assert(ogm_garbage_collector_process() == 0);
var arr4 = instance_arr2
ogm_assert(ogm_garbage_collector_process() == 0);
var arr5 = instance_arr2;
ogm_assert(ogm_garbage_collector_process() == 0);
instance_arr3 = instance_arr2;
ogm_assert(ogm_garbage_collector_process() == 0);
arr4 = 0;
ogm_assert(ogm_garbage_collector_process() == 0);
arr5 = 0;
ogm_assert(ogm_garbage_collector_process() == 0);
instance_arr2 = 0;
ogm_assert(ogm_garbage_collector_process() == 0);
instance_arr3 = 0;
ogm_assert(ogm_garbage_collector_process() == 1);

show_debug_message("GC test complete.")
