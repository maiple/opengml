show_debug_message(md5_string_utf8(""))
show_debug_message(md5_string_utf8("a"))
show_debug_message(md5_string_utf8("我"))

ogm_expected("
d41d8cd98f00b204e9800998ecf8427e
0cc175b9c0f1b6a831c399e269772661
16815254531798dc21ee979d1d9c6675
")