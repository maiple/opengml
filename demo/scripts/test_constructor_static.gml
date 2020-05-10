A = function(x) constructor {
    // typical static
    static X = x;
    
    // instance set to static value
    // (not that interesting)
    myX = X;
    ogm_assert(myX == X);
    
    // rename lookup
    static y = x;
    
    // instance variable
    z = x
}

var a = new A(3);
var b = new A(5);

// static lookup
ogm_assert(a.X == 3)
ogm_assert(b.X == 3)

// builtin variable static lookup
ogm_assert(a.y == 3)
ogm_assert(b.y == 3)

// typical instance variable/
// (to confirm that a and b aren't actually just the same somehow)
ogm_assert(a.z == 3)
ogm_assert(b.z == 5)