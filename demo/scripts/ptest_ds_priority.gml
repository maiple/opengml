v = ds_priority_create()

o = ds_priority_add(v, "hi", 5)
o = ds_priority_add(v, "hi_second", 5)
o = ds_priority_add(v, "hib", 2)
o = ds_priority_add(v, "hic", 8)

ogm_assert(ds_priority_delete_min(v) == "hib");
ogm_assert(ds_priority_delete_max(v) == "hic");
ogm_assert(ds_priority_delete_max(v) == "hi");
ogm_assert(ds_priority_delete_min(v) == "hi_second");
ogm_assert(is_undefined(ds_priority_delete_min(v)));