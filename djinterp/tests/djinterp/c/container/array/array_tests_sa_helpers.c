/*******************************************************************************
* djinterp [container]                                  array_tests_sa_helpers.c
*
* Helper functions for `array.h` unit tests.
* Part of the array standalone test suite.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa_helpers.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"


/*
d_test_int_comparator
  Comparator function for integers used in sorting/search tests.

Parameter(s):
  _a: pointer to first integer
  _b: pointer to second integer
Return:
  negative if a < b, zero if a == b, positive if a > b
*/
int
d_test_int_comparator
(
    const void* _a,
    const void* _b
)
{
    int ia = *(const int*)_a;
    int ib = *(const int*)_b;

    if (ia < ib)
    {
        return -1;
    }

    if (ia > ib)
    {
        return 1;
    }

    return 0;
}

/*
d_test_arrays_equal_int
  Helper function to compare two integer arrays for equality.

Parameter(s):
  _arr1:  pointer to first array
  _arr2:  pointer to second array
  _count: number of elements to compare
Return:
  true if arrays are equal, false otherwise
*/
bool
d_test_arrays_equal_int
(
    const void* _arr1,
    const void* _arr2,
    size_t      _count
)
{
    const int* a1 = (const int*)_arr1;
    const int* a2 = (const int*)_arr2;
    size_t     i;

    if (!a1 || !a2)
    {
        return (a1 == a2);
    }

    for (i = 0; i < _count; i++)
    {
        if (a1[i] != a2[i])
        {
            return false;
        }
    }

    return true;
}

/*
d_test_create_test_data_int
  Creates test data array of integers [1, 2, 3, ..., _count].

Parameter(s):
  _data:  pointer to receive allocated data
  _count: number of elements to create
Return:
  none.
*/
void
d_test_create_test_data_int
(
    int**  _data,
    size_t _count
)
{
    size_t i;

    *_data = malloc(_count * sizeof(int));

    if (*_data)
    {
        for (i = 0; i < _count; i++)
        {
            (*_data)[i] = (int)(i + 1);
        }
    }

    return;
}
