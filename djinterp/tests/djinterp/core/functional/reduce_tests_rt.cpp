// djinterp [test]  reduce_tests_rt.cpp
//   I. reduce_rt -- the runtime driver: a left fold over an iterator range, and
//   the iterable convenience form over anything with begin()/end().

// std
#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "reduce_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_rt_range_form
  The iterator-range form folds [_first, _last).
  Tests the following:
  - a vector's iterators, and raw pointers into a C array
  - the seed participates as the leftmost accumulator
*/
bool
tests_rt_range_form()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, xs.begin(), xs.end()) == 6);
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 10, xs.begin(), xs.end()) == 16);

    // raw pointers are iterators too.
    const int arr[3] = { 4, 5, 6 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, arr, arr + 3) == 15);

    // a sub-range folds only that sub-range.
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, arr, arr + 2) == 9);
#endif

    return ok;
}


/*
tests_rt_iterable_form
  The convenience form takes anything with begin()/end() and wraps the range form.
  Tests the following:
  - a std::vector, a std::array, and a raw C array all fold
*/
bool
tests_rt_iterable_form()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, xs) == 6);

    const std::array<int, 3> as = { { 4, 5, 6 } };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, as) == 15);

    // a C array: std::begin / std::end reach it.
    const int carr[4] = { 1, 2, 3, 4 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, carr) == 10);
#endif

    return ok;
}


/*
tests_rt_both_forms_agree
  The iterable form is documented as a wrapper around the range form, so the two
  must give the same answer for the same data and reducer.
  Tests the following:
  - the two agree across several containers and reducers
  - they agree in return type as well as value
*/
bool
tests_rt_both_forms_agree()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, xs) ==
                reduce_rt(sum_with<rt_leaf>(), 0, xs.begin(), xs.end()));
    ok = ok && (reduce_rt(digits_rt(), 0, xs) ==
                reduce_rt(digits_rt(), 0, xs.begin(), xs.end()));
    ok = ok && (reduce_rt(count_all(), 0, xs) ==
                reduce_rt(count_all(), 0, xs.begin(), xs.end()));

    static_assert(std::is_same<
        decltype(reduce_rt(digits_rt(), 0, std::declval<const std::vector<int>&>())),
        decltype(reduce_rt(digits_rt(), 0, std::declval<std::vector<int>::const_iterator>(),
                                           std::declval<std::vector<int>::const_iterator>()))
        >::value, "the same result type");
#endif

    return ok;
}


/*
tests_rt_empty_returns_the_seed
  An empty range folds to the accumulator, untouched -- the reducer is never
  invoked.
  Tests the following:
  - an empty vector, an empty range, and an empty C-array sub-range
  - the reducer really is not called
*/
bool
tests_rt_empty_returns_the_seed()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const std::vector<int> none;
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 42, none) == 42);
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 42, none.begin(), none.end()) == 42);

    // a degenerate sub-range of a non-empty array.
    const int arr[3] = { 1, 2, 3 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 7, arr, arr) == 7);

    // and the reducer is never called.
    int          calls = 0;
    counting_any counter{ &calls };
    ok = ok && (reduce_rt(counter, 0, none) == 0);
    ok = ok && (calls == 0);
#endif

    return ok;
}


/*
tests_rt_is_a_left_fold
  The driver walks left-to-right, folding each element into the accumulator in
  turn. A non-commutative reducer -- (acc * 10) + x -- pins that order: 1, 2, 3
  from a seed of 0 gives 123 under a left fold and could not under any other.
  Tests the following:
  - the digits reducer yields 123 and 1234
  - the seed is the leftmost accumulator
  - it is emphatically not a right fold
*/
bool
tests_rt_is_a_left_fold()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const int arr[3] = { 1, 2, 3 };
    ok = ok && (reduce_rt(digits_rt(), 0, arr) == 123);
    ok = ok && (reduce_rt(digits_rt(), 0, arr) != 321);

    const int arr4[4] = { 1, 2, 3, 4 };
    ok = ok && (reduce_rt(digits_rt(), 0, arr4) == 1234);

    // the seed sits at the far left.
    const int arr2[2] = { 2, 3 };
    ok = ok && (reduce_rt(digits_rt(), 1, arr2) == 123);
#endif

    return ok;
}


/*
tests_rt_single_element
  The one-element case: exactly one turn of the loop.
  Tests the following:
  - the reducer is applied once, to the seed and the sole element
*/
bool
tests_rt_single_element()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const int one[1] = { 5 };

    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, one) == 5);
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 10, one) == 15);
    ok = ok && (reduce_rt(count_all(), 0, one) == 1);
    ok = ok && (reduce_rt(digits_rt(), 0, one) == 5);
#endif

    return ok;
}


/*
tests_rt_constexpr_over_a_constexpr_range
  The header's claim: reduce_rt is constexpr, "so it also folds at compile time
  over a constexpr range."
  Tests the following:
  - it folds inside static_assert over a constexpr C array and a constexpr
    std::array, in both the iterable and the range forms
  - the left-fold order holds at compile time too
  - and the same call still runs at runtime
*/
bool
tests_rt_constexpr_over_a_constexpr_range()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    static_assert(reduce_rt(sum_with<rt_leaf>(), 0, carr) == 6,
                  "constexpr C array, iterable form");
    static_assert(reduce_rt(sum_with<rt_leaf>(), 0, carr, carr + 3) == 6,
                  "constexpr C array, range form");
    static_assert(reduce_rt(digits_rt(), 0, carr) == 123,
                  "left fold, at compile time");
    static_assert(reduce_rt(count_all(), 0, carr) == 3, "count, at compile time");

    constexpr std::array<int, 3> as = { { 4, 5, 6 } };
    static_assert(reduce_rt(sum_with<rt_leaf>(), 0, as) == 15,
                  "constexpr std::array");

    // the seed participates at compile time too.
    static_assert(reduce_rt(sum_with<rt_leaf>(), 100, carr) == 106, "seeded");

    // ...and the same call runs at runtime.
    bool ok = true;
    int  n  = 1;
    const int runtime_arr[3] = { n, n + 1, n + 2 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, runtime_arr) == 6);
    return ok;
#else
    return true;
#endif
}


/*
tests_rt_accumulator_type_is_fixed
  reduce_rt assigns the reducer's result back into its accumulator and returns
  _Acc, so the accumulator TYPE IS FIXED across the fold -- it is the seed's type,
  whatever the reducer returns. (This is the sharp difference from reduce_ct,
  which recomputes the accumulator type at every step.)
  Tests the following:
  - the result type is the seed's type, for several seeds
  - a reducer that returns a WIDER type still yields the seed's type
  - the value is right nonetheless
*/
bool
tests_rt_accumulator_type_is_fixed()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // the result type is the SEED's type.
    static_assert(std::is_same<decltype(reduce_rt(sum_with<rt_leaf>(), 0, carr)),
                               int>::value, "int seed -> int result");
    static_assert(std::is_same<decltype(reduce_rt(count_all(), 0, carr)),
                               int>::value, "int seed -> int result");

    // widen returns a LONG at every step -- and the fold still yields an int.
    static_assert(std::is_same<decltype(widen()(0, 0)), long>::value,
                  "the reducer really does return long");
    static_assert(std::is_same<decltype(reduce_rt(widen(), 0, carr)), int>::value,
                  "the accumulator type is FIXED at the seed's");
    static_assert(reduce_rt(widen(), 0, carr) == 6, "and the value is right");

    ok = ok && (reduce_rt(widen(), 0, carr) == 6);
#endif

    return ok;
}


/*
tests_rt_pulls_a_lazy_generated_source
  "Lazy/large/infinite sources are fine: it pulls until the range ends." The
  driver needs nothing of a source but *, ++ and != -- there need not be a
  container behind it at all.
  Tests the following:
  - a GENERATED range (values computed on dereference, no storage) folds correctly
  - it folds at compile time as well
  - an empty generated range returns the seed
*/
bool
tests_rt_pulls_a_lazy_generated_source()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // [1, 4) generated on the fly: 1 + 2 + 3.
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0,
                          count_iter{ 1 }, count_iter{ 4 }) == 6);

    // it is a real left fold over the generated values.
    ok = ok && (reduce_rt(digits_rt(), 0,
                          count_iter{ 1 }, count_iter{ 4 }) == 123);

    // a longer pull, with no storage anywhere.
    ok = ok && (reduce_rt(count_all(), 0,
                          count_iter{ 0 }, count_iter{ 100 }) == 100);

    // at compile time, too.
    static_assert(reduce_rt(sum_with<rt_leaf>(), 0,
                            count_iter{ 1 }, count_iter{ 4 }) == 6,
                  "a lazy source folds constexpr");

    // an empty generated range.
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 9,
                          count_iter{ 5 }, count_iter{ 5 }) == 9);
#endif

    return ok;
}


/*
tests_rt_finds_begin_end_by_adl
  The iterable form says `using std::begin; using std::end;` and then calls them
  unqualified, so a range that exposes only FREE begin/end -- no members at all --
  is still admitted, found by argument-dependent lookup.
  Tests the following:
  - a range with no member begin()/end() folds correctly
  - the same range has no members (so ADL is the only way it could have worked)
*/
bool
tests_rt_finds_begin_end_by_adl()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static const int    data[3] = { 1, 2, 3 };
    const     adl_range range{ data, 3 };

    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, range) == 6);
    ok = ok && (reduce_rt(digits_rt(), 0, range) == 123);
    ok = ok && (reduce_rt(count_all(), 0, range) == 3);

    // it really has no members -- only the free functions could have been found.
    static_assert(std::is_same<decltype(begin(range)), const int*>::value,
                  "found by ADL");

    // an empty ADL range returns the seed.
    const adl_range none{ data, 0 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 7, none) == 7);
#endif

    return ok;
}


/*
tests_rt_visits_each_element_once
  The loop pulls each element exactly once, in order -- no element is skipped and
  none is folded twice.
  Tests the following:
  - the reducer is called exactly as many times as there are elements
  - the count is zero for an empty range, and matches for several lengths
*/
bool
tests_rt_visits_each_element_once()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const int arr[4] = { 1, 2, 3, 4 };

    int          calls = 0;
    counting_any counter{ &calls };

    reduce_rt(counter, 0, arr);
    ok = ok && (calls == 4);

    // a sub-range visits only its own elements.
    calls = 0;
    reduce_rt(counter, 0, arr, arr + 2);
    ok = ok && (calls == 2);

    // and the count the reducer itself computes agrees.
    ok = ok && (reduce_rt(count_all(), 0, arr) == 4);
#endif

    return ok;
}


/*
tests_rt_does_not_consume_its_source
  The iterable is taken by const reference and only walked, so the source survives
  the fold unchanged and may be folded again.
  Tests the following:
  - a vector is unchanged after being reduced
  - folding it twice gives the same answer both times
*/
bool
tests_rt_does_not_consume_its_source()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::vector<int> xs;
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    const int first = reduce_rt(sum_with<rt_leaf>(), 0, xs);
    ok = ok && (first == 6);

    // the source is untouched...
    ok = ok && (xs.size() == 3u);
    ok = ok && (xs[0] == 1 && xs[1] == 2 && xs[2] == 3);

    // ...so folding it again gives the same answer.
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, xs) == first);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
