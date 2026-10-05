/*******************************************************************************
* djinterp [c]                                                          monoid.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/monoid.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/monoid.h"


// D_INTERNAL_MONOID_INFIX
//   macro: defines the combine and the identity of a monoid whose operation is
// a binary infix operator. Generates `d_internal_monoid_mappend_<suffix>` and
// `d_internal_monoid_mempty_<suffix>`. The combine reads both operands before
// writing, so `_result` may alias `_left`.
#define D_INTERNAL_MONOID_INFIX(suffix,                                     \
                                type,                                       \
                                op,                                         \
                                identity)                                   \
    static void                                                             \
    d_internal_monoid_mappend_##suffix                                      \
    (                                                                       \
        const struct d_semigroup* _semigroup,                               \
        void*                     _result,                                  \
        const void*               _left,                                    \
        const void*               _right                                    \
    )                                                                       \
    {                                                                       \
        (void)_semigroup;                                                   \
                                                                            \
        if ( (!_result) ||                                                  \
             (!_left)   ||                                                  \
             (!_right)  )                                                   \
        {                                                                   \
            return;                                                         \
        }                                                                   \
                                                                            \
        *(type*)_result = (type)( (*(const type*)_left) op                  \
                                 (*(const type*)_right) );                 \
                                                                            \
        return;                                                             \
    }                                                                       \
                                                                            \
    static void                                                             \
    d_internal_monoid_mempty_##suffix                                       \
    (                                                                       \
        const struct d_monoid* _monoid,                                     \
        void*                  _identity                                    \
    )                                                                       \
    {                                                                       \
        (void)_monoid;                                                      \
                                                                            \
        if (!_identity)                                                     \
        {                                                                   \
            return;                                                         \
        }                                                                   \
                                                                            \
        *(type*)_identity = (identity);                                     \
                                                                            \
        return;                                                             \
    }

// D_INTERNAL_MONOID_SELECT
//   macro: defines the combine and the identity of a monoid that keeps
// whichever operand satisfies a comparison, which is how min and max are
// built. The identity is the absorbing extreme, so it is never selected.
#define D_INTERNAL_MONOID_SELECT(suffix,                                    \
                                 type,                                      \
                                 cmp,                                       \
                                 identity)                                  \
    static void                                                             \
    d_internal_monoid_mappend_##suffix                                      \
    (                                                                       \
        const struct d_semigroup* _semigroup,                               \
        void*                     _result,                                  \
        const void*               _left,                                    \
        const void*               _right                                    \
    )                                                                       \
    {                                                                       \
        type left;                                                          \
        type right;                                                         \
                                                                            \
        (void)_semigroup;                                                   \
                                                                            \
        if ( (!_result) ||                                                  \
             (!_left)   ||                                                  \
             (!_right)  )                                                   \
        {                                                                   \
            return;                                                         \
        }                                                                   \
                                                                            \
        left  = *(const type*)_left;                                        \
        right = *(const type*)_right;                                       \
                                                                            \
        *(type*)_result = (left cmp right) ? left : right;                  \
                                                                            \
        return;                                                             \
    }                                                                       \
                                                                            \
    static void                                                             \
    d_internal_monoid_mempty_##suffix                                       \
    (                                                                       \
        const struct d_monoid* _monoid,                                     \
        void*                  _identity                                    \
    )                                                                       \
    {                                                                       \
        (void)_monoid;                                                      \
                                                                            \
        if (!_identity)                                                     \
        {                                                                   \
            return;                                                         \
        }                                                                   \
                                                                            \
        *(type*)_identity = (identity);                                     \
                                                                            \
        return;                                                             \
    }

D_INTERNAL_MONOID_INFIX(sum_intmax,      intmax_t, +,  (intmax_t)0)
D_INTERNAL_MONOID_INFIX(product_intmax,  intmax_t, *,  (intmax_t)1)
D_INTERNAL_MONOID_SELECT(min_intmax,     intmax_t, <,  INTMAX_MAX)
D_INTERNAL_MONOID_SELECT(max_intmax,     intmax_t, >,  INTMAX_MIN)

D_INTERNAL_MONOID_INFIX(sum_double,      double,   +,  0.0)
D_INTERNAL_MONOID_INFIX(product_double,  double,   *,  1.0)
D_INTERNAL_MONOID_SELECT(min_double,     double,   <,  DBL_MAX)
D_INTERNAL_MONOID_SELECT(max_double,     double,   >,  (-DBL_MAX))

D_INTERNAL_MONOID_INFIX(all,             bool,     &&, true)
D_INTERNAL_MONOID_INFIX(any,             bool,     ||, false)


// d_internal_fold_monoid_binding
//   struct: state threaded through a `d_fold_monoid` drive.
// Holds the monoid being folded into, the mapping from element to monoid value,
// and the scratch that mapped value lands in.
struct d_internal_fold_monoid_binding
{
    const struct d_monoid* monoid;      // the monoid supplying the combine
    fn_transformer         to_monoid;   // element -> monoid value
    void*                  context;     // context for `to_monoid`
    void*                  scratch;     // buffer for the mapped value
};


/*
d_internal_monoid_fold_step
  Maps one element into a monoid value and combines it into the accumulator. A
mapping that reports failure skips the element rather than terminating the fold,
which keeps a partial mapping well-defined over its domain.

Parameter(s):
  _state:   the reducing state whose accumulator is combined into.
  _element: the incoming element, in the source type.
  _context: the `d_internal_fold_monoid_binding` for this drive.
Return:
  none.
*/
static void
d_internal_monoid_fold_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_internal_fold_monoid_binding* binding;
    const struct d_semigroup*              semigroup;

    // a NULL state, binding, or element is a caller error
    if ( (!_state)   ||
         (!_context) ||
         (!_element) )
    {
        return;
    }

    binding = (struct d_internal_fold_monoid_binding*)_context;

    // without a mapping, scratch, or an accumulator there is nothing to fold
    if ( (!binding->to_monoid)    ||
         (!binding->scratch)      ||
         (!binding->monoid)       ||
         (!_state->accumulator)   )
    {
        return;
    }

    // a failed mapping drops the element
    if (!binding->to_monoid(_element, binding->scratch, binding->context))
    {
        return;
    }

    semigroup = d_monoid_semigroup(binding->monoid);

    semigroup->mappend(semigroup,
                       _state->accumulator,
                       _state->accumulator,
                       binding->scratch);

    return;
}


/*
d_monoid_make
  Builds a monoid by decorating a semigroup with an identity.

Parameter(s):
  _semigroup: the underlying associative combine.
  _mempty:    writes the identity element; may be NULL, yielding an invalid
              monoid.
Return:
  A monoid by value.
*/
struct d_monoid
d_monoid_make
(
    struct d_semigroup _semigroup,
    fn_mempty          _mempty
)
{
    struct d_monoid monoid;

    monoid.semigroup = _semigroup;
    monoid.mempty    = _mempty;

    return monoid;
}

/*
d_monoid_is_valid
  Reports whether a monoid carries both a usable semigroup and an identity.

Parameter(s):
  _monoid: the monoid to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the contained semigroup is valid and the identity is set, or
  - false, otherwise.
*/
bool
d_monoid_is_valid
(
    const struct d_monoid* _monoid
)
{
    // a NULL monoid is not valid
    if (!_monoid)
    {
        return false;
    }

    return ( (d_semigroup_is_valid(&_monoid->semigroup)) &&
             (_monoid->mempty != NULL)                   );
}

/*
d_monoid_semigroup
  The forgetful map: reads a monoid as the semigroup it decorates. Free, since
the semigroup is the first member.

Parameter(s):
  _monoid: the monoid to forget; may be NULL.
Return:
  A pointer to the contained semigroup, or NULL if `_monoid` is NULL.
*/
const struct d_semigroup*
d_monoid_semigroup
(
    const struct d_monoid* _monoid
)
{
    // a NULL monoid has no semigroup to forget to
    if (!_monoid)
    {
        return NULL;
    }

    return &_monoid->semigroup;
}

/*
d_mempty
  Writes the identity element of a monoid. `T` is explicit in the C++ face; here
the width travels in the monoid and the destination is the caller's.

Parameter(s):
  _monoid:   the monoid supplying the identity.
  _identity: destination of at least `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the identity was written, or
  - false, if the parameters were unusable.
*/
bool
d_mempty
(
    const struct d_monoid* _monoid,
    void*                  _identity
)
{
    // an unusable request writes nothing
    if ( (!d_monoid_is_valid(_monoid)) ||
         (!_identity)                  )
    {
        return false;
    }

    _monoid->mempty(_monoid, _identity);

    return true;
}

/*
d_monoid_reducer
  Lifts a monoid into a reducer, by forgetting to its semigroup. The caller
seeds the accumulator with `d_mempty` before driving, which is what makes the
fold total over an empty source.

Parameter(s):
  _monoid: the borrowed monoid; must outlive the reducer.
Return:
  A reducer by value; an invalid reducer if the monoid is unusable.
*/
struct d_reducer
d_monoid_reducer
(
    const struct d_monoid* _monoid
)
{
    // an unusable monoid yields an invalid reducer
    if (!d_monoid_is_valid(_monoid))
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_semigroup_reducer(d_monoid_semigroup(_monoid));
}

/*
d_mconcat
  Combines every element of an array under a monoid. An empty array answers the
identity rather than failing, which is the whole operational difference from
`d_semigroup_reduce`.

Parameter(s):
  _monoid:   the monoid supplying the combine and the identity.
  _elements: the base of the array; may be NULL only if `_count` is 0.
  _count:    the number of elements to combine.
  _out:      destination of at least `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the combination (or the identity) was written, or
  - false, if the parameters were unusable.
*/
bool
d_mconcat
(
    const struct d_monoid* _monoid,
    const void*            _elements,
    size_t                 _count,
    void*                  _out
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;

    // an unusable request writes nothing
    if ( (!d_monoid_is_valid(_monoid))    ||
         (!_out)                          ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    // seeding with the identity is what makes the empty case total
    _monoid->mempty(_monoid, _out);

    // an empty array is already answered by the identity alone
    if (_count == 0)
    {
        return true;
    }

    reducer = d_monoid_reducer(_monoid);

    d_reducing_state_init(&state, _out);
    d_reducer_drive_array(&reducer,
                          &state,
                          _elements,
                          _count,
                          _monoid->semigroup.value_size);

    return true;
}

/*
d_mconcat_producer
  Combines every value of a producer under a monoid. The producer must be finite
or bounded; an unbounded one will not return.

Parameter(s):
  _monoid:   the monoid supplying the combine and the identity.
  _producer: the source to drain.
  _out:      destination of at least `value_size` bytes.
  _scratch:  caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the combination (or the identity) was written, or
  - false, if the parameters were unusable.
*/
bool
d_mconcat_producer
(
    const struct d_monoid* _monoid,
    struct d_producer*     _producer,
    void*                  _out,
    void*                  _scratch
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;

    // an unusable request writes nothing
    if ( (!d_monoid_is_valid(_monoid)) ||
         (!_out)                       ||
         (!_scratch)                   )
    {
        return false;
    }

    // an exhausted or absent producer still answers the identity
    _monoid->mempty(_monoid, _out);

    if (!d_producer_is_valid(_producer))
    {
        return true;
    }

    reducer = d_monoid_reducer(_monoid);

    d_reducing_state_init(&state, _out);
    d_producer_drive(_producer, &reducer, &state, _scratch);

    return true;
}

/*
d_fold_monoid
  Maps every element of an array into a monoid value and combines them. This is
the protocol-driven counterpart of an explicit-monoid `fold_map`: the identity
and the combine both come from `_monoid`, so only the mapping is supplied here.
  The same result is obtainable by composing a `d_transducer_map` stage onto a
monoid sink; this function is the direct spelling.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _monoid:       the monoid the elements are mapped into.
  _count:        the number of elements to map and combine.
  _element_size: the stride between source elements in bytes; must be non-zero.
  _to_monoid:    the mapping from source element to monoid value.
  _context:      context forwarded to `_to_monoid`; may be NULL.
  _out:          destination of at least `value_size` bytes.
  _scratch:      caller-owned buffer of at least `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the combination (or the identity) was written, or
  - false, if the parameters were unusable.
*/
bool
d_fold_monoid
(
    const struct d_monoid* _monoid,
    const void*            _elements,
    size_t                 _count,
    size_t                 _element_size,
    fn_transformer         _to_monoid,
    void*                  _context,
    void*                  _out,
    void*                  _scratch
)
{
    struct d_internal_fold_monoid_binding binding;
    struct d_reducer                      reducer;
    struct d_reducing_state               state;

    // an unusable request writes nothing
    if ( (!d_monoid_is_valid(_monoid))    ||
         (!_to_monoid)                    ||
         (!_out)                          ||
         (!_scratch)                      ||
         (_element_size == 0)             ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    // seeding with the identity is what makes the empty case total
    _monoid->mempty(_monoid, _out);

    // an empty array is already answered by the identity alone
    if (_count == 0)
    {
        return true;
    }

    binding.monoid    = _monoid;
    binding.to_monoid = _to_monoid;
    binding.context   = _context;
    binding.scratch   = _scratch;

    reducer = d_reducer_make(&d_internal_monoid_fold_step, &binding);

    d_reducing_state_init(&state, _out);
    d_reducer_drive_array(&reducer,
                          &state,
                          _elements,
                          _count,
                          _element_size);

    return true;
}

/*
d_monoid_sum_intmax
  Integer addition under 0.

Parameter(s):
  none.
Return:
  A monoid by value over `intmax_t`.
*/
struct d_monoid
d_monoid_sum_intmax
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_sum_intmax,
                                          NULL,
                                          sizeof(intmax_t)),
                         &d_internal_monoid_mempty_sum_intmax);
}

/*
d_monoid_product_intmax
  Integer multiplication under 1.

Parameter(s):
  none.
Return:
  A monoid by value over `intmax_t`.
*/
struct d_monoid
d_monoid_product_intmax
(
    void
)
{
    return d_monoid_make(
        d_semigroup_make(&d_internal_monoid_mappend_product_intmax,
                         NULL,
                         sizeof(intmax_t)),
        &d_internal_monoid_mempty_product_intmax);
}

/*
d_monoid_min_intmax
  Integer minimum under INTMAX_MAX.

Parameter(s):
  none.
Return:
  A monoid by value over `intmax_t`.
*/
struct d_monoid
d_monoid_min_intmax
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_min_intmax,
                                          NULL,
                                          sizeof(intmax_t)),
                         &d_internal_monoid_mempty_min_intmax);
}

/*
d_monoid_max_intmax
  Integer maximum under INTMAX_MIN.

Parameter(s):
  none.
Return:
  A monoid by value over `intmax_t`.
*/
struct d_monoid
d_monoid_max_intmax
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_max_intmax,
                                          NULL,
                                          sizeof(intmax_t)),
                         &d_internal_monoid_mempty_max_intmax);
}

/*
d_monoid_sum_double
  Floating-point addition under 0.0.
  Note: floating-point addition is not associative, so this instance satisfies
the monoid laws only up to rounding. That is a stated non-property, and it is
the one place the parity law needs a tolerance rather than equality.

Parameter(s):
  none.
Return:
  A monoid by value over `double`.
*/
struct d_monoid
d_monoid_sum_double
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_sum_double,
                                          NULL,
                                          sizeof(double)),
                         &d_internal_monoid_mempty_sum_double);
}

/*
d_monoid_product_double
  Floating-point multiplication under 1.0. Associative only up to rounding, as
with addition.

Parameter(s):
  none.
Return:
  A monoid by value over `double`.
*/
struct d_monoid
d_monoid_product_double
(
    void
)
{
    return d_monoid_make(
        d_semigroup_make(&d_internal_monoid_mappend_product_double,
                         NULL,
                         sizeof(double)),
        &d_internal_monoid_mempty_product_double);
}

/*
d_monoid_min_double
  Floating-point minimum under DBL_MAX.

Parameter(s):
  none.
Return:
  A monoid by value over `double`.
*/
struct d_monoid
d_monoid_min_double
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_min_double,
                                          NULL,
                                          sizeof(double)),
                         &d_internal_monoid_mempty_min_double);
}

/*
d_monoid_max_double
  Floating-point maximum under -DBL_MAX, matching the C++ face's use of
`numeric_limits<T>::lowest()` rather than the smallest positive value.

Parameter(s):
  none.
Return:
  A monoid by value over `double`.
*/
struct d_monoid
d_monoid_max_double
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_max_double,
                                          NULL,
                                          sizeof(double)),
                         &d_internal_monoid_mempty_max_double);
}

/*
d_monoid_all
  Boolean conjunction under true.

Parameter(s):
  none.
Return:
  A monoid by value over `bool`.
*/
struct d_monoid
d_monoid_all
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_all,
                                          NULL,
                                          sizeof(bool)),
                         &d_internal_monoid_mempty_all);
}

/*
d_monoid_any
  Boolean disjunction under false.

Parameter(s):
  none.
Return:
  A monoid by value over `bool`.
*/
struct d_monoid
d_monoid_any
(
    void
)
{
    return d_monoid_make(d_semigroup_make(&d_internal_monoid_mappend_any,
                                          NULL,
                                          sizeof(bool)),
                         &d_internal_monoid_mempty_any);
}
