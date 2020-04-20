show_debug_message("Please enter the path to a shared object library.");
var libpath = get_string("path");

// (fails if no library.)
ogm_external_list(libpath);

show_debug_message("")
show_debug_message("please enter commands of this form:")
show_debug_message("calltype returntype name value*")
show_debug_message('example: cdecl real testfn "hello"')

while (true)
{
    var line = get_string("path");
    var args = []
    while (string_length(line) > 0)
    {
        var i;
        var in_string = false;
        for (i = 0; i < string_length(line); ++i)
        {
            if (string_char_at(line, i + 1) == " " && !in_string)
            {
                break;
            }
            if (string_char_at(line, i + 1) == '"')
            {
                in_string ^= true;
            }
        }
        
        args[array_length(args)] = string_copy(line, 1, i) + (in_string ? '"' : "");
        line = string_copy(line, i + 2, string_length(line) - i);
    }
    
    if (array_length(args) < 3)
    {
        show_debug_message("Need at least three arguments: calltype, returntype, name");
    }
    else
    {
        var calltype = args[0];
        var type = args[1];
        var name = args[2];
        
        // parse calltype
        if (calltype == "cdecl" || calltype == "dll_cdecl")
        {
            calltype = dll_cdecl;
        }
        else if (calltype == "stdcall" || calltype == "dll_stdcall")
        {
            calltype = dll_stdcall;
        }
        else
        {
            show_debug_message("Unknown calltype " + calltype);
        }
        
        // parse type
        if (type == "real" || type == "ty_real")
        {
            type = ty_real;
        }
        else if (type == "string" || type == "ty_string")
        {
            type = ty_string;
        }
        else
        {
            show_debug_message("Unknown returntype " + type);
        }
        
        // args
        var argc = array_length(args) - 3;
        var argt = [ ]
        var arg = [ ];
        for (var i = 0; i < argc; ++i)
        {
            arg[i] = args[i + 3];
            argt[i] = ty_real;
            if (string_char_at(arg[i], 1) == '"')
            {
                arg[i] = string_copy(arg[i], 2, string_length(arg[i]) - 2);
                argt[i] = ty_string;
            }
            else
            {
                args[i] = real(arg[i]);
            }
        }
        
        switch(argc)
        {
        case 0:
            var fn = external_define(libpath, name, calltype, type, argc);
            if (fn < 0) break;
            show_debug_message(external_call(fn));
            external_free(fn);
            break;
        case 1:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0]));
            external_free(fn);
            break;
        case 2:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1]));
            external_free(fn);
            break;
        case 3:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2]));
            external_free(fn);
            break;
        case 4:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3]));
            external_free(fn);
            break;
        case 5:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4]));
            external_free(fn);
            break;
        case 6:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4], argt[5]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]));
            external_free(fn);
            break;
        case 7:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4], argt[5], argt[6]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]));
            external_free(fn);
            break;
        case 8:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4], argt[5], argt[6], argt[7]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]));
            external_free(fn);
            break;
        case 9:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4], argt[5], argt[6], argt[7], argt[8]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]));
            external_free(fn);
            break;
        case 10:
            var fn = external_define(libpath, name, calltype, type, argc, argt[0], argt[1], argt[2], argt[3], argt[4], argt[5], argt[6], argt[7], argt[8], argt[9]);
            if (fn < 0) break;
            show_debug_message(external_call(fn, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9]));
            external_free(fn);
            break;
        default:
            show_debug_message("Too many arguments supplied.");
        }
    }
}