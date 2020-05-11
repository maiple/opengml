var mstr = ">"
for (var i = 0; i < 20; ++i)
{
    str = "_"
    if (i == 0) then
    {
        str = "A"
    }
    else if (i == 1) str = "B"
    else if (i == 2) str = "C"
    else if (i >= 4 && i < 8) str = "D"
    else if (i == 10) str = "E"
    else if (i == 13 || i == 15 || i == 17) {
        str = "F"
    }
    else
    {
        str = "-"
    }
    mstr += str;
}

show_debug_message(mstr + "<");

ogm_expected(
">ABC-DDDD--E--F-F-F--<"
)