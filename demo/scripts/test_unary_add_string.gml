// this test explains why we can't assert
// that the type is not string or array in Variable::set
var z;
z[1] = "";
++z[0]