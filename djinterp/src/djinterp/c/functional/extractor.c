/*******************************************************************************
* djinterp [c]                                                       extractor.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/extractor.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/extractor.h"


/*
d_extractor_make
  Binds a projection to its context so the pair travels as one value.

Parameter(s):
  _project: the projection `Source -> Target`; may be NULL, yielding an invalid
            extractor.
  _context: context forwarded to `_project`; may be NULL.
Return:
  An extractor by value.
*/
struct d_extractor
d_extractor_make
(
    fn_transformer _project,
    void*          _context
)
{
    struct d_extractor extractor;

    extractor.project = _project;
    extractor.context = _context;

    return extractor;
}

/*
d_extractor_is_valid
  Reports whether an extractor carries a projection.

Parameter(s):
  _extractor: the extractor to inspect; may be NULL.
Return:
  A boolean value corresponding to whether the projection is set.
*/
bool
d_extractor_is_valid
(
    const struct d_extractor* _extractor
)
{
    // a NULL extractor is not valid
    if (!_extractor)
    {
        return false;
    }

    return (_extractor->project != NULL);
}

/*
d_extract
  Applies a projection.

Parameter(s):
  _extractor: the projection to apply.
  _source:    the value to read from.
  _target:    destination for the projected feature.
Return:
  A boolean value corresponding to either:
  - true, if the projection produced a value, or
  - false, if it declined or the parameters were unusable.
*/
bool
d_extract
(
    const struct d_extractor* _extractor,
    const void*               _source,
    void*                     _target
)
{
    // an unusable request projects nothing
    if ( (!d_extractor_is_valid(_extractor)) ||
         (!_source)                          ||
         (!_target)                          )
    {
        return false;
    }

    return _extractor->project(_source, _target, _extractor->context);
}

/*
d_extractor_chain_init
  Composes two projections into `Source -> Mid -> Target`.

Parameter(s):
  _chain:   the chain to initialise.
  _first:   the projection `Source -> Mid`.
  _second:  the projection `Mid -> Target`.
  _scratch: caller-owned slot for the intermediate `Mid`.
Return:
  A boolean value corresponding to whether the chain was initialised.
*/
bool
d_extractor_chain_init
(
    struct d_extractor_chain* _chain,
    struct d_extractor        _first,
    struct d_extractor        _second,
    void*                     _scratch
)
{
    // an unusable request leaves the chain untouched
    if ( (!_chain)                          ||
         (!_scratch)                        ||
         (!d_extractor_is_valid(&_first))   ||
         (!d_extractor_is_valid(&_second))  )
    {
        return false;
    }

    _chain->first   = _first;
    _chain->second  = _second;
    _chain->scratch = _scratch;

    return true;
}

/*
d_extractor_chain_apply
  Runs a composed projection. Deliberately shaped as an `fn_transformer`, so a
chain is itself usable anywhere a single projection is -- which is what makes
composition associative in practice rather than only on paper.

Parameter(s):
  _source: the value to read from.
  _target: destination for the projected feature.
  _chain:  the `d_extractor_chain` to run.
Return:
  A boolean value corresponding to whether both projections produced a value.
*/
bool
d_extractor_chain_apply
(
    const void* _source,
    void*       _target,
    void*       _chain
)
{
    struct d_extractor_chain* chain;

    // an unusable chain projects nothing
    if (!_chain)
    {
        return false;
    }

    chain = (struct d_extractor_chain*)_chain;

    // the first projection must land before the second can read it
    if (!d_extract(&chain->first, _source, chain->scratch))
    {
        return false;
    }

    return d_extract(&chain->second, chain->scratch, _target);
}

/*
d_contramap_predicate_init
  Binds a predicate over `Target` to a projection, giving a predicate over
`Source`.

Parameter(s):
  _binding:   the binding to initialise.
  _extract:   the projection `Source -> Target`.
  _predicate: the test over `Target`.
  _context:   context forwarded to `_predicate`; may be NULL.
  _scratch:   caller-owned slot for the projected value.
Return:
  A boolean value corresponding to whether the binding was initialised.
*/
bool
d_contramap_predicate_init
(
    struct d_contramap_predicate* _binding,
    struct d_extractor            _extract,
    fn_predicate                  _predicate,
    void*                         _context,
    void*                         _scratch
)
{
    // an unusable request leaves the binding untouched
    if ( (!_binding)                          ||
         (!_predicate)                        ||
         (!_scratch)                          ||
         (!d_extractor_is_valid(&_extract))   )
    {
        return false;
    }

    _binding->extract   = _extract;
    _binding->predicate = _predicate;
    _binding->context   = _context;
    _binding->scratch   = _scratch;

    return true;
}

/*
d_contramap_test
  The contravariantly mapped predicate. Its signature IS `fn_predicate`, with
the binding travelling as the context, so it drops into `d_transducer_filter`,
`d_reducer_any` or anything else taking a predicate with no new overloads.

Parameter(s):
  _element: the `Source` value under test.
  _binding: the `d_contramap_predicate` to run.
Return:
  A boolean value corresponding to either:
  - true, if the projection succeeded and the predicate admitted it, or
  - false, otherwise. A failed projection reads as a rejection, since there is
    no feature to test.
*/
bool
d_contramap_test
(
    const void* _element,
    void*       _binding
)
{
    struct d_contramap_predicate* binding;

    // an unusable binding admits nothing
    if (!_binding)
    {
        return false;
    }

    binding = (struct d_contramap_predicate*)_binding;

    // a value whose feature cannot be read cannot satisfy a test on it
    if (!d_extract(&binding->extract, _element, binding->scratch))
    {
        return false;
    }

    return binding->predicate(binding->scratch, binding->context);
}

/*
d_contramap_comparator_init
  Binds an ordering over `Target` to a projection, giving an ordering over
`Source` -- which is what `sort_by_key` is.

Parameter(s):
  _binding:  the binding to initialise.
  _extract:  the projection `Source -> Target`.
  _precedes: the ordering over `Target`.
  _context:  context forwarded to `_precedes`; may be NULL.
  _left:     caller-owned slot for the left projection.
  _right:    caller-owned slot for the right projection.
Return:
  A boolean value corresponding to whether the binding was initialised.
*/
bool
d_contramap_comparator_init
(
    struct d_contramap_comparator* _binding,
    struct d_extractor             _extract,
    fn_binary_predicate            _precedes,
    void*                          _context,
    void*                          _left,
    void*                          _right
)
{
    // two distinct slots are required, since both operands are projected
    if ( (!_binding)                        ||
         (!_precedes)                       ||
         (!_left)                           ||
         (!_right)                          ||
         (_left == _right)                  ||
         (!d_extractor_is_valid(&_extract)) )
    {
        return false;
    }

    _binding->extract  = _extract;
    _binding->precedes = _precedes;
    _binding->context  = _context;
    _binding->left     = _left;
    _binding->right    = _right;

    return true;
}

/*
d_contramap_precedes
  The contravariantly mapped ordering. Its signature IS `fn_binary_predicate`,
so it drops straight into `d_sorted` and `d_is_sorted`.

Parameter(s):
  _left:    the left `Source` value.
  _right:   the right `Source` value.
  _binding: the `d_contramap_comparator` to run.
Return:
  A boolean value corresponding to either:
  - true, if the left value's feature strictly precedes the right's, or
  - false, otherwise. A failed projection on either side orders neither before
    the other, which keeps the ordering strict rather than inventing a rank.
*/
bool
d_contramap_precedes
(
    const void* _left,
    const void* _right,
    void*       _binding
)
{
    struct d_contramap_comparator* binding;

    // an unusable binding orders nothing
    if (!_binding)
    {
        return false;
    }

    binding = (struct d_contramap_comparator*)_binding;

    // both features must be readable for the comparison to mean anything
    if (!d_extract(&binding->extract, _left, binding->left))
    {
        return false;
    }

    if (!d_extract(&binding->extract, _right, binding->right))
    {
        return false;
    }

    return binding->precedes(binding->left,
                             binding->right,
                             binding->context);
}
