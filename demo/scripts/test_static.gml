var inc = function()
{
    static X = 10;
    X++;
    return X;
}

show_debug_message(inc());
show_debug_message(inc());
show_debug_message(inc());
show_debug_message(inc());

ogm_expected("
11
12
13
14
")