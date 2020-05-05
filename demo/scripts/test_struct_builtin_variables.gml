var obj = {
    x: 4, b: 5
};
obj.y = 3;

ogm_assert(obj.x == 4);
ogm_assert(obj.y == 3);
ogm_assert(obj.b == 5);
with (obj)
{
    ogm_assert(variable_instance_get(self, "b") == 5);
    ogm_assert(variable_instance_get(self, "y") == 3);
    ogm_assert(variable_instance_get(self, "x") == 4);
}