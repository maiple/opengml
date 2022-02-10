#macro TEST_MACRO "hello from a macro!"

show_debug_message(TEST_MACRO)

ogm_expected("
hello from a macro!
")
