var arr, arr2;
arr[1] = 1;
arr2[1][1][1] = 1;

ogm_assert(arr[1] == 1);
ogm_assert(arr2[1][1][1] == 1);
ogm_assert(arr2[1][1][0] == 0);
ogm_assert(arr2[1][0] == 0);
ogm_assert(arr2[0] == 0);
ogm_assert(array_length(arr2[1]) == 2);
ogm_assert(array_length(arr2[1][1]) == 2);