/// this tests for a bug in which returning a value within a `repeat` block inside a `with` block causes a stack error.

var fn = function()
{
    var str = { };
    
    with (str)
    {
        repeat (10)
        {
            return 5;
        }
    }
}

show_debug_message(fn());