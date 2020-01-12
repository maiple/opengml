ogm_expected("
-5
8
65
")


var b = buffer_create(32, buffer_fixed, 4);
buffer_write(b, buffer_s32, -5)
buffer_write(b, buffer_s32, 8)
buffer_write(b, buffer_s32, 65)
buffer_seek(b, buffer_seek_start, 0);
for (var i = 0; i < 3; ++i)
{
    show_debug_message(buffer_read(b, buffer_s32))
}
