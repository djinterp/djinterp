/******************************************************************************
* djinterp [test]                                                     assert.c
*
*   Implementation of runtime assertion functions for unit testing.
*
*
* path:      \src\test\assert.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2023.04.29
******************************************************************************/

#include "..\..\inc\test\assert.h"


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_assert_default_compare
  Performs default pointer comparison for assertion operations.  Returns
  non-zero if both pointers point to the same address, zero otherwise.

Parameter(s):
  _expected: pointer to the expected value
  _actual:   pointer to the actual value

Return:
  Non-zero (true) if pointers are equal, zero (false) otherwise.
*/
int
d_assert_default_compare
(
    const void* _expected,
    const void* _actual
)
{
    return (_expected == _actual);
}


/******************************************************************************
 * ASSERTION CREATION FUNCTIONS
 *****************************************************************************/

/*
d_assert_new
  Creates a new d_assert with the specified result and corresponding message.

Parameter(s):
  _expression:    boolean value indicating if this assertion passed (true) or
                  failed (false)
  _message_true:  message to use if expression is true
  _message_false: message to use if expression is false

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert* 
d_assert_new
(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));

    if (new_assertion)
    {
        new_assertion->result  = _expression;
        new_assertion->message = _expression
                                 ? _message_true
                                 : _message_false;
    }

    return new_assertion;
}


/******************************************************************************
 * BOOLEAN ASSERTION FUNCTIONS
 *****************************************************************************/

/*
d_assert_true
  Asserts that the expression is true.  The resulting d_assert has result
  equal to true if, and only if, the expression evaluates to true.

Parameter(s):
  _expression:    boolean expression to evaluate
  _message_true:  message to use if assertion passes (expression is true)
  _message_false: message to use if assertion fails (expression is false)

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_true
(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));

    if (new_assertion)
    {
        new_assertion->result  = _expression;
        new_assertion->message = _expression
                                 ? _message_true
                                 : _message_false;
    }

    return new_assertion;
}

/*
d_assert_false
  Asserts that the expression is false.  The resulting d_assert has result
  equal to true if, and only if, the expression evaluates to false.

Parameter(s):
  _expression:    boolean expression to evaluate
  _message_true:  message to use if assertion passes (expression is false)
  _message_false: message to use if assertion fails (expression is true)

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_false
(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));

    if (new_assertion)
    {
        new_assertion->result  = !_expression;
        new_assertion->message = !_expression
                                 ? _message_true
                                 : _message_false;
    }

    return new_assertion;
}


/******************************************************************************
 * COMPARISON ASSERTION FUNCTIONS
 *****************************************************************************/

/*
d_assert_eq
  Asserts that two values are EQUAL using the provided comparator.  If no
  comparator is provided, compares the raw pointer values using the equality
  operator (returns true only if both pointers point to the same address).

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if values are equal
  _message_false: message to use if values are not equal

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert* 
d_assert_eq
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));

    if (new_assertion)
    {
        new_assertion->result  = (_comparator) 
                                 ? (_comparator(_a, _b) == 0)
                                 : _a == _b;
        new_assertion->message = new_assertion->result
                                 ? _message_true
                                 : _message_false;
    }

    return new_assertion;
}

/*
d_assert_neq
  Asserts that two values are NOT EQUAL using the provided comparator.  If no
  comparator is provided, compares the raw pointer values using the inequality
  operator (returns true only if both pointers do not point to the same 
  address).

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if values are not equal
  _message_false: message to use if values are equal

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_neq
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(!=);
}

/*
d_assert_lt
  Asserts that the first value is LESS THAN the second using the provided
  comparator.  If no comparator is provided, compares the raw pointer values
  using the less-than operator.

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if _a is less than _b
  _message_false: message to use if _a is not less than _b

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_lt
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(<)
}

/*
d_assert_lt_eq
  Asserts that the first value is LESS THAN OR EQUAL TO the second using the
  provided comparator.  If no comparator is provided, compares the raw pointer
  values using the less-than-or-equal operator.

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if _a is less than or equal to _b
  _message_false: message to use if _a is greater than _b

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_lt_eq
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(<=)
}

/*
d_assert_gt
  Asserts that the first value is GREATER THAN the second using the provided
  comparator.  If no comparator is provided, compares the raw pointer values
  using the greater-than operator.

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if _a is greater than _b
  _message_false: message to use if _a is not greater than _b

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_gt
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(>)
}

/*
d_assert_gt_eq
  Asserts that the first value is GREATER THAN OR EQUAL TO the second using
  the provided comparator.  If no comparator is provided, compares the raw
  pointer values using the greater-than-or-equal operator.

Parameter(s):
  _a:             first value being compared
  _b:             second value being compared
  _comparator:    comparator function (may be NULL for pointer comparison)
  _message_true:  message to use if _a is greater than or equal to _b
  _message_false: message to use if _a is less than _b

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_gt_eq
(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(>=)
}


/******************************************************************************
 * STRING ASSERTION FUNCTIONS
 *****************************************************************************/

/*
d_assert_str_eq
  Asserts that two strings are EQUAL, comparing up to the minimum of the two
  specified lengths.  If both strings are NULL, they are considered equal.
  If only one is NULL, they are not equal.

Parameter(s):
  _str1:          first string to compare
  _str1_length:   maximum length to consider for first string
  _str2:          second string to compare
  _str2_length:   maximum length to consider for second string
  _message_true:  message to use if strings are equal
  _message_false: message to use if strings are not equal

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_str_eq
(
    const char* _str1,
    size_t      _str1_length,
    const char* _str2,
    size_t      _str2_length,
    const char* _message_true,
    const char* _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));
    size_t           compare_length;
    bool             result;

    if (!new_assertion)
    {
        return NULL;
    }

    // handle NULL cases
    if (_str1 == NULL && _str2 == NULL)
    {
        result = true;
    }
    else if (_str1 == NULL || _str2 == NULL)
    {
        result = false;
    }
    else
    {
        // compare up to the minimum of both lengths
        compare_length = (_str1_length < _str2_length) 
                         ? _str1_length 
                         : _str2_length;
        
        // strings must match and have the same effective length to be equal
        if (_str1_length != _str2_length)
        {
            result = false;
        }
        else
        {
            result = (strncmp(_str1, _str2, compare_length) == 0);
        }
    }

    new_assertion->result  = result;
    new_assertion->message = result ? _message_true : _message_false;

    return new_assertion;
}

/*
d_assert_str_neq
  Asserts that two strings are NOT EQUAL, comparing up to the minimum of the
  two specified lengths.  If both strings are NULL, they are considered equal
  (assertion fails).  If only one is NULL, they are not equal (assertion 
  passes).

Parameter(s):
  _str1:          first string to compare
  _str1_length:   maximum length to consider for first string
  _str2:          second string to compare
  _str2_length:   maximum length to consider for second string
  _message_true:  message to use if strings are not equal
  _message_false: message to use if strings are equal

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_str_neq
(
    const char* _str1,
    size_t      _str1_length,
    const char* _str2,
    size_t      _str2_length,
    const char* _message_true,
    const char* _message_false
)
{
    struct d_assert* new_assertion = malloc(sizeof(struct d_assert));
    size_t           compare_length;
    bool             strings_equal;

    if (!new_assertion)
    {
        return NULL;
    }

    // handle NULL cases
    if (_str1 == NULL && _str2 == NULL)
    {
        strings_equal = true;
    }
    else if (_str1 == NULL || _str2 == NULL)
    {
        strings_equal = false;
    }
    else
    {
        // compare up to the minimum of both lengths
        compare_length = (_str1_length < _str2_length) 
                         ? _str1_length 
                         : _str2_length;
        
        // strings are equal only if lengths match and content matches
        if (_str1_length != _str2_length)
        {
            strings_equal = false;
        }
        else
        {
            strings_equal = (strncmp(_str1, _str2, compare_length) == 0);
        }
    }

    // for neq, we want result to be true when strings are NOT equal
    new_assertion->result  = !strings_equal;
    new_assertion->message = !strings_equal ? _message_true : _message_false;

    return new_assertion;
}


/******************************************************************************
 * NULL POINTER ASSERTION FUNCTIONS
 *****************************************************************************/

/*
d_assert_null
  Asserts that the given pointer is NULL.  The resulting d_assert has result
  equal to true if, and only if, the pointer is NULL.

Parameter(s):
  _expression:    pointer to test
  _message_true:  message to use if pointer is NULL
  _message_false: message to use if pointer is not NULL

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_null
(
    const void* _expression,
    const char* _message_true,
    const char* _message_false
)
{
    return d_assert_eq(_expression, NULL, NULL, _message_true, _message_false);
}

/*
d_assert_nonnull
  Asserts that the given pointer is not NULL.  The resulting d_assert has
  result equal to true if, and only if, the pointer is not NULL.

Parameter(s):
  _expression:    pointer to test
  _message_true:  message to use if pointer is not NULL
  _message_false: message to use if pointer is NULL

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_nonnull
(
    const void* _expression,
    const char* _message_true,
    const char* _message_false
)
{
    return d_assert_neq(_expression, NULL, NULL, _message_true, _message_false);
}


/******************************************************************************
 * ARRAY ASSERTION FUNCTIONS
 *****************************************************************************/

/*
d_assert_array_is_valid
  Tests if an array pointer is non-NULL, that the first element does not point
  to NULL, and that the given array length is greater than zero.

Parameter(s):
  _arr:           pointer to the array to validate
  _arr_length:    reported length of the array
  _message_true:  message to use if array is valid
  _message_false: message to use if array is not valid

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_array_is_valid
(
    const void** _arr,
    size_t       _arr_length,
    const char*  _message_true,
    const char*  _message_false
)
{
    return d_assert_true((_arr) && (*_arr) && (_arr_length > 0), 
                         _message_true, 
                         _message_false);
}

/*
d_assert_arrays_eq
  Asserts that two arrays are equal by comparing each element using the
  provided comparator function.  Both arrays must be non-NULL.

Parameter(s):
  _arr1:          pointer to the first array
  _arr2:          pointer to the second array
  _arr_size:      number of elements in each array
  _element_size:  size of each element in bytes
  _comparator:    function used to compare elements
  _message_true:  message to use if arrays are equal
  _message_false: message to use if arrays are not equal

Return:
  A pointer to the newly-allocated d_assert, or NULL if allocation failed.
*/
struct d_assert*
d_assert_arrays_eq
(
    const void*   _arr1,
    const void*   _arr2,
    size_t        _arr_size,
    size_t        _element_size,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
)
{
    const char* arr1_ptr;
    const char* arr2_ptr;
    const void* element1;
    const void* element2;
    size_t      i;

    // check if either array is NULL
    if ( (_arr1 == NULL) || 
         (_arr2 == NULL))
    {
        return d_assert_new(false, _message_true, _message_false);
    }
    
    // compare each element of the arrays
    arr1_ptr = (const char*)_arr1;
    arr2_ptr = (const char*)_arr2;
    
    for (i = 0; i < _arr_size; i++)
    {
        element1 = (const void*)(arr1_ptr + i * _element_size);
        element2 = (const void*)(arr2_ptr + i * _element_size);
        
        // use the comparator to check for equality
        if (_comparator(element1, element2) != 0)
        {
            return d_assert_new(false, _message_true, _message_false);
        }
    }
    
    // all elements are equal
    return d_assert_new(true, _message_true, _message_false);
}


/******************************************************************************
 * MEMORY MANAGEMENT FUNCTIONS
 *****************************************************************************/

/*
d_assert_free
  Frees the memory associated with the given d_assert.

Parameter(s):
  _assertion: the d_assert being freed (may be NULL)

Return:
  None.
*/
void 
d_assert_free
(
    struct d_assert* _assertion
)
{
    if (_assertion)
    {
        free(_assertion);
    }

    return;
}