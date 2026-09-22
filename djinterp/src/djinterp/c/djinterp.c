/*******************************************************************************
* djinterp [c]                                                        djinterp.c
*
* Definitions for the non-inline declarations in `djinterp.h`.
*   Implements the negative-index validation and conversion functions; the rest
* of the header consists of macros and typedefs, which need no definitions.
*
* path:      /src/djinterp/c/djinterp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.09.22
*******************************************************************************/
#include "../../../inc/djinterp/c/djinterp.h"  // corresponding header
// std
#include <assert.h>   // static_assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL


// the index arithmetic below depends on both of these properties of d_index
static_assert((d_index)-1 < 0,
              "d_index must be a signed type: every index function below "
              "branches on its sign");
static_assert(sizeof(d_index) <= sizeof(size_t),
              "d_index must be no wider than size_t: index magnitudes are "
              "converted to size_t without a range check");

/*
d_internal_index_magnitude
  Returns the exact magnitude of an index that every caller has already found
to be negative, including the most negative d_index.
  The obvious expression (size_t)(-_index) is undefined for the most negative
d_index because its positive value is not representable in the signed type.
The expression below first moves the value one step toward zero, negates that
representable value, and restores the final unit in the unsigned domain, where
the width assertion above guarantees that it fits.
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
  Signed and unsigned comparisons are performed only after the sign is known,
so that a negative d_index is never converted to a large size_t before the range
test. The two branches differ only in `<` against `<=`, which is where the
asymmetric limits documented on the declaration come from.
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
  In the negative case, the conversion to size_t and the addition to _count
both use well-defined unsigned modular arithmetic, so the sum wraps to _count
minus the magnitude of _index with no signed intermediate that could overflow,
even for the most negative d_index.
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
  Composes the two public functions rather than repeating their logic, so the
checked and unchecked conversions cannot drift apart. The destination is written
only after every check has passed, which is what leaves it untouched on failure.
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
    if (!d_index_is_valid(_index,
                          _count))
    {
        return false;
    }

    *(_destination) = d_index_convert_fast(_index,
                                           _count);

    return true;
}
