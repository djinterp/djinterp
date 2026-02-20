/******************************************************************************
* djinterp [test]                                  compose_tests_sa_helpers.h
*
*   Shared helper transformer and consumer functions used across compose test
* files. Extracted to avoid duplication between transformer and partial
* application tests. All helpers are declared static inline so each
* translation unit gets its own copy without linker conflicts.
*
*
* path:      \tests\functional\compose\compose_tests_sa_helpers.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.10
******************************************************************************/

#ifndef DJINTERP_TESTS_COMPOSE_SA_HELPERS_
#define DJINTERP_TESTS_COMPOSE_SA_HELPERS_ 1


///////////////////////////////////////////////////////////////////////////////
///             TRANSFORMER HELPERS                                         ///
///////////////////////////////////////////////////////////////////////////////

// transform_double
//   helper: doubles an integer value.
// Returns false if _input or _output is NULL.
static inline bool
transform_double
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = (*in) * 2;

    return true;
}

// transform_add_10
//   helper: adds 10 to an integer value.
// Returns false if _input or _output is NULL.
static inline bool
transform_add_10
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = (*in) + 10;

    return true;
}

// transform_negate
//   helper: negates an integer value.
// Returns false if _input or _output is NULL.
static inline bool
transform_negate
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = -(*in);

    return true;
}

// transform_multiply_by_context
//   helper: multiplies an integer by the int value provided through
// _context. Returns false if any pointer is NULL.
static inline bool
transform_multiply_by_context
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;
    const int* multiplier;

    if ( (!_input)  ||
         (!_output) ||
         (!_context) )
    {
        return false;
    }

    in         = (const int*)_input;
    out        = (int*)_output;
    multiplier = (const int*)_context;

    *out = (*in) * (*multiplier);

    return true;
}

// transform_add_context
//   helper: adds the int value provided through _context to an integer.
// Returns false if any pointer is NULL.
static inline bool
transform_add_context
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;
    const int* addend;

    if ( (!_input)  ||
         (!_output) ||
         (!_context) )
    {
        return false;
    }

    in     = (const int*)_input;
    out    = (int*)_output;
    addend = (const int*)_context;

    *out = (*in) + (*addend);

    return true;
}

// transform_always_fails
//   helper: transformer that always returns false regardless of input.
static inline bool
transform_always_fails
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_input;
    (void)_output;
    (void)_context;

    return false;
}

// transform_identity
//   helper: copies the input int directly to output (identity transform).
// Returns false if _input or _output is NULL.
static inline bool
transform_identity
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = *in;

    return true;
}

// transform_square
//   helper: squares an integer value.
// Returns false if _input or _output is NULL.
static inline bool
transform_square
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = (*in) * (*in);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///             CONSUMER HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// consumer_context
//   struct: accumulator context for consumer testing.
struct consumer_context
{
    int sum;
    int count;
};

// consumer_accumulate
//   helper: adds integer element value to the context's running sum and
// increments the context's count. Does nothing if either pointer is NULL.
static inline void
consumer_accumulate
(
    void* _element,
    void* _context
)
{
    int*                     value;
    struct consumer_context* ctx;

    if ( (!_element) ||
         (!_context) )
    {
        return;
    }

    value = (int*)_element;
    ctx   = (struct consumer_context*)_context;

    ctx->sum += *value;
    ctx->count++;

    return;
}

// consumer_double_in_place
//   helper: doubles the integer element value in place. Does nothing if
// _element is NULL.
static inline void
consumer_double_in_place
(
    void* _element,
    void* _context
)
{
    int* value;

    (void)_context;

    if (!_element)
    {
        return;
    }

    value  = (int*)_element;
    *value = (*value) * 2;

    return;
}

// consumer_add_context
//   helper: adds the int value from _context to the element in place.
// Does nothing if either pointer is NULL.
static inline void
consumer_add_context
(
    void* _element,
    void* _context
)
{
    int* value;
    int* addend;

    if ( (!_element) ||
         (!_context) )
    {
        return;
    }

    value  = (int*)_element;
    addend = (int*)_context;

    *value += *addend;

    return;
}

// consumer_noop
//   helper: no-op consumer that does nothing.
static inline void
consumer_noop
(
    void* _element,
    void* _context
)
{
    (void)_element;
    (void)_context;

    return;
}


#endif  // DJINTERP_TESTS_COMPOSE_SA_HELPERS_
