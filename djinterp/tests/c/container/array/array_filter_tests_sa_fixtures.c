#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * TEST FIXTURE HELPERS
 *****************************************************************************/

/*
d_tests_array_filter_fill_sequential
  Fills an int array with sequential values [0, 1, 2, ..., count-1].

Parameter(s):
  _arr:   destination array; must have room for _count ints.
  _count: number of elements to write.
Return:
  none.
*/
void
d_tests_array_filter_fill_sequential
(
    int*   _arr,
    size_t _count
)
{
    size_t i;

    if (!_arr)
    {
        return;
    }

    for (i = 0; i < _count; ++i)
    {
        _arr[i] = (int)i;
    }

    return;
}


/*
d_tests_array_filter_fill_with_duplicates
  Fills an int array with a fixed pattern containing duplicates:
  {3, 1, 4, 1, 5, 9, 2, 6, 5, 3}.  If _count < 10, only the first
  _count values are written.  If _count > 10, remaining slots are 0.

Parameter(s):
  _arr:   destination array; must have room for _count ints.
  _count: number of elements to write.
Return:
  none.
*/
void
d_tests_array_filter_fill_with_duplicates
(
    int*   _arr,
    size_t _count
)
{
    static const int pattern[] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
    size_t           pattern_len;
    size_t           i;

    if (!_arr)
    {
        return;
    }

    pattern_len = sizeof(pattern) / sizeof(pattern[0]);

    for (i = 0; i < _count; ++i)
    {
        _arr[i] = (i < pattern_len) ? pattern[i] : 0;
    }

    return;
}


/*
d_tests_array_filter_is_even
  Predicate: returns true if the int element is even.

Parameter(s):
  _element: pointer to an int.
  _context: unused (may be NULL).
Return:
  true if *_element is divisible by 2, false otherwise.
*/
bool
d_tests_array_filter_is_even
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if (!_element)
    {
        return false;
    }

    return (*(const int*)_element % 2) == 0;
}


/*
d_tests_array_filter_is_positive
  Predicate: returns true if the int element is strictly positive.

Parameter(s):
  _element: pointer to an int.
  _context: unused (may be NULL).
Return:
  true if *_element > 0, false otherwise.
*/
bool
d_tests_array_filter_is_positive
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if (!_element)
    {
        return false;
    }

    return *(const int*)_element > 0;
}


/*
d_tests_array_filter_is_greater_than
  Predicate: returns true if the int element is greater than the
  threshold stored in _context.

Parameter(s):
  _element: pointer to an int.
  _context: pointer to an int threshold.
Return:
  true if *_element > *(int*)_context, false otherwise.
*/
bool
d_tests_array_filter_is_greater_than
(
    const void* _element,
    void*       _context
)
{
    if ( (!_element) ||
         (!_context) )
    {
        return false;
    }

    return *(const int*)_element > *(const int*)_context;
}


/*
d_tests_array_filter_always_true
  Predicate: returns true for any element.

Parameter(s):
  _element: unused.
  _context: unused.
Return:
  true.
*/
bool
d_tests_array_filter_always_true
(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}


/*
d_tests_array_filter_always_false
  Predicate: returns false for any element.

Parameter(s):
  _element: unused.
  _context: unused.
Return:
  false.
*/
bool
d_tests_array_filter_always_false
(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}


/*
d_tests_array_filter_compare_int
  Comparator: three-way comparison for int elements.

Parameter(s):
  _a:       pointer to first int.
  _b:       pointer to second int.
  _context: unused (may be NULL).
Return:
  negative if *_a < *_b, 0 if equal, positive if *_a > *_b.
*/
int
d_tests_array_filter_compare_int
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    int va;
    int vb;

    (void)_context;

    if ( (!_a) ||
         (!_b) )
    {
        return 0;
    }

    va = *(const int*)_a;
    vb = *(const int*)_b;

    if (va < vb)
    {
        return -1;
    }

    if (va > vb)
    {
        return 1;
    }

    return 0;
}
