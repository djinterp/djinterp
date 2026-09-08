/******************************************************************************
* djinterp [core]                                                   djinterp.c
*
*   Definitions for the non-inline declarations in djinterp.h.
*   Currently that is the d_index family: the conversion from djinterp's
* signed, Python-style index to a plain container offset.
*
*
* path:      /src/djinterp/c/djinterp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.11.12
*                                                          revised: 2026.09.07
******************************************************************************/
#include "../../../inc/djinterp/c/djinterp.h"


///////////////////////////////////////////////////////////////////////////////
///             I.  MAGNITUDE HELPER                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_index_magnitude
  The absolute value of a NEGATIVE index, as a size_t.
  Spelled the long way on purpose. The obvious `(size_t)(-_index)` is undefined
behaviour at exactly one input -- the most negative d_index -- because its
negation is not representable in the signed type. That input is unlikely and
the failure is silent, which is the combination worth spending three tokens to
avoid. `-(_index + 1)` is always representable (the addition moves the value
one step away from the edge first), and adding the 1 back in unsigned domain
cannot overflow.

Parameter(s):
  _index: an index known to be negative.
Return:
  |_index|, exactly, for every negative d_index including the minimum.
*/
static size_t
d_internal_index_magnitude
(
    d_index _index
)
{
    return ((size_t)(-(_index + 1))) + (size_t)1;
}


///////////////////////////////////////////////////////////////////////////////
///             II.  VALIDITY                                               ///
///////////////////////////////////////////////////////////////////////////////

/*
d_index_is_valid
  Whether _index addresses a real element of a container holding _count of
them.
  The two halves are not symmetric, and that asymmetry is the contract rather
than an oversight: a positive index must be strictly less than _count (index 0
is the first element), while a negative one may have magnitude equal to
_count (-_count is the first element, -1 the last). So _count == 3 accepts
0, 1, 2 and -1, -2, -3, and rejects 3 and -4.
  Comparisons are made after the sign is decided. Comparing a d_index against
a size_t directly promotes the signed operand to unsigned, at which point every
negative index is a very large positive one and every out-of-range check
silently passes.

Parameter(s):
  _index: the index to test.
  _count: the number of elements available.
Return:
  1 when _index addresses an element, 0 otherwise. A _count of 0 has no valid
index of either sign.
*/
bool
d_index_is_valid
(
    d_index _index,
    size_t  _count
)
{
    if (_index >= 0)
    {
        return ((size_t)_index < _count);
    }

    return (d_internal_index_magnitude(_index) <= _count);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVERSION                                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_index_convert_fast
  Converts a signed index to an offset WITHOUT validating it.
  The unchecked half of the pair, for callers that have already established the
range (a loop bound, a prior d_index_is_valid) and do not want to pay for it
twice.
  The negative case is unsigned modular arithmetic, not an accident: converting
a negative d_index to size_t yields a huge value, and adding _count to it wraps
back to exactly _count - |_index|, which is the answer. It is well defined for
unsigned types -- but ONLY produces the intended result while |_index| <=
_count. Outside that range it wraps to a plausible-looking offset rather than
failing, which is precisely why the safe variant exists and why this one says
"fast" in its name.

Parameter(s):
  _index: the index to convert. Assumed valid for _count.
  _count: the number of elements available.
Return:
  The zero-based offset _index refers to. Meaningless if _index was not valid.
*/
size_t
d_index_convert_fast
(
    d_index _index,
    size_t  _count
)
{
    if (_index >= 0)
    {
        return (size_t)_index;
    }

    return _count + (size_t)_index;
}

/*
d_index_convert_safe
  Converts a signed index to an offset, refusing anything out of range.
  The offset is written through _destination rather than returned so that
failure has a value of its own: every size_t is a legal offset, so there is no
in-band sentinel this function could return that a caller could not also have
meant. Splitting the two apart is what lets the failure be checked.
  _destination is left untouched on failure. A caller that ignores the return
value therefore sees whatever it had before rather than a fabricated 0, which
keeps a missed check from looking like a successful conversion to the first
element.

Parameter(s):
  _index:       the index to convert.
  _count:       the number of elements available.
  _destination: receives the offset on success; untouched on failure.
Return:
  1 on success, 0 if _destination is NULL or _index does not address an
element of a _count-element container.
*/
bool
d_index_convert_safe
(
    d_index _index,
    size_t  _count,
    size_t* _destination
)
{
    if (!_destination)
    {
        return false;
    }

    if (!d_index_is_valid(_index, _count))
    {
        return false;
    }

    *_destination = d_index_convert_fast(_index, _count);

    return true;
}
