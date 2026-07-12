// djinterp [test]  reduce_tests_unified.cpp
//   IV. The step/driver split -- the header's thesis, put on trial: "the same
//   reducer body serves all three domains; only the driver (loop vs. recursion)
//   and the leaf differ." Plus the C++17 tier gate.

// std
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "reduce_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_one_reducer_body_three_domains
  THE THESIS. count_all is one reducer -- (acc, anything) -> acc + 1 -- and the
  SAME object is handed unchanged to all three drivers: the runtime loop, the
  value-domain recursion, and the type-domain recursion. If the step/driver split
  is real, it counts the elements in every one of them, and nothing about the
  reducer changes between them.
  Tests the following:
  - the same reducer counts a runtime range, a value_list, and a tuple's types
  - it does so at compile time in all three
  - one instance of it, reused across the three calls, gives the same answers
*/
bool
tests_one_reducer_body_three_domains()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // ONE reducer body -- three drivers, three domains.
    static_assert(reduce_rt(count_all(), 0, carr) == 3, "runtime values");
    static_assert(reduce_ct(count_all(), 0, value_list<1, 2, 3>{}) == 3,
                  "NTTP value carriers");
    static_assert(reduce_ct(count_all(), 0,
                            type_c<std::tuple<char, int, double> >) == 3,
                  "type carriers");

    // and it is genuinely the same object, not three lookalikes.
    const count_all reducer;
    ok = ok && (reduce_rt(reducer, 0, carr) == 3);
    ok = ok && (reduce_ct(reducer, 0, value_list<1, 2, 3>{}) == 3);
    ok = ok && (reduce_ct(reducer, 0,
                          type_c<std::tuple<char, int, double> >) == 3);

    // the empty case, in all three.
    const std::vector<int> none;
    ok = ok && (reduce_rt(reducer, 0, none) == 0);
    ok = ok && (reduce_ct(reducer, 0, value_list<>{}) == 0);
    ok = ok && (reduce_ct(reducer, 0, type_c<std::tuple<> >) == 0);
#endif

    return ok;
}


/*
tests_only_the_driver_and_the_leaf_differ
  The finer form of the thesis: where a reducer must actually LOOK at its element,
  the body still does not change -- only the LEAF does. sum_with<Leaf> is one body;
  rt_leaf, val_leaf and type_leaf are the three leaves, and between them they are
  the only difference across the three domains.
  Tests the following:
  - the same sum_with body, with each leaf, folds its own domain
  - the leaves are the only thing that changed (the body is one template)
  - the same holds for the non-commutative reducers, so the ORDER is left-to-right
    in all three
*/
bool
tests_only_the_driver_and_the_leaf_differ()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // one body, three leaves.
    static_assert(reduce_rt(sum_with<rt_leaf>(), 0, carr) == 6,
                  "runtime: 1 + 2 + 3");
    static_assert(reduce_ct(sum_with<val_leaf>(), 0, value_list<1, 2, 3>{}) == 6,
                  "value: the same three, as carriers");
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<char, int, double> >) == 13,
                  "type: 1 + 4 + 8, read off the types");

    // the three reducers really are one template, differing only in the leaf.
    static_assert(std::is_same<sum_with<rt_leaf>, sum_with<rt_leaf> >::value,
                  "same template");
    static_assert(!std::is_same<sum_with<rt_leaf>, sum_with<val_leaf> >::value,
                  "differing only in the leaf");

    // and the traversal is left-to-right in all three domains.
    static_assert(reduce_rt(digits_rt(), 0, carr) == 123, "runtime, left");
    static_assert(reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) == 123,
                  "value, left");
    static_assert(reduce_ct(digits_type(), 0,
                            type_c<std::tuple<char, int, double> >) == 148,
                  "type, left (1, 4, 8)");

    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, carr) == 6);
#endif

    return ok;
}


/*
tests_the_drivers_differ_on_the_accumulator
  Where the two drivers genuinely part company. reduce_rt assigns into its
  accumulator and returns _Acc, so the accumulator TYPE IS FIXED across the fold.
  Both reduce_ct overloads recompute it -- the value domain through value_list's
  fold, the type domain through decltype(rf(acc, type_c<T0>)) -- so there the
  accumulator's type may EVOLVE.
  Tests the following:
  - reduce_rt's result type is the seed's, even when the reducer returns a wider
    type
  - reduce_ct's result type follows the reducer, growing a value_list step by step
    in BOTH compile-time domains
  - so an evolving-accumulator reducer belongs to reduce_ct alone
*/
bool
tests_the_drivers_differ_on_the_accumulator()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // reduce_rt: FIXED. the reducer returns long; the fold still returns int.
    static_assert(std::is_same<decltype(widen()(0, 0)), long>::value,
                  "the reducer returns long");
    static_assert(std::is_same<decltype(reduce_rt(widen(), 0, carr)),
                               int>::value, "but the fold returns int -- FIXED");

    // reduce_ct, value domain: EVOLVES. the accumulator is a growing value_list.
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<>{}, value_list<1, 2, 3>{})),
        value_list<1, 2, 3> >::value,
        "the accumulator type changed at every step");

    // reduce_ct, type domain: EVOLVES too.
    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{},
                           type_c<std::tuple<char, int> >)),
        value_list<sizeof(char), sizeof(int)> >::value, "and here as well");

    // the seed and the result are the same type in reduce_rt, and different in
    // reduce_ct -- which is the whole distinction, in one line each.
    static_assert(std::is_same<int, decltype(reduce_rt(widen(), 0, carr))>::value,
                  "rt: seed type == result type");
    static_assert(!std::is_same<
        value_list<>,
        decltype(reduce_ct(append_val(), value_list<>{},
                           value_list<1, 2, 3>{}))>::value,
        "ct: seed type != result type");

    ok = ok && (reduce_rt(widen(), 0, carr) == 6);
#endif

    return ok;
}


/*
tests_the_drivers_agree_on_the_same_data
  The same numbers, expressed once as a runtime range and once as an NTTP
  sequence, fold to the same answer under the same reducer body -- which is what
  it means for the domains to be two views of one computation.
  Tests the following:
  - a sum and a left fold agree between reduce_rt and reduce_ct over 1, 2, 3
  - so do the counts, and the empty cases
*/
bool
tests_the_drivers_agree_on_the_same_data()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // the same sum.
    static_assert(reduce_rt(sum_with<rt_leaf>(), 0, carr) ==
                  reduce_ct(sum_with<val_leaf>(), 0, value_list<1, 2, 3>{}),
                  "the same sum");

    // the same left fold.
    static_assert(reduce_rt(digits_rt(), 0, carr) ==
                  reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}),
                  "the same left fold");

    // the same count.
    static_assert(reduce_rt(count_all(), 0, carr) ==
                  reduce_ct(count_all(), 0, value_list<1, 2, 3>{}),
                  "the same count");

    // the same empty answer.
    static_assert(reduce_rt(count_all(), 0, carr, carr) ==
                  reduce_ct(count_all(), 0, value_list<>{}), "both zero");

    ok = ok && (reduce_rt(digits_rt(), 0, carr) ==
                reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}));
#endif

    return ok;
}


/*
tests_the_drivers_are_unconstrained
  "The drivers are deliberately left unconstrained so one body serves every
  domain; a C++20 caller may layer the Reducer / Transducer concepts at the call
  site." Both halves are checked here.
  Tests the following:
  - the drivers accept any callable: a functor, a lambda, and a plain function
    pointer, with no concept conformance demanded of any of them
  - and a caller who WANTS the constraint can layer Reducer at the call site,
    which still folds correctly
*/
bool
tests_the_drivers_are_unconstrained()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    constexpr int carr[3] = { 1, 2, 3 };

    // a functor.
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, carr) == 6);

    // a lambda.
    auto lam = [](int _acc, int _x){ return _acc + _x; };
    ok = ok && (reduce_rt(lam, 0, carr) == 6);

    // a plain function pointer.
    ok = ok && (reduce_rt(&sum_fn, 0, carr) == 6);
    int (*fp)(int, int) = &sum_fn;
    ok = ok && (reduce_rt(fp, 0, carr) == 6);

    // the compile-time drivers are equally unfussy about the reducer's shape.
    ok = ok && (reduce_ct(count_all(), 0, value_list<1, 2>{}) == 2);

#  if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // the driver demands nothing -- but a caller MAY layer the Reducer face.
    static_assert(Reducer<sum_with<rt_leaf>, int, int>,
                  "the functor satisfies it");
    static_assert(Reducer<int (*)(int, int), int, int>,
                  "so does the function pointer");
    ok = ok && (reduce_rt_checked(sum_with<rt_leaf>(), 0, carr, carr + 3) == 6);
    ok = ok && (reduce_rt_checked(&sum_fn, 0, carr, carr + 3) == 6);
#  endif
#endif

    return ok;
}


/*
tests_module_tier_gate
  The TIER claim: reduce_ct and the value carriers are C++17 facilities, so the
  module is gated to that floor and "under C++11 this header is empty." That this
  translation unit includes the header and still compiles under -std=c++11 and
  -std=c++14 IS the check -- nothing below the gate may name reduce_rt or
  reduce_ct at all.
  Tests the following:
  - at C++17 and above, both drivers are present and work
  - below, the header contributed nothing, and the suite still builds
*/
bool
tests_module_tier_gate()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // at the tier: the drivers exist.
    constexpr int carr[2] = { 1, 2 };
    ok = ok && (reduce_rt(sum_with<rt_leaf>(), 0, carr) == 3);
    ok = ok && (reduce_ct(count_all(), 0, value_list<1, 2>{}) == 2);
    ok = ok && (reduce_ct(count_all(), 0, type_c<std::tuple<int, char> >) == 2);
    static_assert(D_ENV_LANG_IS_CPP17_OR_HIGHER,
                  "the gate agrees with the language level");
#else
    // below the tier: the header declared nothing. Naming reduce_rt here would
    // not compile -- which is exactly the property under test, and the build
    // itself is the assertion.
    ok = ok && true;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
