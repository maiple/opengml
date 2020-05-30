var iterations = floor(100000 + random(4));

// warm up cache?
for (var i = 0; i < iterations; ++i)
{
    ogm_volatile(
        random(1) * random(1) * random(1) * random(1)
    )
}

var start_time = get_timer();
var a = random(1);
var b = random(1);
var c = random(1);
var d = random(1);
for (var i = 0; i < iterations; ++i)
{
    ogm_volatile(
        ogm_volatile(a) * ogm_volatile(b) * ogm_volatile(c) * ogm_volatile(d)
    )
}

var reference_time = get_timer() - start_time;
show_debug_message("o0: " + string(reference_time/1000) + " ms");

/////////////////////////////////////////////////////////////////////////

start_time = get_timer();
var a = random(1);
var b = random(1);
var c = random(1);
var d = random(1);
for (var i = 0; i < iterations; ++i)
{
    ogm_volatile(
        a * b * c * d
    )
}
var o2_time = get_timer() - start_time;
show_debug_message("o2: " + string(o2_time/1000) + " ms");

/////////////////////////////////////////////////////////////////////////

start_time = get_timer();
for (var i = 0; i < iterations; ++i)
{
    ogm_volatile(
        4.165234 * 3.15625243 * 2.263523 * 1.5234
    )
}
var o1_time = get_timer() - start_time;
show_debug_message("o1: " + string(o1_time/1000) + " ms");