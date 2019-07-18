/// gigVariableSet(variable name, value, global, self, [array index 1, array index 2])
// sets variable of the given name.

var vname = argument[0];
var val = argument[1];
var _global = argument[2];
var _self = argument[3];
var arrayAccess = argument_count > 4;
var i, j;
if (arrayAccess)
{
    i = argument[4];
    j = argument[5];
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
        background_colour = val;
    else if (vname == "view_xview")
        view_xview = val;
    else if (vname == "view_yview")
        view_yview = val;
    // TODO: add more built-in variables
    else
    {
        if (_global)
        {
            variable_global_set(vname, val);
        }
        else
        {
            variable_instance_set(_self, vname, val);
        }
    }
}
else
{
    // array access
    if (vname == "view_xview")
        view_xview[i, j] = val;
    else if (vname == "view_yview")
        view_yview[i, j] = val;
    // TODO: add more built-in variables
    else
    {
        var v;
        v[0] = 0;
        var arr;
        if (_global)
        {
            if (!variable_global_exists(vname))
            {
                variable_global_set(vname, v);
            }
            arr = variable_global_get(vname);
        }
        else
        {
            if (!variable_instance_exists(_self, vname))
            {
                variable_instance_set(_self, vname, v);
            }
            arr = variable_instance_get(_self, vname);
        }
        
        arr[@ i, j] = val;
    }
}
