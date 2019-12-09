var s = "test";

show_debug_message(s);
show_debug_message(s == "test")

var r = "te";

show_debug_message(r + "st");
show_debug_message(s == r + "st");
show_debug_message("");

show_debug_message(string_copy(s, 1, 4))
show_debug_message(string_copy(s, 1, 3))
show_debug_message(string_copy(s, 2, 2))
show_debug_message(string_copy(s, 2, 1))
show_debug_message(string_copy(s, 2, 0))
show_debug_message(string_copy(s, 2))
show_debug_message(string_copy(s))
show_debug_message(string_copy("test", 2, 2))

show_debug_message("");

var a = "test";
var b = a;
var c = a;
b += "1"
c += "2"
var d = b;
d += "3"
var e = string_copy(d, 2, 2);
show_debug_message(e);
var f = e + "*";
show_debug_message(a)
show_debug_message(b)
show_debug_message(c)
show_debug_message(d)
show_debug_message(e)
show_debug_message(f)

var q = "a";
repeat(100000) q += "b";
show_debug_message(int64(string_length(q)))

show_debug_message(string_delete("1234567", -2, 19))
show_debug_message(string_delete("1234567", 2, 3))
show_debug_message(string_delete("1234567", 5, 6))
show_debug_message(string_delete("1234567", 1, 3))

// double-check these expects, they might be wrong :C
ogm_expected("
test
True
test
True

test
tes
es
e

est
test
es

es
test
test1
test2
test13
es
es*
10001

1567
1234
4567
")
