/******************************************************************************
* djinterp [test]                                   result_tests_foldable.cpp
*
* Tests for the foldable_traits<result<_T, _E>> specialization defined in
* result.hpp section V (it sits directly beside monad_traits<result>, which
* result_tests_monad_traits.cpp already covers).  The generic foldable protocol
* -- the free fold_left / fold_right / ... in foldable.hpp -- is the foldable
* module's own suite to exercise; here we test only result's PARTICIPATION in
* it:
*   - is_foldable / foldable_value_type_t recognise result and recover T
*   - the specialization's own surface: is_specialized, value_type, fold_left
*   - fold_left's behaviour: the reducer is applied exactly once for an ok
*     value and is the identity on the accumulator for an err (the error is not
*     an element, consistent with map / and_then propagating err untouched)
*   - the generic free folds (fold_left / fold_right) delegate to it correctly
*
*   result folds over its success type; err is treated as empty.  result
* supplies fold_left as its single obligation; every other fold is derived from
* it generically, so verifying fold_left (direct and through the free driver)
* is what pins result's end of the contract.
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime section mirrors them so the counts roll into the report.
******************************************************************************/

#include <string>
#include <type_traits>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


// ---- compile-time surface of the specialization ----
static_assert(foldable_traits<ri>::is_specialized::value,
    "foldable_traits<result>: is_specialized is true_type");
static_assert(std::is_same<foldable_traits<ri>::value_type, int>::value,
    "foldable_traits<result>: value_type is the success type");

// ---- result participates in the generic foldable protocol ----
static_assert(is_foldable<ri>::value,
    "is_foldable: result<int,string> is foldable");
static_assert(!is_foldable<int>::value,
    "is_foldable: a plain value type is not foldable");
static_assert(std::is_same<foldable_value_type_t<ri>, int>::value,
    "foldable_value_type_t: recovers result's success type");


void test_foldable_traits(test::test_handler& _h)
{
    // ---- detection + inner-type extraction ----
    test::record_assertion(_h, is_foldable<ri>::value,
        "foldable: is_foldable is true for result<int,string>");
    test::record_assertion(_h, !is_foldable<int>::value,
        "foldable: is_foldable is false for a plain value type");
    test::record_assertion(_h,
        (std::is_same<foldable_value_type_t<ri>, int>::value),
        "foldable: foldable_value_type_t recovers the success type");

    // ---- the specialization's own members ----
    test::record_assertion(_h,
        foldable_traits<ri>::is_specialized::value,
        "foldable: the result specialization is marked is_specialized");

    // direct fold_left over an ok value applies the reducer exactly once
    ri okv = ok<int, std::string>(21);
    int direct_ok =
        foldable_traits<ri>::fold_left(okv, 100, fn_add());
    test::record_assertion(_h, direct_ok == 121,
        "foldable: direct fold_left over ok applies the reducer once (100+21)");

    // direct fold_left over err is the identity on the accumulator
    ri errv = err<int, std::string>("e");
    int direct_err =
        foldable_traits<ri>::fold_left(errv, 100, fn_add());
    test::record_assertion(_h, direct_err == 100,
        "foldable: direct fold_left over err returns the initial accumulator");

    // ---- the generic free folds delegate to the specialization ----
    test::record_assertion(_h, (fold_left(okv, 0, fn_add()) == 21),
        "foldable: free fold_left(ok(21), 0, add) == 21");
    test::record_assertion(_h, (fold_left(errv, 0, fn_add()) == 0),
        "foldable: free fold_left(err, 0, add) == 0 (identity)");

    // fold_right agrees for a result: at most one element, so association is moot
    test::record_assertion(_h, (fold_right(okv, 0, fn_add()) == 21),
        "foldable: free fold_right(ok(21), 0, add) == 21");
    test::record_assertion(_h, (fold_right(errv, 0, fn_add()) == 0),
        "foldable: free fold_right(err, 0, add) == 0");

    return;
}


NS_END  // testing
NS_END  // djinterp
