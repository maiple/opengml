a = 0;
if (true)
    ++a;


show_debug_message(a);

ogm_expected("
1
")
