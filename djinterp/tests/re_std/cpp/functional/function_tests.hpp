/*******************************************************************************
* djinterp [re_std]                                           function_tests.hpp
*
* re_std::function's suite in DTest's C face (c/modules/test/test_suite.h):
* the sixteen cases of the smoke test it replaces, a test each, in one block.
*   The C face runs a C++ suite's bool (void) tests as they are, so the suite
* needs none of the C++ framework.
*
*
* path:      /tests/re_std/cpp/functional/function_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  re_std::function
2.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_RE_STD_CPP_FUNCTIONAL_FUNCTION_TESTS_HPP
#define DJINTERP_TESTS_RE_STD_CPP_FUNCTIONAL_FUNCTION_TESTS_HPP 1

// djinterp
#include "djinterp/cpp/djinterp.hpp"                    // framework root
#include "djinterp/c/modules/test/test_suite.h"         // D_TEST_*
// re_std
#ifndef DTEST_SPEC_MODE
    #include "re_std/cpp/functional/function.hpp"           // under test
    #include "re_std/cpp/functional/reference_wrapper.hpp"  // ref's result
    #include "re_std/cpp/functional/ref.hpp"                // re_std::ref
#endif  // !DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    re_std::function
//------------------------------------------------------------------------------
bool
tests_function_empty();

bool
tests_function_lambda();

bool
tests_function_pointer();

bool
tests_function_null_pointer();

bool
tests_function_copy();

bool
tests_function_move();

bool
tests_function_assign();

bool
tests_function_swap();

bool
tests_function_target();

bool
tests_function_void_return();

bool
tests_function_reference_wrapper();

bool
tests_function_stateful();

bool
tests_function_member_pointer();

bool
tests_function_convertible_return();

bool
tests_function_reference_return();

bool
tests_function_by_value_argument();


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// function_tests_rows
//   constant: the block's rows.
static const struct dj_test_case_spec function_tests_rows[] =
{
    { "tests_function_empty",
      "a default function is empty, equals nullptr, and throws when called",
      &tests_function_empty },
    { "tests_function_lambda",
      "a lambda is a target",
      &tests_function_lambda },
    { "tests_function_pointer",
      "a function pointer is a target, decayed or not",
      &tests_function_pointer },
    { "tests_function_null_pointer",
      "a null function pointer leaves the function empty",
      &tests_function_null_pointer },
    { "tests_function_copy",
      "a copy is independent of its source",
      &tests_function_copy },
    { "tests_function_move",
      "a move leaves its source empty",
      &tests_function_move },
    { "tests_function_assign",
      "assigning a lambda sets a target; assigning nullptr clears it",
      &tests_function_assign },
    { "tests_function_swap",
      "member and non-member swap exchange targets",
      &tests_function_swap },
    { "tests_function_target",
      "target<T> finds the target or gives null; target_type names it",
      &tests_function_target },
    { "tests_function_void_return",
      "a void signature calls a target and discards its result",
      &tests_function_void_return },
    { "tests_function_reference_wrapper",
      "a reference_wrapper target shares its object's state",
      &tests_function_reference_wrapper },
    { "tests_function_stateful",
      "a stateful functor is kept by value and keeps its state",
      &tests_function_stateful },
    { "tests_function_member_pointer",
      "a pointer to member function is a target",
      &tests_function_member_pointer },
    { "tests_function_convertible_return",
      "a result converts to the signature's return type",
      &tests_function_convertible_return },
    { "tests_function_reference_return",
      "a reference-returning target returns the reference",
      &tests_function_reference_return },
    { "tests_function_by_value_argument",
      "an argument taken by value passes through",
      &tests_function_by_value_argument }
};

// function_tests_blocks
//   constant: the module's one block.
static const struct dj_test_block_spec function_tests_blocks[] =
{
    { "function",
      "re_std::function, as the smoke test had it",
      function_tests_rows,
      sizeof(function_tests_rows) / sizeof(function_tests_rows[0]) }
};

// function_tests_module
//   constant: the module spec.
static const struct dj_test_module_spec function_tests_module =
{
    "function",
    "re_std/cpp/functional/function.hpp: the type-erased callable",
    function_tests_blocks,
    sizeof(function_tests_blocks) / sizeof(function_tests_blocks[0])
};

// function_tests_spec
//   function: the suite's provider.
inline const struct dj_test_module_spec*
function_tests_spec()
{
    return &function_tests_module;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_RE_STD_CPP_FUNCTIONAL_FUNCTION_TESTS_HPP
