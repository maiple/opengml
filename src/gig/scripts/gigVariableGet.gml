/// gigVariableSet(variable name, global, self, [array index 1, array index 2])
// gets variable of the given name.

var vname = argument[0];
var _global = argument[1];
var _self = argument[2];
var arrayAccess = argument_count > 3;
var i, j;
if (arrayAccess)
{
    i = argument[3];
    j = argument[4];
}

// remove `global.` from start of name.
if (string_length(vname) > 7)
{
    if (string_copy(vname, 1, 7) == "global.")
    {
        _global = true;
        vname = string_copy(vname, 8, string_length(vname) - 8);
    }
}

if (!arrayAccess)
{
    // check built-in variables first
    if (vname == "background_colour")
        return background_colour;
    else if (vname == "view_xview")
        return view_xview;
    else if (vname == "view_yview")
        return view_yview;
    // TODO: add more built-in variables
    else
    {
        if (_global)
        {
            return variable_global_get(vname);
        }
        else
        {
            return variable_instance_get(_self, vname);
        }
    }
}
else
{
    // array access
    if (vname == "view_xview")
        return view_xview[i, j];
    else if (vname == "view_yview")
        return view_yview[i, j];
    // TODO: add more built-in variables
    else
    {
        var arr;
        if (_global)
        {
            arr = variable_global_get(vname);
        }
        else
        {
            arr = variable_instance_get(_self, vname);
        }
        
        return arr[@ i, j];
    }
}
