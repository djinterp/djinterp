/******************************************************************************
* djinterp [test]                                 type_traits_tests_callable.cpp
*
*   Unit tests for the callable traits in Section I.2 of type_traits.hpp:
*     - invoke_result, invoke_result_t
*     - is_invocable, is_invocable_v
*     - is_invocable_r, is_invocable_r_v
*     - is_nothrow_invocable, is_nothrow_invocable_v
*     - is_nothrow_invocable_r, is_nothrow_invocable_r_v
*
*   The C++17+ path imports these directly from std::; the C++11/14 path
* uses djinterp's own SFINAE-based implementations.  Either way, the
* semantics must match.
*
*   Subjects (anonymous-namespace types):
*     - free function (int(int))
*     - function object with operator()(int) -> int
*     - function object with operator()(int) noexcept -> int
*     - function object with operator()(int, int) -> int
*     - non-callable type
*
*   Coverage matrix:
*   - is_invocable: callable / non-callable, arity mismatch, type mismatch
*   - is_invocable_r: convertible return, NON-convertible return, failure
*   - is_nothrow_invocable: noexcept callable / throwing callable
*   - is_nothrow_invocable_r: composes noexcept with convertibility
*
*
* path:      /inc/djinterp/test/type_traits_tests_callable.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


namespace
{

// free_fn_int_to_int -- declared (definition is unused; SFINAE only)
int  free_fn_int_to_int(int);

// callable_int_to_int
//   functor: int(int).  May throw.
struct callable_int_to_int
{
    int
    operator()(int _x) const
    {
        return _x * 2;
    }
};

// callable_int_to_int_noexcept
//   functor: int(int) noexcept.
struct callable_int_to_int_noexcept
{
    int
    operator()(int _x) const noexcept
    {
        return _x * 2;
    }
};

// callable_two_ints_to_int
//   functor: int(int, int).
struct callable_two_ints_to_int
{
    int
    operator()(int _x, int _y) const
    {
        return _x + _y;
    }
};

// non_callable_t -- no operator(), used as the negative case.
struct non_callable_t
{
    int data;
};

// to_double_t -- a class implicitly convertible from int (used by
// is_invocable_r to demonstrate "return is convertible to _Ret").
struct to_double_t
{
    double value;

    to_double_t(int _x) : value(_x) { return; }
};

// from_int_unrelated_t -- callable returning an opaque type NOT
// convertible to int. Used to verify is_invocable_r negative path.
struct opaque_return_t {};

struct callable_int_to_opaque
{
    opaque_return_t
    operator()(int _x) const
    {
        (void)_x;
        return opaque_return_t{};
    }
};

}  // namespace


// =========================================================================
// I.   invoke_result / invoke_result_t  (compile-time)
// =========================================================================

// free function
static_assert(std::is_same<typename invoke_result<decltype(&free_fn_int_to_int), int>::type,
                           int>::value,
              "invoke_result<free fn, int> == int");
static_assert(std::is_same<invoke_result_t<decltype(&free_fn_int_to_int), int>,
                           int>::value,
              "invoke_result_t alias");

// functor
static_assert(std::is_same<invoke_result_t<callable_int_to_int, int>, int>::value,
              "invoke_result_t<functor, int> == int");

// two-arg
static_assert(std::is_same<invoke_result_t<callable_two_ints_to_int, int, int>,
                           int>::value,
              "invoke_result_t<two-arg functor, int, int> == int");


// =========================================================================
// II.  is_invocable / is_invocable_v  (compile-time)
// =========================================================================

// positive: callable with the supplied arg shape
static_assert(is_invocable<callable_int_to_int, int>::value == true,
              "is_invocable<functor, int> -> true");
static_assert(is_invocable<callable_two_ints_to_int, int, int>::value == true,
              "is_invocable<two-arg functor, int, int> -> true");
static_assert(is_invocable<decltype(&free_fn_int_to_int), int>::value == true,
              "is_invocable<free fn, int> -> true");

// negative: not callable at all
static_assert(is_invocable<non_callable_t, int>::value == false,
              "is_invocable<non-callable, int> -> false");
static_assert(is_invocable<int, int>::value == false,
              "is_invocable<int, int> -> false (int is not callable)");

// negative: wrong arity
static_assert(is_invocable<callable_int_to_int, int, int>::value == false,
              "is_invocable<1-arg, int, int> -> false (arity)");
static_assert(is_invocable<callable_int_to_int>::value == false,
              "is_invocable<1-arg, /no args/> -> false (arity)");

// negative: incompatible arg type
static_assert(is_invocable<callable_int_to_int, non_callable_t>::value == false,
              "is_invocable<1-arg(int), non_callable_t> -> false (no conversion)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_invocable_v<callable_int_to_int, int> == true,
                  "is_invocable_v: matches ::value (true)");
    static_assert(is_invocable_v<non_callable_t, int> == false,
                  "is_invocable_v: matches ::value (false)");
#endif


// =========================================================================
// III. is_invocable_r / is_invocable_r_v  (compile-time)
// =========================================================================

// return type matches exactly
static_assert(is_invocable_r<int, callable_int_to_int, int>::value == true,
              "is_invocable_r<int, functor, int> -> true (exact)");

// return type convertible
static_assert(is_invocable_r<long, callable_int_to_int, int>::value == true,
              "is_invocable_r<long, functor returning int, int> -> true (int->long)");
static_assert(is_invocable_r<to_double_t, callable_int_to_int, int>::value == true,
              "is_invocable_r<to_double_t, functor returning int, int> -> true (via ctor)");

// return type NOT convertible
static_assert(is_invocable_r<int, callable_int_to_opaque, int>::value == false,
              "is_invocable_r<int, functor returning opaque, int> -> false");

// not callable at all
static_assert(is_invocable_r<int, non_callable_t, int>::value == false,
              "is_invocable_r<int, non-callable, int> -> false");

// void return is special: anything is convertible to void
static_assert(is_invocable_r<void, callable_int_to_int, int>::value == true,
              "is_invocable_r<void, functor returning int, int> -> true (anything -> void)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_invocable_r_v<int, callable_int_to_int, int> == true,
                  "is_invocable_r_v (true)");
    static_assert(is_invocable_r_v<int, callable_int_to_opaque, int> == false,
                  "is_invocable_r_v (false)");
#endif


// =========================================================================
// IV.  is_nothrow_invocable / is_nothrow_invocable_v  (compile-time)
// =========================================================================

// noexcept functor: nothrow-invocable
static_assert(is_nothrow_invocable<callable_int_to_int_noexcept, int>::value == true,
              "is_nothrow_invocable<noexcept functor, int> -> true");

// throwing functor: NOT nothrow-invocable (but IS invocable)
static_assert(is_invocable<callable_int_to_int, int>::value == true,
              "preflight: throwing functor is invocable");
static_assert(is_nothrow_invocable<callable_int_to_int, int>::value == false,
              "is_nothrow_invocable<throwing functor, int> -> false");

// non-callable -> false
static_assert(is_nothrow_invocable<non_callable_t, int>::value == false,
              "is_nothrow_invocable<non-callable, int> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_nothrow_invocable_v<callable_int_to_int_noexcept, int> == true,
                  "is_nothrow_invocable_v (true)");
    static_assert(is_nothrow_invocable_v<callable_int_to_int, int> == false,
                  "is_nothrow_invocable_v (false)");
#endif


// =========================================================================
// V.   is_nothrow_invocable_r / is_nothrow_invocable_r_v  (compile-time)
// =========================================================================

// nothrow + convertible return
static_assert(is_nothrow_invocable_r<int, callable_int_to_int_noexcept, int>::value == true,
              "is_nothrow_invocable_r<int, noexcept functor, int> -> true");
static_assert(is_nothrow_invocable_r<long, callable_int_to_int_noexcept, int>::value == true,
              "is_nothrow_invocable_r<long, noexcept functor returning int, int> -> true");

// throwing -> false
static_assert(is_nothrow_invocable_r<int, callable_int_to_int, int>::value == false,
              "is_nothrow_invocable_r<int, throwing functor, int> -> false");

// non-convertible -> false
static_assert(is_nothrow_invocable_r<non_callable_t, callable_int_to_int_noexcept, int>::value == false,
              "is_nothrow_invocable_r<incompatible return> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_nothrow_invocable_r_v<int, callable_int_to_int_noexcept, int> == true,
                  "is_nothrow_invocable_r_v (true)");
    static_assert(is_nothrow_invocable_r_v<int, callable_int_to_int, int> == false,
                  "is_nothrow_invocable_r_v (false, throws)");
#endif


// =========================================================================
// VI.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_callable(
    test_handler& _test_handler
)
{
    // ---- invoke_result ----
    record_assertion(_test_handler, 
        std::is_same<invoke_result_t<decltype(&free_fn_int_to_int), int>, int>::value,
        "invoke_result_t<free fn, int> -> int");
    record_assertion(_test_handler, 
        std::is_same<invoke_result_t<callable_int_to_int, int>, int>::value,
        "invoke_result_t<1-arg functor, int> -> int");
    record_assertion(_test_handler, 
        std::is_same<invoke_result_t<callable_two_ints_to_int, int, int>, int>::value,
        "invoke_result_t<2-arg functor, int, int> -> int");

    // ---- is_invocable ----
    record_assertion(_test_handler, 
        is_invocable<callable_int_to_int, int>::value == true,
        "is_invocable: 1-arg functor with int -> true");
    record_assertion(_test_handler, 
        is_invocable<callable_two_ints_to_int, int, int>::value == true,
        "is_invocable: 2-arg functor -> true");
    record_assertion(_test_handler, 
        is_invocable<non_callable_t, int>::value == false,
        "is_invocable: non-callable -> false");
    record_assertion(_test_handler, 
        is_invocable<int, int>::value == false,
        "is_invocable: int -> false");
    record_assertion(_test_handler, 
        is_invocable<callable_int_to_int, int, int>::value == false,
        "is_invocable: arity mismatch (too many) -> false");
    record_assertion(_test_handler, 
        is_invocable<callable_int_to_int>::value == false,
        "is_invocable: arity mismatch (too few) -> false");
    record_assertion(_test_handler, 
        is_invocable<callable_int_to_int, non_callable_t>::value == false,
        "is_invocable: incompatible arg type -> false");

    // ---- is_invocable_r ----
    record_assertion(_test_handler, 
        is_invocable_r<int, callable_int_to_int, int>::value == true,
        "is_invocable_r<int, functor returning int, int> -> true");
    record_assertion(_test_handler, 
        is_invocable_r<long, callable_int_to_int, int>::value == true,
        "is_invocable_r<long, functor returning int, int> -> true (convertible)");
    record_assertion(_test_handler, 
        is_invocable_r<to_double_t, callable_int_to_int, int>::value == true,
        "is_invocable_r<to_double_t, functor returning int, int> -> true (via ctor)");
    record_assertion(_test_handler, 
        is_invocable_r<int, callable_int_to_opaque, int>::value == false,
        "is_invocable_r<int, functor returning opaque, int> -> false");
    record_assertion(_test_handler, 
        is_invocable_r<void, callable_int_to_int, int>::value == true,
        "is_invocable_r<void, ..., int> -> true (anything convertible to void)");

    // ---- is_nothrow_invocable ----
    record_assertion(_test_handler, 
        is_nothrow_invocable<callable_int_to_int_noexcept, int>::value == true,
        "is_nothrow_invocable<noexcept functor, int> -> true");
    record_assertion(_test_handler, 
        is_nothrow_invocable<callable_int_to_int, int>::value == false,
        "is_nothrow_invocable<throwing functor, int> -> false");
    record_assertion(_test_handler, 
        is_nothrow_invocable<non_callable_t, int>::value == false,
        "is_nothrow_invocable<non-callable, int> -> false");

    // ---- is_nothrow_invocable_r ----
    record_assertion(_test_handler, 
        is_nothrow_invocable_r<int, callable_int_to_int_noexcept, int>::value == true,
        "is_nothrow_invocable_r<int, noexcept functor, int> -> true");
    record_assertion(_test_handler, 
        is_nothrow_invocable_r<int, callable_int_to_int, int>::value == false,
        "is_nothrow_invocable_r<int, throwing functor, int> -> false (throws)");
    record_assertion(_test_handler, 
        is_nothrow_invocable_r<non_callable_t,
                               callable_int_to_int_noexcept, int>::value == false,
        "is_nothrow_invocable_r: incompatible return -> false");

    return;
}


NS_END  // test
NS_END  // djinterp
