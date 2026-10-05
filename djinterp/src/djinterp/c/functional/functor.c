/*******************************************************************************
* djinterp [c]                                                         functor.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/functor.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/functor.h"


/*
d_internal_functor_map_maybe
  The Functor instance for `maybe`. The two carriers are both `struct d_maybe`,
holding whatever widths their cells declared, which is why no monomorphisation
is needed.

Parameter(s):
  _in:        a `struct d_maybe`.
  _out:       a `struct d_maybe` to write through.
  _transform: the mapping.
  _context:   context forwarded to `_transform`.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
static bool
d_internal_functor_map_maybe
(
    const void*    _in,
    void*          _out,
    fn_transformer _transform,
    void*          _context
)
{
    return d_maybe_map((const struct d_maybe*)_in,
                       (struct d_maybe*)_out,
                       _transform,
                       _context);
}

/*
d_internal_functor_map_result
  The Functor instance for `result`, mapping the success arm only.

Parameter(s):
  _in:        a `struct d_result`.
  _out:       a `struct d_result` to write through.
  _transform: the mapping.
  _context:   context forwarded to `_transform`.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
static bool
d_internal_functor_map_result
(
    const void*    _in,
    void*          _out,
    fn_transformer _transform,
    void*          _context
)
{
    return d_result_map((const struct d_result*)_in,
                        (struct d_result*)_out,
                        _transform,
                        _context);
}

/*
d_internal_applicative_pure_maybe
  `pure` for `maybe`: a value becomes a just.

Parameter(s):
  _out:   a `struct d_maybe` to write through.
  _value: the value to embed.
Return:
  A boolean value corresponding to whether the value was stored.
*/
static bool
d_internal_applicative_pure_maybe
(
    void*       _out,
    const void* _value
)
{
    return d_maybe_just((struct d_maybe*)_out, _value);
}

/*
d_internal_applicative_pure_result
  `pure` for `result`: a value becomes an ok.

Parameter(s):
  _out:   a `struct d_result` to write through.
  _value: the value to embed.
Return:
  A boolean value corresponding to whether the value was stored.
*/
static bool
d_internal_applicative_pure_result
(
    void*       _out,
    const void* _value
)
{
    return d_result_ok((struct d_result*)_out, _value);
}

/*
d_internal_applicative_map2_maybe
  `map2` for `maybe`: the combine runs only when both cells hold a value, and
the result is nothing otherwise. This is where the "effects are collected"
character of Applicative shows up -- absence on either side propagates without
either side knowing about the other.

Parameter(s):
  _first:   a `struct d_maybe`.
  _second:  a `struct d_maybe`.
  _out:     a `struct d_maybe` to write through.
  _combine: the binary function.
  _context: context forwarded to `_combine`.
Return:
  A boolean value corresponding to whether the output holds a value.
*/
static bool
d_internal_applicative_map2_maybe
(
    const void* _first,
    const void* _second,
    void*       _out,
    fn_zipper  _combine,
    void*       _context
)
{
    const struct d_maybe* first;
    const struct d_maybe* second;
    struct d_maybe*       out;

    first  = (const struct d_maybe*)_first;
    second = (const struct d_maybe*)_second;
    out    = (struct d_maybe*)_out;

    // an unusable request leaves the destination alone
    if ( (!d_maybe_is_valid(out)) ||
         (!_combine)              )
    {
        return false;
    }

    // absence on either side propagates
    if ( (!d_maybe_is_just(first))  ||
         (!d_maybe_is_just(second)) )
    {
        d_maybe_nothing(out);

        return false;
    }

    // a combine that declines yields nothing rather than a stale value
    if (!_combine(first->value, second->value, out->value, _context))
    {
        d_maybe_nothing(out);

        return false;
    }

    *(out->has_value) = true;

    return true;
}

/*
d_internal_applicative_map2_result
  `map2` for `result`: the combine runs only when both carriers hold a success,
and the FIRST error wins otherwise. Left-biased error selection is the choice
that makes a chain of validations report the earliest failure.

Parameter(s):
  _first:   a `struct d_result`.
  _second:  a `struct d_result`.
  _out:     a `struct d_result` to write through.
  _combine: the binary function.
  _context: context forwarded to `_combine`.
Return:
  A boolean value corresponding to whether the output holds a success.
*/
static bool
d_internal_applicative_map2_result
(
    const void* _first,
    const void* _second,
    void*       _out,
    fn_zipper  _combine,
    void*       _context
)
{
    const struct d_result* first;
    const struct d_result* second;
    struct d_result*       out;

    first  = (const struct d_result*)_first;
    second = (const struct d_result*)_second;
    out    = (struct d_result*)_out;

    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(out)) ||
         (!_combine)               )
    {
        return false;
    }

    // the earliest error wins, so a validation chain reports the first failure
    if (!d_result_is_ok(first))
    {
        d_result_err(out, first->payload);

        return false;
    }

    if (!d_result_is_ok(second))
    {
        d_result_err(out, second->payload);

        return false;
    }

    // a combine that declines leaves the destination untouched
    if (!_combine(first->payload, second->payload, out->payload, _context))
    {
        return false;
    }

    *(out->is_ok) = true;

    return true;
}

/*
d_internal_monad_bind_maybe
  The Monad instance for `maybe`. The Kleisli arrow is type-erased over the
output carrier, so one arrow shape serves every carrier.

Parameter(s):
  _in:      a `struct d_maybe`.
  _out:     a `struct d_maybe` to write through.
  _arrow:   the Kleisli arrow.
  _context: context forwarded to `_arrow`.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
static bool
d_internal_monad_bind_maybe
(
    const void* _in,
    void*       _out,
    fn_kleisli  _arrow,
    void*       _context
)
{
    const struct d_maybe* in;
    struct d_maybe*       out;

    in  = (const struct d_maybe*)_in;
    out = (struct d_maybe*)_out;

    // an unusable request leaves the destination alone
    if ( (!d_maybe_is_valid(in))  ||
         (!d_maybe_is_valid(out)) ||
         (!_arrow)                )
    {
        return false;
    }

    // an empty input short-circuits without calling the arrow
    if (!d_maybe_is_just(in))
    {
        return d_maybe_nothing(out);
    }

    _arrow(in->value, out, _context);

    return true;
}

/*
d_internal_monad_bind_result
  The Monad instance for `result`. An error short-circuits and is carried across
unchanged.

Parameter(s):
  _in:      a `struct d_result`.
  _out:     a `struct d_result` to write through.
  _arrow:   the Kleisli arrow.
  _context: context forwarded to `_arrow`.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
static bool
d_internal_monad_bind_result
(
    const void* _in,
    void*       _out,
    fn_kleisli  _arrow,
    void*       _context
)
{
    const struct d_result* in;
    struct d_result*       out;

    in  = (const struct d_result*)_in;
    out = (struct d_result*)_out;

    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(in))  ||
         (!d_result_is_valid(out)) ||
         (!_arrow)                 )
    {
        return false;
    }

    // an error short-circuits without calling the arrow
    if (!d_result_is_ok(in))
    {
        return d_result_err(out, in->payload);
    }

    _arrow(in->payload, out, _context);

    return true;
}


/*
d_functor_maybe
  The Functor dictionary for `maybe`. Its carriers are `struct d_maybe`.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_functor
d_functor_maybe
(
    void
)
{
    struct d_functor functor;

    functor.map = &d_internal_functor_map_maybe;

    return functor;
}

/*
d_functor_result
  The Functor dictionary for `result`. Its carriers are `struct d_result`, and
the mapping touches the success arm only.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_functor
d_functor_result
(
    void
)
{
    struct d_functor functor;

    functor.map = &d_internal_functor_map_result;

    return functor;
}

/*
d_functor_is_valid
  Reports whether a Functor dictionary carries a mapping.

Parameter(s):
  _functor: the dictionary to inspect; may be NULL.
Return:
  A boolean value corresponding to whether the mapping is set.
*/
bool
d_functor_is_valid
(
    const struct d_functor* _functor
)
{
    // a NULL dictionary is not valid
    if (!_functor)
    {
        return false;
    }

    return (_functor->map != NULL);
}

/*
d_applicative_maybe
  The Applicative dictionary for `maybe`.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_applicative
d_applicative_maybe
(
    void
)
{
    struct d_applicative applicative;

    applicative.pure = &d_internal_applicative_pure_maybe;
    applicative.map2 = &d_internal_applicative_map2_maybe;

    return applicative;
}

/*
d_applicative_result
  The Applicative dictionary for `result`, with left-biased error selection.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_applicative
d_applicative_result
(
    void
)
{
    struct d_applicative applicative;

    applicative.pure = &d_internal_applicative_pure_result;
    applicative.map2 = &d_internal_applicative_map2_result;

    return applicative;
}

/*
d_applicative_is_valid
  Reports whether an Applicative dictionary carries both operations.

Parameter(s):
  _applicative: the dictionary to inspect; may be NULL.
Return:
  A boolean value corresponding to whether both operations are set.
*/
bool
d_applicative_is_valid
(
    const struct d_applicative* _applicative
)
{
    // a NULL dictionary is not valid
    if (!_applicative)
    {
        return false;
    }

    return ( (_applicative->pure != NULL) &&
             (_applicative->map2 != NULL) );
}

/*
d_monad_maybe
  The Monad dictionary for `maybe`.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_monad
d_monad_maybe
(
    void
)
{
    struct d_monad monad;

    monad.unit = &d_internal_applicative_pure_maybe;
    monad.bind = &d_internal_monad_bind_maybe;

    return monad;
}

/*
d_monad_result
  The Monad dictionary for `result`.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_monad
d_monad_result
(
    void
)
{
    struct d_monad monad;

    monad.unit = &d_internal_applicative_pure_result;
    monad.bind = &d_internal_monad_bind_result;

    return monad;
}

/*
d_monad_is_valid
  Reports whether a Monad dictionary carries both operations.

Parameter(s):
  _monad: the dictionary to inspect; may be NULL.
Return:
  A boolean value corresponding to whether both operations are set.
*/
bool
d_monad_is_valid
(
    const struct d_monad* _monad
)
{
    // a NULL dictionary is not valid
    if (!_monad)
    {
        return false;
    }

    return ( (_monad->unit != NULL) &&
             (_monad->bind != NULL) );
}

/*
d_functor_map
  Applies a Functor's mapping through its dictionary.

Parameter(s):
  _functor:   the carrier's dictionary.
  _in:        the input carrier.
  _out:       the output carrier.
  _transform: the mapping.
  _context:   context forwarded to `_transform`; may be NULL.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
bool
d_functor_map
(
    const struct d_functor* _functor,
    const void*             _in,
    void*                   _out,
    fn_transformer          _transform,
    void*                   _context
)
{
    // an unusable dictionary maps nothing
    if (!d_functor_is_valid(_functor))
    {
        return false;
    }

    return _functor->map(_in, _out, _transform, _context);
}

/*
d_applicative_pure
  Embeds a value through an Applicative's dictionary.

Parameter(s):
  _applicative: the carrier's dictionary.
  _out:         the output carrier.
  _value:       the value to embed.
Return:
  A boolean value corresponding to whether the value was embedded.
*/
bool
d_applicative_pure
(
    const struct d_applicative* _applicative,
    void*                       _out,
    const void*                 _value
)
{
    // an unusable dictionary embeds nothing
    if (!d_applicative_is_valid(_applicative))
    {
        return false;
    }

    return _applicative->pure(_out, _value);
}

/*
d_applicative_map2
  Lifts a binary function over two carriers through an Applicative's dictionary.

Parameter(s):
  _applicative: the carrier's dictionary.
  _first:       the first carrier.
  _second:      the second carrier.
  _out:         the output carrier.
  _combine:     the binary function.
  _context:     context forwarded to `_combine`; may be NULL.
Return:
  A boolean value corresponding to whether the output holds a value.
*/
bool
d_applicative_map2
(
    const struct d_applicative* _applicative,
    const void*                 _first,
    const void*                 _second,
    void*                       _out,
    fn_zipper                  _combine,
    void*                       _context
)
{
    // an unusable dictionary lifts nothing
    if (!d_applicative_is_valid(_applicative))
    {
        return false;
    }

    return _applicative->map2(_first, _second, _out, _combine, _context);
}

/*
d_monad_unit
  Embeds a value through a Monad's dictionary. The same operation as
`d_applicative_pure`, reachable without the Applicative dictionary.

Parameter(s):
  _monad: the carrier's dictionary.
  _out:   the output carrier.
  _value: the value to embed.
Return:
  A boolean value corresponding to whether the value was embedded.
*/
bool
d_monad_unit
(
    const struct d_monad* _monad,
    void*                 _out,
    const void*           _value
)
{
    // an unusable dictionary embeds nothing
    if (!d_monad_is_valid(_monad))
    {
        return false;
    }

    return _monad->unit(_out, _value);
}

/*
d_monad_bind
  Sequences a Kleisli arrow through a Monad's dictionary.

Parameter(s):
  _monad:   the carrier's dictionary.
  _in:      the input carrier.
  _out:     the output carrier.
  _arrow:   the Kleisli arrow.
  _context: context forwarded to `_arrow`; may be NULL.
Return:
  A boolean value corresponding to whether the output was left definite.
*/
bool
d_monad_bind
(
    const struct d_monad* _monad,
    const void*           _in,
    void*                 _out,
    fn_kleisli            _arrow,
    void*                 _context
)
{
    // an unusable dictionary sequences nothing
    if (!d_monad_is_valid(_monad))
    {
        return false;
    }

    return _monad->bind(_in, _out, _arrow, _context);
}
