var _x = "@@Difficulty: "

for (var i = 0; i < 2; ++i)
{
    var text = _x;
    show_debug_message("1")
    text = string_copy(text, 3);
    show_debug_message("2")
    text += "normal"
    show_debug_message("3")
    show_debug_message(text)
}
