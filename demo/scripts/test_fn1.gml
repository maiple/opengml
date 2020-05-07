var fn = function() {
    show_debug_message("hello from function.")
}

show_debug_message("hello from main.")

fn();

ogm_expected("
hello from main.
hello from function.
")