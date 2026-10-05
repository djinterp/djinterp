/*******************************************************************************
* djinterp [c]                                               functional_common.c
*
* The functional core's utilities and higher-order functions.
*   Defines what functional_common.h declares in its sections IX and X. Every
* higher-order function hands its callback a pointer to the element, never
* the element itself, and treats a missing callback, or a missing array with
* elements to visit, as a failure: false, 0, or NULL, as the return allows.
*
*
* path:      /src/djinterp/c/functional/functional_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/functional_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcpy


/*
d_internal_functional_valid
  The shared argument test: a callback, and an array whenever there is
anything to visit in it.
*/
static bool
d_internal_functional_valid(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    bool        _has_callback
)
{
    return ( (_has_callback) &&
             ( (_count == 0u) ||
               ( (_input != NULL) && (_element_size > 0u) ) ) );
}

/*
d_internal_functional_at
  The address of element _i.
*/
static const void*
d_internal_functional_at(
    const void* _input,
    size_t      _i,
    size_t      _element_size
)
{
    return (const unsigned char*)_input + (_i * _element_size);
}

/*
d_functional_identity_transformer
  An element's size is not in a transformer's signature, so the identity
reads it from the context: `_context` must point to a size_t holding it.
Without one there is nothing it can safely copy, and it fails.
*/
bool
d_functional_identity_transformer(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const size_t* const size = _context;

    // the size is required, and so are both ends
    if ( (!size)   ||
         (!_input) ||
         (!_output) )
    {
        return false;
    }

    memcpy(_output, _input, *size);

    return true;
}

/*
d_functional_identity_predicate
  The element as its own truth value, for arrays of bool; a missing element
is false.
*/
bool
d_functional_identity_predicate(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return ( (_element != NULL) &&
             (*(const bool*)_element) );
}

bool
d_functional_constant_true(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}

bool
d_functional_constant_false(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}

/*
d_internal_functional_order
  -1, 0 or 1 for the pointers' order, used when either is missing: a missing
operand sorts first, and two missing ones are equal.
*/
static int
d_internal_functional_order(
    const void* _a,
    const void* _b
)
{
    if (_a == _b)
    {
        return 0;
    }

    return (!_a) ? -1 : 1;
}

int
d_functional_compare_int(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    (void)_context;

    // a missing operand sorts first
    if ( (!_a) ||
         (!_b) )
    {
        return d_internal_functional_order(_a, _b);
    }

    const int a = *(const int*)_a;
    const int b = *(const int*)_b;

    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

/*
d_functional_compare_double
  A total order: NaN sorts after every number and equals another NaN, so a
sort over doubles that contain NaN still terminates and is deterministic.
*/
int
d_functional_compare_double(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    (void)_context;

    // a missing operand sorts first
    if ( (!_a) ||
         (!_b) )
    {
        return d_internal_functional_order(_a, _b);
    }

    const double a     = *(const double*)_a;
    const double b     = *(const double*)_b;
    const bool   a_nan = (a != a);
    const bool   b_nan = (b != b);

    // NaN after every number, and equal to itself
    if ( (a_nan) ||
         (b_nan) )
    {
        return (a_nan == b_nan) ? 0 : (a_nan ? 1 : -1);
    }

    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

int
d_functional_compare_size_t(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    (void)_context;

    // a missing operand sorts first
    if ( (!_a) ||
         (!_b) )
    {
        return d_internal_functional_order(_a, _b);
    }

    const size_t a = *(const size_t*)_a;
    const size_t b = *(const size_t*)_b;

    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

bool
d_functional_equal_int(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    return (d_functional_compare_int(_a, _b, _context) == 0);
}

bool
d_functional_equal_size_t(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    return (d_functional_compare_size_t(_a, _b, _context) == 0);
}

/*
d_functional_is_null
  For arrays of pointers: the element holds a NULL pointer. A missing element
counts as null too.
*/
bool
d_functional_is_null(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return ( (_element == NULL) ||
             (*(const void* const*)_element == NULL) );
}

bool
d_functional_is_not_null(
    const void* _element,
    void*       _context
)
{
    return !d_functional_is_null(_element, _context);
}

/*
d_functional_map
  Output elements have the input's size; a transformer that fails stops the
map where it failed, and the elements already written stay written.
*/
bool
d_functional_map(
    const void*    _input,
    void*          _output,
    size_t         _count,
    size_t         _element_size,
    fn_transformer _transform,
    void*          _context
)
{
    // an output is needed whenever there is anything to map
    if ( (!d_internal_functional_valid(_input,
                                       _count,
                                       _element_size,
                                       (_transform != NULL))) ||
         ( (_count > 0u) && (!_output) ) )
    {
        return false;
    }

    // transform each element into its slot, in order
    for (size_t i = 0u; i < _count; ++i)
    {
        if (!_transform(d_internal_functional_at(_input, i, _element_size),
                        (unsigned char*)_output + (i * _element_size),
                        _context))
        {
            return false;
        }
    }

    return true;
}

bool
d_functional_fold_left(
    const void*    _input,
    size_t         _count,
    size_t         _element_size,
    void*          _accumulator,
    fn_accumulator _combine,
    void*          _context
)
{
    // an accumulator is always needed
    if ( (!d_internal_functional_valid(_input,
                                       _count,
                                       _element_size,
                                       (_combine != NULL))) ||
         (!_accumulator) )
    {
        return false;
    }

    // combine from the first element to the last
    for (size_t i = 0u; i < _count; ++i)
    {
        if (!_combine(_accumulator,
                      d_internal_functional_at(_input, i, _element_size),
                      _context))
        {
            return false;
        }
    }

    return true;
}

bool
d_functional_fold_right(
    const void*    _input,
    size_t         _count,
    size_t         _element_size,
    void*          _accumulator,
    fn_accumulator _combine,
    void*          _context
)
{
    // an accumulator is always needed
    if ( (!d_internal_functional_valid(_input,
                                       _count,
                                       _element_size,
                                       (_combine != NULL))) ||
         (!_accumulator) )
    {
        return false;
    }

    // combine from the last element to the first
    for (size_t i = _count; i > 0u; --i)
    {
        if (!_combine(_accumulator,
                      d_internal_functional_at(_input, i - 1u, _element_size),
                      _context))
        {
            return false;
        }
    }

    return true;
}

void
d_functional_for_each(
    void*       _input,
    size_t      _count,
    size_t      _element_size,
    fn_consumer _apply,
    void*       _context
)
{
    // nothing to apply, or nothing to apply it to
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_apply != NULL)))
    {
        return;
    }

    // apply to each element, in order
    for (size_t i = 0u; i < _count; ++i)
    {
        _apply((unsigned char*)_input + (i * _element_size), _context);
    }

    return;
}

void
d_functional_for_each_const(
    const void*       _input,
    size_t            _count,
    size_t            _element_size,
    fn_consumer_const _apply,
    void*             _context
)
{
    // nothing to apply, or nothing to apply it to
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_apply != NULL)))
    {
        return;
    }

    // apply to each element, in order
    for (size_t i = 0u; i < _count; ++i)
    {
        _apply(d_internal_functional_at(_input, i, _element_size), _context);
    }

    return;
}

/*
d_functional_all
  Vacuously true for no elements; false for invalid arguments, since nothing
was shown to hold.
*/
bool
d_functional_all(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    // nothing can be vouched for without a test
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_test != NULL)))
    {
        return false;
    }

    // the first failure decides
    for (size_t i = 0u; i < _count; ++i)
    {
        if (!_test(d_internal_functional_at(_input, i, _element_size),
                   _context))
        {
            return false;
        }
    }

    return true;
}

bool
d_functional_any(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    return (d_functional_find_if(_input,
                                 _count,
                                 _element_size,
                                 _test,
                                 _context) != NULL);
}

/*
d_functional_none
  Vacuously true for no elements; false for invalid arguments, as for all.
*/
bool
d_functional_none(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    // nothing can be vouched for without a test
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_test != NULL)))
    {
        return false;
    }

    return (d_functional_find_if(_input,
                                 _count,
                                 _element_size,
                                 _test,
                                 _context) == NULL);
}

size_t
d_functional_count_if(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t matches = 0u;

    // nothing to count
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_test != NULL)))
    {
        return 0u;
    }

    // count every element the test accepts
    for (size_t i = 0u; i < _count; ++i)
    {
        if (_test(d_internal_functional_at(_input, i, _element_size),
                  _context))
        {
            ++matches;
        }
    }

    return matches;
}

void*
d_functional_find_if(
    const void*  _input,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    // nothing to search
    if (!d_internal_functional_valid(_input,
                                     _count,
                                     _element_size,
                                     (_test != NULL)))
    {
        return NULL;
    }

    // the first element the test accepts
    for (size_t i = 0u; i < _count; ++i)
    {
        const void* const element =
            d_internal_functional_at(_input, i, _element_size);

        if (_test(element, _context))
        {
            return (void*)element;
        }
    }

    return NULL;
}
