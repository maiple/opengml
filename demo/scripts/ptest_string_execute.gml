global.A = 0;

string_execute("
    var z = 3;
    ++z;
    z++;
    global.A = z;
    {
        // nested string execution
        string_execute('
            global.A++;
        ');
        
        // test cache
        string_execute('
            global.A++;
        ');
    }
")

// test cache again
{
    {
        string_execute('
            global.A++;
        ');
    }
}

ogm_assert(global.A == 8);