if (!ogm_garbage_collector_enabled()) exit;

ogm_assert(ogm_garbage_collector_process() == 0)
ogm_assert(ogm_garbage_collector_count() == 0)

for (var i = 0; i < 4; i++)
{
    for (var j = 0; j <= 12; j++)
    {
        global.ammo[i][j] = 0;
    }
}

ogm_assert(ogm_garbage_collector_node_outgoing_count(global.ammo) == 4)

a = global.ammo

proc = ogm_garbage_collector_process();
ogm_assert(proc == 0)
ogm_assert(ogm_garbage_collector_count() == 5)

global.ammo[@ 0, 0] = 0;

ogm_assert(ogm_garbage_collector_process() == 0)
ogm_assert(ogm_garbage_collector_count() == 5)

global.ammo = 0;
a = 0;

ogm_assert(ogm_garbage_collector_process() == 5)
ogm_assert(ogm_garbage_collector_count() == 0)

var l = [1, 2, 3]
for (var i = 0; i < 4; i++)
{
    global.b[i] = l;
}
global.b[0] = 0;
l = 0;

ogm_assert(ogm_garbage_collector_process() == 0)
ogm_assert(ogm_garbage_collector_count() == 2)

global.b = 0;

ogm_assert(ogm_garbage_collector_process() == 2)
ogm_assert(ogm_garbage_collector_count() == 0)

show_debug_message("GC nest test complete.")
