/******************************************************************************
* djinterp [test]                                 predicate_tests_sa_helpers.h
*
*   Shared helper predicate functions used across predicate test files.
*   Extracted to avoid duplication between constructor and evaluation tests.
*   All helpers are declared static inline so each translation unit gets its
* own copy without linker conflicts.
*
*
* path:      \tests\functional\predicate_tests_sa_helpers.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.10
******************************************************************************/

#ifndef DJINTERP_TESTS_PREDICATE_SA_HELPERS_
#define DJINTERP_TESTS_PREDICATE_SA_HELPERS_ 1


// pred_always_true
//   helper: predicate that always returns true regardless of input.
static inline bool
pred_always_true
(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}

// pred_always_false
//   helper: predicate that always returns false regardless of input.
static inline bool
pred_always_false
(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}

// pred_is_even
//   helper: predicate that returns true if the int element is even.
// Returns false if _element is NULL.
static inline bool
pred_is_even
(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 == 0);
}

// pred_is_odd
//   helper: predicate that returns true if the int element is odd.
// Returns false if _element is NULL.
static inline bool
pred_is_odd
(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 != 0);
}

// pred_is_positive
//   helper: predicate that returns true if the int element is > 0.
// Returns false if _element is NULL.
static inline bool
pred_is_positive
(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 0);
}

// pred_is_negative
//   helper: predicate that returns true if the int element is < 0.
// Returns false if _element is NULL.
static inline bool
pred_is_negative
(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value < 0);
}

// pred_greater_than_threshold
//   helper: predicate that returns true if the int element exceeds the
// int threshold provided through _context. Returns false if either
// _element or _context is NULL.
static inline bool
pred_greater_than_threshold
(
    const void* _element,
    void*       _context
)
{
    const int* value;
    const int* threshold;

    if ( (!_element) ||
         (!_context) )
    {
        return false;
    }

    value     = (const int*)_element;
    threshold = (const int*)_context;

    return (*value > *threshold);
}


#endif  // DJINTERP_TESTS_PREDICATE_SA_HELPERS_
