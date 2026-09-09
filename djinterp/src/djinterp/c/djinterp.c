/******************************************************************************
* djinterp [c]                                                      djinterp.c
*
* Definitions for the non-inline declarations in `djinterp.h`.
*
*
* path:      /src/djinterp/c/djinterp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.11.12
*                                                          revised: 2026.09.07
******************************************************************************/
#include "../../../inc/djinterp/c/djinterp.h"


/*
d_internal_index_magnitude
  Returns the magnitude of a negative index as size_t.

  The obvious expression (size_t)(-_index) is undefined for the most negative
d_index because its positive value is not representable in the signed type.
The expression below first moves the value one step toward zero, negates that
representable value, and restores the final unit in the unsigned domain.

Parameter(s):
  _index: an index known to be negative.
Return:
  The exact magnitude of _index, including for the minimum d_index.
*/
D_STATIC size_t
d_internal_index_magnitude(
    d_index _index
)
{

    return ((size_t)(-(_index + 1))) + (size_t)1;
}


/*
d_index_is_valid
  Determines whether _index addresses an element of a container containing
_count elements.

  Positive and negative indices have intentionally asymmetric limits. A
non-negative index must be less than _count, while a negative index may have a
magnitude equal to _count. For _count == 3, the valid indices are 0, 1, 2,
-1, -2, and -3.

  Signed and unsigned comparisons are performed only after the sign is known
so that a negative d_index is not converted to a large size_t before the range
test.

Parameter(s):
  _index: the index to test.
  _count: the number of elements available.
Return:
  true when _index addresses an element; false otherwise. A count of 0 has no
valid index of either sign.
*/
bool
d_index_is_valid(
    d_index _index,
    size_t  _count
)
{
    // non-negative indices map directly to zero-based offsets
    if (_index >= 0)
    {

        return ((size_t)_index < _count);
    }

    // negative indices count backward from the end of the container

    return (d_internal_index_magnitude(_index) <= _count);
}


/*
d_index_convert_fast
  Converts a signed index to a zero-based offset without validating it.

  This is the unchecked conversion for callers that have already established
that _index is valid for _count. In the negative case, conversion to size_t and
addition to _count use well-defined unsigned modular arithmetic to produce
_count - |_index| for valid inputs.

Parameter(s):
  _index: the index to convert; assumed valid for _count.
  _count: the number of elements available.
Return:
  The zero-based offset corresponding to _index. The result is not meaningful
if _index is invalid for _count.
*/
size_t
d_index_convert_fast(
    d_index _index,
    size_t  _count
)
{
    // non-negative indices are already zero-based offsets
    if (_index >= 0)
    {

        return (size_t)_index;
    }

    // valid negative indices wrap to the corresponding positive offset

    return _count + (size_t)_index;
}


/*
d_index_convert_safe
  Converts a signed index to a zero-based offset after validating it.

  The offset is written through _destination so that conversion failure has a
separate value. The destination is left untouched on failure.

Parameter(s):
  _index:       the index to convert.
  _count:       the number of elements available.
  _destination: receives the offset on success; untouched on failure.
Return:
  true on success; false if _destination is NULL or _index is invalid.
*/
bool
d_index_convert_safe(
    d_index _index,
    size_t  _count,
    size_t* _destination
)
{
    // validate the destination pointer
    if (!_destination)
    {

        return false;
    }

    // reject indices that do not address an available element
    if (!d_index_is_valid(_index, _count))
    {

        return false;
    }

    *(_destination) = d_index_convert_fast(_index, _count);

    return true;
}