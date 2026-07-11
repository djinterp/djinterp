/******************************************************************************
* djinterp [test]                                    maybe_tests_foldable.cpp
*
* Tests for the foldable_traits<maybe<_T>> specialization defined in maybe.hpp
* section V (it sits directly beside monad_traits<maybe>, which
* maybe_tests_monad_traits.cpp already covers).  The generic foldable protocol
* -- the free fold_left / fold_right / fold_map / ... in foldable.hpp -- is the
* foldable module's own suite to exercise; here we test only maybe's
* PARTICIPATION in it:
*   - is_foldable / foldable_value_type_t recognise maybe and recover T
*   - the specialization's own surface: is_specialized, value_type, fold_left
*   - fold_left's behaviour: the reducer is applied exactly once for a present
*     value and is the identity on the accumulator for an empty maybe
*   - the generic free folds (fold_left / fold_right) delegate to it correctly
*
*   maybe supplies fold_left as its single obligation; every other fold is
* derived from it generically, so verifying fold_left (direct and through the
* free driver) is what pins maybe's end of the contract.
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime section mirrors them so the counts roll into the report.
******************************************************************************/

#include <string>
#include <type_traits>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// ---- compile-time surface of the specialization ----
static_assert(foldable_traits<maybe<int> >::is_specialized::value,
    "foldable_traits<maybe>: is_specialized is true_type");
static_assert(
    std::is_same<foldable_traits<maybe<int> >::value_type, int>::value,
    "foldable_traits<maybe>: value_type is the contained type");

// ---- maybe participates in the generic foldable protocol ----
static_assert(is_foldable<maybe<int> >::value,
    "is_foldable: maybe<int> is foldable");
static_assert(!is_foldable<int>::value,
    "is_foldable: a plain value type is not foldable");
static_assert(
    std::is_same<foldable_value_type_t<maybe<int> >, int>::value,
    "foldable_value_type_t: recovers maybe's inner type");


void test_foldable_traits(test::test_handler& _h)
{
    // ---- detection + inner-type extraction ----
    test::record_assertion(_h, is_foldable<maybe<int> >::value,
        "foldable: is_foldable is true for maybe<int>");
    test::record_assertion(_h, !is_foldable<int>::value,
        "foldable: is_foldable is false for a plain value type");
    test::record_assertion(_h,
        (std::is_same<foldable_value_type_t<maybe<int> >, int>::value),
        "foldable: foldable_value_type_t recovers the inner type");

    // ---- the specialization's own members ----
    test::record_assertion(_h,
        foldable_traits<maybe<int> >::is_specialized::value,
        "foldable: the maybe specialization is marked is_specialized");

    // direct fold_left over a present value applies the reducer exactly once
    maybe<int> v(21);
    int direct_present =
        foldable_traits<maybe<int> >::fold_left(v, 100, fn_add());
    test::record_assertion(_h, direct_present == 121,
        "foldable: direct fold_left over just applies the reducer once (100+21)");

    // direct fold_left over empty is the identity on the accumulator
    maybe<int> e;
    int direct_empty =
        foldable_traits<maybe<int> >::fold_left(e, 100, fn_add());
    test::record_assertion(_h, direct_empty == 100,
        "foldable: direct fold_left over nothing returns the initial accumulator");

    // ---- the generic free folds delegate to the specialization ----
    test::record_assertion(_h, (fold_left(v, 0, fn_add()) == 21),
        "foldable: free fold_left(just(21), 0, add) == 21");
    test::record_assertion(_h, (fold_left(e, 0, fn_add()) == 0),
        "foldable: free fold_left(nothing, 0, add) == 0 (identity)");

    // fold_right agrees for a maybe: at most one element, so association is moot
    test::record_assertion(_h, (fold_right(v, 0, fn_add()) == 21),
        "foldable: free fold_right(just(21), 0, add) == 21");
    test::record_assertion(_h, (fold_right(e, 0, fn_add()) == 0),
        "foldable: free fold_right(nothing, 0, add) == 0");

    return;
}


NS_END  // testing
NS_END  // djinterp
