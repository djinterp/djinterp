#include "..\..\inc\functional\functional_common.h"


/*
d_functional_identity_transformer
  Copies the input element to the output unchanged.

Parameter(s):
  _input:   pointer to the input element.
  _output:  pointer to the output destination.
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if _input and _output were both non-NULL, or
  - false, if either was NULL.
*/
bool
d_functional_identity_transformer
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    // validate parameters
    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    // a shallow pointer copy is the only safe generic operation
    *(const void**)_output = _input;

    return true;
}

/*
d_functional_identity_predicate
  Always returns true for any non-NULL element.

Parameter(s):
  _element: pointer to the element to test.
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if _element was non-NULL, or
  - false, if _element was NULL.
*/
bool
d_functional_identity_predicate
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (_element != NULL);
}

/*
d_functional_constant_true
  Always returns true regardless of input.

Parameter(s):
  _element: pointer to the element (ignored).
  _context: unused; may be NULL.
Return:
  true, always.
*/
bool
d_functional_constant_true
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
d_functional_constant_false
  Always returns false regardless of input.

Parameter(s):
  _element: pointer to the element (ignored).
  _context: unused; may be NULL.
Return:
  false, always.
*/
bool
d_functional_constant_false
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
d_functional_compare_int
  Three-way comparison for int values.

Parameter(s):
  _a:       pointer to the first int.
  _b:       pointer to the second int.
  _context: unused; may be NULL.
Return:
  A negative value if *_a < *_b, 0 if *_a == *_b, or a positive value if
*_a > *_b. Returns 0 if either pointer is NULL.
*/
int
d_functional_compare_int
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    int a;
    int b;

    (void)_context;

    // validate parameters
    if ( (!_a) ||
         (!_b) )
    {
        return 0;
    }

    a = *(const int*)_a;
    b = *(const int*)_b;

    // avoid potential overflow from subtraction
    if (a < b)
    {
        return -1;
    }

    if (a > b)
    {
        return 1;
    }

    return 0;
}

/*
d_functional_compare_size_t
  Three-way comparison for size_t values.

Parameter(s):
  _a:       pointer to the first size_t.
  _b:       pointer to the second size_t.
  _context: unused; may be NULL.
Return:
  A negative value if *_a < *_b, 0 if *_a == *_b, or a positive value if
*_a > *_b. Returns 0 if either pointer is NULL.
*/
int
d_functional_compare_size_t
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    size_t a;
    size_t b;

    (void)_context;

    // validate parameters
    if ( (!_a) ||
         (!_b) )
    {
        return 0;
    }

    a = *(const size_t*)_a;
    b = *(const size_t*)_b;

    if (a < b)
    {
        return -1;
    }

    if (a > b)
    {
        return 1;
    }

    return 0;
}

/*
d_functional_compare_double
  Three-way comparison for double values.

Parameter(s):
  _a:       pointer to the first double.
  _b:       pointer to the second double.
  _context: unused; may be NULL.
Return:
  A negative value if *_a < *_b, 0 if *_a == *_b, or a positive value if
*_a > *_b. Returns 0 if either pointer is NULL.
*/
int
d_functional_compare_double
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    double a;
    double b;

    (void)_context;

    // validate parameters
    if ( (!_a) ||
         (!_b) )
    {
        return 0;
    }

    a = *(const double*)_a;
    b = *(const double*)_b;

    if (a < b)
    {
        return -1;
    }

    if (a > b)
    {
        return 1;
    }

    return 0;
}

/*
d_functional_equal_int
  Equality comparison for int values.

Parameter(s):
  _a:       pointer to the first int.
  _b:       pointer to the second int.
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if *_a == *_b, or
  - false, if *_a != *_b or either pointer was NULL.
*/
bool
d_functional_equal_int
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    (void)_context;

    // validate parameters
    if ( (!_a) ||
         (!_b) )
    {
        return false;
    }

    return (*(const int*)_a == *(const int*)_b);
}

/*
d_functional_equal_size_t
  Equality comparison for size_t values.

Parameter(s):
  _a:       pointer to the first size_t.
  _b:       pointer to the second size_t.
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if *_a == *_b, or
  - false, if *_a != *_b or either pointer was NULL.
*/
bool
d_functional_equal_size_t
(
    const void* _a,
    const void* _b,
    void*       _context
)
{
    (void)_context;

    // validate parameters
    if ( (!_a) ||
         (!_b) )
    {
        return false;
    }

    return (*(const size_t*)_a == *(const size_t*)_b);
}

/*
d_functional_is_null
  Tests whether an element pointer is NULL. Intended for use with arrays of
pointers, where each element is itself a pointer. Dereferences the element
to read the stored pointer value.

Parameter(s):
  _element: pointer to the element to test (pointer to a pointer).
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the pointer stored at *_element is NULL, or
  - false, if the pointer stored at *_element is non-NULL.
  Returns true if _element itself is NULL.
*/
bool
d_functional_is_null
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    // a NULL element pointer is considered NULL
    if (!_element)
    {
        return true;
    }

    return (*(const void* const*)_element == NULL);
}

/*
d_functional_is_not_null
  Tests whether an element pointer is non-NULL. Intended for use with arrays
of pointers, where each element is itself a pointer. Dereferences the
element to read the stored pointer value.

Parameter(s):
  _element: pointer to the element to test (pointer to a pointer).
  _context: unused; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the pointer stored at *_element is non-NULL, or
  - false, if the pointer stored at *_element is NULL.
  Returns false if _element itself is NULL.
*/
bool
d_functional_is_not_null
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    // a NULL element pointer is not "not null"
    if (!_element)
    {
        return false;
    }

    return (*(const void* const*)_element != NULL);
}

/*
d_functional_map
  Applies a transformer to each element of an input array, writing the
results to an output array.

Parameter(s):
  _input:        pointer to the input array.
  _output:       pointer to the output array; must be at least
                 _count * _element_size bytes.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _transform:    transformer function to apply to each element.
  _context:      context forwarded to _transform; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all parameters were valid and every transformation succeeded, or
  - false, if any parameter was NULL/zero or any transformation failed.
*/
bool
d_functional_map
(
    const void*   _input,
    void*         _output,
    size_t        _count,
    size_t        _element_size,
    fn_transformer _transform,
    void*         _context
)
{
    const unsigned char* src;
    unsigned char*       dst;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_output)           ||
         (!_transform)        ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;
    dst = (unsigned char*)_output;

    // apply the transformer to each element
    for (i = 0; i < _count; i++)
    {
        if (!_transform(src + (i * _element_size),
                        dst + (i * _element_size),
                        _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_functional_filter
  Copies elements from an input array to an output array for which the
predicate returns true. Elements are packed contiguously in the output.

Parameter(s):
  _input:        pointer to the input array.
  _output:       pointer to the output array; must be at least
                 _count * _element_size bytes.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  The number of elements written to _output, or 0 if any parameter was
NULL/zero.
*/
size_t
d_functional_filter
(
    const void* _input,
    void*       _output,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    unsigned char*       dst;
    size_t               out_count;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_output)           ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return 0;
    }

    src       = (const unsigned char*)_input;
    dst       = (unsigned char*)_output;
    out_count = 0;

    // copy elements that pass the predicate
    for (i = 0; i < _count; i++)
    {
        if (_test(src + (i * _element_size), _context))
        {
            memcpy(dst + (out_count * _element_size),
                   src + (i * _element_size),
                   _element_size);
            out_count++;
        }
    }

    return out_count;
}

/*
d_functional_fold_left
  Performs a left fold (reduce) over an input array. The accumulator is
combined with each element from left to right.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _accumulator:  pointer to the accumulated value; serves as both the
                 initial value and the destination for the result.
  _combine:      accumulator function applied at each step.
  _context:      context forwarded to _combine; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all parameters were valid and every accumulation succeeded, or
  - false, if any parameter was NULL/zero or any accumulation failed.
*/
bool
d_functional_fold_left
(
    const void*   _input,
    size_t        _count,
    size_t        _element_size,
    void*         _accumulator,
    fn_accumulator _combine,
    void*         _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_accumulator)      ||
         (!_combine)          ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;

    // accumulate from left to right
    for (i = 0; i < _count; i++)
    {
        if (!_combine(_accumulator,
                      src + (i * _element_size),
                      _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_functional_fold_right
  Performs a right fold (reduce) over an input array. The accumulator is
combined with each element from right to left.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _accumulator:  pointer to the accumulated value; serves as both the
                 initial value and the destination for the result.
  _combine:      accumulator function applied at each step.
  _context:      context forwarded to _combine; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all parameters were valid and every accumulation succeeded, or
  - false, if any parameter was NULL/zero or any accumulation failed.
*/
bool
d_functional_fold_right
(
    const void*   _input,
    size_t        _count,
    size_t        _element_size,
    void*         _accumulator,
    fn_accumulator _combine,
    void*         _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_accumulator)      ||
         (!_combine)          ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;

    // accumulate from right to left
    for (i = _count; i > 0; i--)
    {
        if (!_combine(_accumulator,
                      src + ((i - 1) * _element_size),
                      _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_functional_for_each
  Applies a consumer function to each element of a mutable input array.

Parameter(s):
  _input:        pointer to the input array (mutable).
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _apply:        consumer function to apply to each element.
  _context:      context forwarded to _apply; may be NULL.
Return:
  none.
*/
void
d_functional_for_each
(
    void*      _input,
    size_t     _count,
    size_t     _element_size,
    fn_consumer _apply,
    void*      _context
)
{
    unsigned char* src;
    size_t         i;

    // validate parameters
    if ( (!_input)            ||
         (!_apply)            ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return;
    }

    src = (unsigned char*)_input;

    // apply to each element
    for (i = 0; i < _count; i++)
    {
        _apply(src + (i * _element_size), _context);
    }

    return;
}

/*
d_functional_for_each_const
  Applies a const consumer function to each element of an immutable input
array.

Parameter(s):
  _input:        pointer to the input array (immutable).
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _apply:        const consumer function to apply to each element.
  _context:      context forwarded to _apply; may be NULL.
Return:
  none.
*/
void
d_functional_for_each_const
(
    const void*      _input,
    size_t           _count,
    size_t           _element_size,
    fn_consumer_const _apply,
    void*            _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_apply)            ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return;
    }

    src = (const unsigned char*)_input;

    // apply to each element
    for (i = 0; i < _count; i++)
    {
        _apply(src + (i * _element_size), _context);
    }

    return;
}

/*
d_functional_any
  Tests whether any element in the input array satisfies the predicate.
Short-circuits on the first match.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if at least one element satisfied the predicate, or
  - false, if no element satisfied the predicate or any parameter was
    NULL/zero.
*/
bool
d_functional_any
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;

    // short-circuit on first match
    for (i = 0; i < _count; i++)
    {
        if (_test(src + (i * _element_size), _context))
        {
            return true;
        }
    }

    return false;
}

/*
d_functional_all
  Tests whether all elements in the input array satisfy the predicate.
Short-circuits on the first failure.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if every element satisfied the predicate, or
  - false, if any element failed the predicate or any parameter was
    NULL/zero.
*/
bool
d_functional_all
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;

    // short-circuit on first failure
    for (i = 0; i < _count; i++)
    {
        if (!_test(src + (i * _element_size), _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_functional_none
  Tests whether no element in the input array satisfies the predicate.
Short-circuits on the first match.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if no element satisfied the predicate, or
  - false, if any element satisfied the predicate or any parameter was
    NULL/zero.
*/
bool
d_functional_none
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return false;
    }

    src = (const unsigned char*)_input;

    // short-circuit on first match
    for (i = 0; i < _count; i++)
    {
        if (_test(src + (i * _element_size), _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_functional_count_if
  Counts the number of elements in the input array that satisfy the
predicate.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  The number of elements for which the predicate returned true, or 0 if any
parameter was NULL/zero.
*/
size_t
d_functional_count_if
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    size_t               result;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return 0;
    }

    src    = (const unsigned char*)_input;
    result = 0;

    // count elements that pass the predicate
    for (i = 0; i < _count; i++)
    {
        if (_test(src + (i * _element_size), _context))
        {
            result++;
        }
    }

    return result;
}

/*
d_functional_find_if
  Returns a pointer to the first element in the input array that satisfies
the predicate. Short-circuits on the first match.

Parameter(s):
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _test:         predicate function to test each element.
  _context:      context forwarded to _test; may be NULL.
Return:
  A pointer to the first matching element, or NULL if no element matched or
any parameter was NULL/zero. Note: the returned pointer is into the original
input array; the const qualifier is cast away to match the generic return
type.
*/
void*
d_functional_find_if
(
    const void* _input,
    size_t      _count,
    size_t      _element_size,
    fn_predicate _test,
    void*       _context
)
{
    const unsigned char* src;
    size_t               i;

    // validate parameters
    if ( (!_input)            ||
         (!_test)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        return NULL;
    }

    src = (const unsigned char*)_input;

    // short-circuit on first match
    for (i = 0; i < _count; i++)
    {
        if (_test(src + (i * _element_size), _context))
        {
            return (void*)(src + (i * _element_size));
        }
    }

    return NULL;
}
