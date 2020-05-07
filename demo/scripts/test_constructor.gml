globalvar length = function(x, y, z)
{
    return sqrt(sqr(x) + sqr(y) + sqr(z));
}

Vector = function(x, y, z) constructor
{
    show_debug_message(x);
    show_debug_message(y);
    show_debug_message(z);
    self.x = x;
    self.y = y;
    self.z = z;
    self._length = length(x, y, z);
}

var vec = new Vector(2, 6, 3);

ogm_assert(vec.x == 2);
ogm_assert(vec.y == 6);
ogm_assert(vec.z == 3);
ogm_assert(vec._length == 7);