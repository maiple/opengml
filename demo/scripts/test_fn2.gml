var fn = function(a, b, c) {
    a *= b * (b - 1);
    show_debug_message(a);
    show_debug_message(c + a);
    return c + a;
}

var z = fn(2, 3, 99);

ogm_assert(z == (2 * 3 * 2) + 99);

ogm_expected("
12
111
")