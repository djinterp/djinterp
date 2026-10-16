/*******************************************************************************
* djinterp [test]                                   concepts_tests_invocable.cpp
*
*   Tests for concepts.hpp section II.9: invocable concepts.
* Covers: invocable_r, nothrow_invocable, nothrow_invocable_r.
*
*   Key semantic notes:
*     - invocable_r short-circuits on void: if `_Ret` is void, the
*       return-type convertibility check is skipped (so any invocable
*       expression qualifies regardless of its return type).  This is
*       documented behaviour - see concepts.hpp section II.9.
*     - nothrow_invocable / nothrow_invocable_r distinguish callables
*       declared `noexcept` from those that may throw.  The returns_int
*       fixture is NOT noexcept; returns_int_noexcept is.  These two
*       fixtures together let us exercise both branches.
*     - convertibility, not exact match: invocable_r<long, F, int> is
*       satisfied if F(int) returns something convertible to long, not
*       only if it returns long exactly.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_invocable.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_invocable(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::returns_int;
    using fixtures::returns_int_noexcept;
    using fixtures::returns_void;

    // -------------------------------------------------------------------
    // invocable_r
    // -------------------------------------------------------------------
    //   invocable_r<Ret, Fn, Args...> requires:
    //   (a) std::invocable<Fn, Args...>, AND
    //   (b) std::is_void_v<Ret>  ||  convertible_to<invoke_result, Ret>

    // -- positive: matching return type ----------------------------------
    DT_CONCEPT    ((invocable_r<int,    returns_int,         int>));
    DT_CONCEPT    ((invocable_r<int,    returns_int_noexcept, int>));
    DT_CONCEPT    ((invocable_r<void,   returns_void>));

    // -- positive: convertible return type -------------------------------
    DT_CONCEPT    ((invocable_r<long,    returns_int, int>));   // int -> long
    DT_CONCEPT    ((invocable_r<double,  returns_int, int>));   // int -> double
    DT_CONCEPT    ((invocable_r<bool,    returns_int, int>));   // int -> bool

    // -- positive: void return is unconstrained --------------------------
    //   Documented: void on the left short-circuits the convertibility
    // check.  A non-void-returning callable still satisfies the void
    // form.
    DT_CONCEPT    ((invocable_r<void,    returns_int, int>));

    // -- negative: not invocable at all ----------------------------------
    DT_NOT_CONCEPT((invocable_r<int, returns_int>));            // missing arg
    DT_NOT_CONCEPT((invocable_r<int, int, int>));               // int isn't callable

    // -- negative: invocable, but return not convertible -----------------
    //   returns_void's invoke_result is void, NOT convertible to int
    DT_NOT_CONCEPT((invocable_r<int,  returns_void>));

    // -------------------------------------------------------------------
    // nothrow_invocable
    // -------------------------------------------------------------------

    DT_CONCEPT    ((nothrow_invocable<returns_int_noexcept, int>));
    DT_NOT_CONCEPT((nothrow_invocable<returns_int,          int>));   // potentially throws
    DT_NOT_CONCEPT((nothrow_invocable<returns_int>));                 // not invocable
    DT_NOT_CONCEPT((nothrow_invocable<int>));                         // int not callable

    // -------------------------------------------------------------------
    // nothrow_invocable_r
    // -------------------------------------------------------------------
    //   The combination: nothrow + return-type convertibility.

    DT_CONCEPT    ((nothrow_invocable_r<int,    returns_int_noexcept, int>));
    DT_CONCEPT    ((nothrow_invocable_r<long,   returns_int_noexcept, int>));   // convertible
    DT_CONCEPT    ((nothrow_invocable_r<void,   returns_int_noexcept, int>));   // void short-circuit

    //   throwing version fails the nothrow predicate even if the
    // return type matches
    DT_NOT_CONCEPT((nothrow_invocable_r<int,    returns_int, int>));

    //   missing arg
    DT_NOT_CONCEPT((nothrow_invocable_r<int,    returns_int_noexcept>));

    //   non-callable
    DT_NOT_CONCEPT((nothrow_invocable_r<int,    int, int>));

    // -------------------------------------------------------------------
    // Structural invariants
    // -------------------------------------------------------------------
    //   nothrow_invocable_r<Ret, Fn, Args...> ==>
    //       invocable_r<Ret, Fn, Args...>  AND  nothrow_invocable<Fn, Args...>
    //   The outer double-parens shield the inner template-argument
    // commas from the preprocessor.  Without them the macro would see
    // multiple comma-separated args rather than one boolean expression.
    DT_CONCEPT((!(nothrow_invocable_r<int, returns_int_noexcept, int>)
                ||  (invocable_r<int, returns_int_noexcept, int>)));
    DT_CONCEPT((!(nothrow_invocable_r<int, returns_int_noexcept, int>)
                ||  (nothrow_invocable<returns_int_noexcept, int>)));

    return;
}


NS_END  // test
NS_END  // djinterp
