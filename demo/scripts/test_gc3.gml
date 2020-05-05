// ensure number of deletions by GC is as expected.

if (!ogm_garbage_collector_enabled()) exit;

// array copy-on-write

inst_arr60 = [0, 1, 2]; // -- A --
inst_arr62 = [inst_arr60]; // -- B --
inst_arr60[@ 0] = inst_arr62; // circular reference
inst_arr61 = inst_arr60;

// clear references
inst_arr60 = 0;
inst_arr62 = 0;

// nothing cleared because arr61 still points to array A
ogm_assert(ogm_garbage_collector_process() == 0)

// copies array, removing reference to previous arrays
inst_arr61[0] = 2; // -- C --

ogm_assert(ogm_garbage_collector_process() == 2) // deletes A and B

inst_arr61[0] = 3; // another change has no effect.
ogm_assert(ogm_garbage_collector_process() == 0)

inst_arr61 = 0; // clear array.
ogm_assert(ogm_garbage_collector_process() == 1)
ogm_assert(ogm_garbage_collector_process() == 0)

show_debug_message("GC test 3 complete.")