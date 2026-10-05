/*******************************************************************************
* djinterp [c]                                                       semigroup.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/semigroup.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/semigroup.h"


/*
d_internal_semigroup_mappend_first
  Keeps the left operand. Associative, and has no identity: no value `e`
satisfies `first(e, x) == x` for two distinct values of `x`, since
`first(e, x)` is `e` for every `x`.

Parameter(s):
  _semigroup: the receiver, supplying the width.
  _result:    destination; may alias `_left`.
  _left:      the operand kept.
  _right:     the operand discarded.
Return:
  none.
*/
static void
d_internal_semigroup_mappend_first
(
    const struct d_semigroup* _semigroup,
    void*                     _result,
    const void*               _left,
    const void*               _right
)
{
    (void)_right;

    // a NULL receiver or operand leaves the destination alone
    if ( (!_semigroup) ||
         (!_result)    ||
         (!_left)      )
    {
        return;
    }

    // aliasing the destination onto the kept operand makes this a no-op
    if (_result != _left)
    {
        memcpy(_result, _left, _semigroup->value_size);
    }

    return;
}

/*
d_internal_semigroup_mappend_last
  Keeps the right operand. Associative, and likewise has no identity.

Parameter(s):
  _semigroup: the receiver, supplying the width.
  _result:    destination; may alias `_left`.
  _left:      the operand discarded.
  _right:     the operand kept.
Return:
  none.
*/
static void
d_internal_semigroup_mappend_last
(
    const struct d_semigroup* _semigroup,
    void*                     _result,
    const void*               _left,
    const void*               _right
)
{
    (void)_left;

    // a NULL receiver or operand leaves the destination alone
    if ( (!_semigroup) ||
         (!_result)    ||
         (!_right)     )
    {
        return;
    }

    memcpy(_result, _right, _semigroup->value_size);

    return;
}

/*
d_internal_semigroup_step
  The reducer step of a semigroup: combines the accumulator with the incoming
element in place. This is the whole bridge between the algebra and the dataflow
layer, and it is why `mappend` permits `_result` to alias `_left`.

Parameter(s):
  _state:   the reducing state whose accumulator is combined into.
  _element: the incoming element.
  _context: the borrowed `d_semigroup` supplying the combine.
Return:
  none.
*/
static void
d_internal_semigroup_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    const struct d_semigroup* semigroup;

    // a NULL state, semigroup, or element is a caller error
    if ( (!_state)   ||
         (!_context) ||
         (!_element) )
    {
        return;
    }

    semigroup = (const struct d_semigroup*)_context;

    // an unset combine, or no accumulator, leaves the fold as it was
    if ( (!semigroup->mappend)  ||
         (!_state->accumulator) )
    {
        return;
    }

    semigroup->mappend(semigroup,
                       _state->accumulator,
                       _state->accumulator,
                       _element);

    return;
}


/*
d_semigroup_make
  Builds a semigroup from a combine, its configuration, and its width.

Parameter(s):
  _mappend:    the associative combine; may be NULL, yielding an invalid
               semigroup.
  _context:    instance configuration; may be NULL.
  _value_size: byte size of the values combined; must be non-zero to be valid.
Return:
  A semigroup by value.
*/
struct d_semigroup
d_semigroup_make
(
    fn_mappend _mappend,
    void*      _context,
    size_t     _value_size
)
{
    struct d_semigroup semigroup;

    semigroup.mappend    = _mappend;
    semigroup.context    = _context;
    semigroup.value_size = _value_size;

    return semigroup;
}

/*
d_semigroup_is_valid
  Reports whether a semigroup carries a callable combine over a non-zero width.

Parameter(s):
  _semigroup: the semigroup to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the combine is set and the width is non-zero, or
  - false, otherwise.
*/
bool
d_semigroup_is_valid
(
    const struct d_semigroup* _semigroup
)
{
    // a NULL semigroup is not valid
    if (!_semigroup)
    {
        return false;
    }

    return ( (_semigroup->mappend != NULL) &&
             (_semigroup->value_size != 0) );
}

/*
d_mappend
  Combines two values through a semigroup. This is Haskell's `<>`, and the free
function the C++ face exposes under the same name.

Parameter(s):
  _semigroup: the semigroup supplying the combine.
  _result:    destination for the combination; may alias `_left`.
  _left:      the left operand.
  _right:     the right operand.
Return:
  A boolean value corresponding to either:
  - true, if the combination was written, or
  - false, if the parameters were unusable.
*/
bool
d_mappend
(
    const struct d_semigroup* _semigroup,
    void*                     _result,
    const void*               _left,
    const void*               _right
)
{
    // an unusable request combines nothing
    if ( (!d_semigroup_is_valid(_semigroup)) ||
         (!_result)                          ||
         (!_left)                            ||
         (!_right)                           )
    {
        return false;
    }

    _semigroup->mappend(_semigroup, _result, _left, _right);

    return true;
}

/*
d_semigroup_reducer
  Lifts a semigroup into a reducer whose accumulator is combined in place. The
semigroup is borrowed, not copied, and must outlive the reducer.
  Because the result is an ordinary `d_reducer`, a semigroup fold is drivable by
every driver in the subframework: over an array, over a producer, or as the sink
of a transducer chain.

Parameter(s):
  _semigroup: the borrowed semigroup; may be NULL.
Return:
  A reducer by value; an invalid reducer if the semigroup is unusable.
*/
struct d_reducer
d_semigroup_reducer
(
    const struct d_semigroup* _semigroup
)
{
    // an unusable semigroup yields an invalid reducer
    if (!d_semigroup_is_valid(_semigroup))
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_semigroup_step, (void*)_semigroup);
}

/*
d_semigroup_reduce
  Combines a non-empty array left to right, seeding the accumulator from the
first element. A semigroup has no identity, so an empty array has no result and
is reported as a failure rather than given one. This is the operational
difference from `d_mconcat`, which answers `mempty` on an empty array.

Parameter(s):
  _semigroup:   the semigroup supplying the combine.
  _elements:    the base of the array.
  _count:       the number of elements; must be at least 1.
  _accumulator: destination for the combination, of `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the array was combined into `_accumulator`, or
  - false, if the array was empty or the parameters were unusable.
*/
bool
d_semigroup_reduce
(
    const struct d_semigroup* _semigroup,
    const void*               _elements,
    size_t                    _count,
    void*                     _accumulator
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;
    const unsigned char*    base;

    // an empty array has no result, because there is no identity to return
    if ( (!d_semigroup_is_valid(_semigroup)) ||
         (!_elements)                        ||
         (!_accumulator)                     ||
         (_count == 0)                       )
    {
        return false;
    }

    base = (const unsigned char*)_elements;

    memcpy(_accumulator, base, _semigroup->value_size);

    // a single element is already its own combination
    if (_count == 1)
    {
        return true;
    }

    reducer = d_semigroup_reducer(_semigroup);

    d_reducing_state_init(&state, _accumulator);
    d_reducer_drive_array(&reducer,
                          &state,
                          (const void*)(base + _semigroup->value_size),
                          _count - 1,
                          _semigroup->value_size);

    return true;
}

/*
d_semigroup_first
  The semigroup that keeps the left operand. Size-generic, and deliberately
without an identity: it is a semigroup and not a monoid, which is the
distinction the two protocols exist to separate.

Parameter(s):
  _value_size: byte size of the values combined; must be non-zero.
Return:
  A semigroup by value; an invalid semigroup if `_value_size` is 0.
*/
struct d_semigroup
d_semigroup_first
(
    size_t _value_size
)
{
    // a zero width has nothing to keep
    if (_value_size == 0)
    {
        return d_semigroup_make(NULL, NULL, 0);
    }

    return d_semigroup_make(&d_internal_semigroup_mappend_first,
                            NULL,
                            _value_size);
}

/*
d_semigroup_last
  The semigroup that keeps the right operand. Size-generic, and likewise without
an identity.

Parameter(s):
  _value_size: byte size of the values combined; must be non-zero.
Return:
  A semigroup by value; an invalid semigroup if `_value_size` is 0.
*/
struct d_semigroup
d_semigroup_last
(
    size_t _value_size
)
{
    // a zero width has nothing to keep
    if (_value_size == 0)
    {
        return d_semigroup_make(NULL, NULL, 0);
    }

    return d_semigroup_make(&d_internal_semigroup_mappend_last,
                            NULL,
                            _value_size);
}
