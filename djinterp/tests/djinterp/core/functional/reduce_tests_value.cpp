// djinterp [test]  reduce_tests_value.cpp
//   II. reduce_ct over a value_list -- the VALUE domain. The unified entry point;
//   the recursion itself lives in value_list's own fold.

// std
#include <type_traits>
// djinterp
#include "reduce_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_ct_value_list_folds
  The compile-time driver folds an NTTP sequence, feeding each element to the
  reducer as a val_t carrier.
  Tests the following:
  - a sum over a short and a longer list
  - the seed participates as the leftmost accumulator
  - the count reducer gives the length
*/
bool
tests_ct_value_list_folds()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(sum_with<val_leaf>(), 0, value_list<1, 2, 3>{}) == 6,
                  "1 + 2 + 3");
    static_assert(reduce_ct(sum_with<val_leaf>(), 0,
                            value_list<1, 2, 3, 4, 5>{}) == 15, "longer");
    static_assert(reduce_ct(sum_with<val_leaf>(), 10,
                            value_list<1, 2, 3>{}) == 16, "the seed counts");

    static_assert(reduce_ct(count_all(), 0, value_list<9, 9, 9, 9>{}) == 4,
                  "counted");

    ok = ok && (reduce_ct(sum_with<val_leaf>(), 0, value_list<1, 2, 3>{}) == 6);
#endif

    return ok;
}


/*
tests_ct_value_list_empty_returns_the_seed
  The empty list folds to the accumulator, untouched.
  Tests the following:
  - the seed comes back, by value and by type
  - it holds for a carrier-shaped accumulator too
*/
bool
tests_ct_value_list_empty_returns_the_seed()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(sum_with<val_leaf>(), 42, value_list<>{}) == 42,
                  "the seed, unchanged");
    static_assert(std::is_same<
        decltype(reduce_ct(sum_with<val_leaf>(), 42, value_list<>{})),
        int>::value, "and its type");

    // a list-shaped accumulator comes back untouched too.
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<7>{}, value_list<>{})),
        value_list<7> >::value, "list seed");

    ok = ok && (reduce_ct(count_all(), 0, value_list<>{}) == 0);
#endif

    return ok;
}


/*
tests_ct_value_list_is_a_left_fold
  The elements arrive first-to-last, which the non-commutative reducer pins:
  (acc * 10) + v over 1, 2, 3 from a seed of 0 gives 123 under a left fold and
  could not under any other.
  Tests the following:
  - the digits reducer yields 123 and 1234
  - the seed sits at the far left
  - it is not a right fold
*/
bool
tests_ct_value_list_is_a_left_fold()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) == 123,
                  "((0*10 + 1)*10 + 2)*10 + 3");
    static_assert(reduce_ct(digits_val(), 0, value_list<1, 2, 3, 4>{}) == 1234,
                  "four digits");
    static_assert(reduce_ct(digits_val(), 1, value_list<2, 3>{}) == 123,
                  "seed 1, then 2, then 3");
    static_assert(reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) != 321,
                  "not a right fold");

    ok = ok && (reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) == 123);
#endif

    return ok;
}


/*
tests_ct_value_list_delegates_to_fold
  The value-domain recursion lives in value_list's own fold; reduce_ct is purely
  the unified entry point. Note the argument order is NOT the same -- reduce_ct
  takes (rf, acc, list) and fold takes (list, acc, rf) -- so the delegation has to
  put them back in the right places.
  Tests the following:
  - reduce_ct(rf, acc, list) equals fold(list, acc, rf), in value and in TYPE
  - it holds for a plain accumulator and for an evolving one
*/
bool
tests_ct_value_list_delegates_to_fold()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // the same answer, with the arguments in each function's own order.
    static_assert(reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) ==
                  fold(value_list<1, 2, 3>{}, 0, digits_val()),
                  "delegates to fold");
    static_assert(reduce_ct(sum_with<val_leaf>(), 5, value_list<1, 2>{}) ==
                  fold(value_list<1, 2>{}, 5, sum_with<val_leaf>()),
                  "and with a seed");

    // and the same TYPE, including where the accumulator evolves.
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<>{}, value_list<1, 2, 3>{})),
        decltype(fold(value_list<1, 2, 3>{}, value_list<>{}, append_val()))
        >::value, "the same result type");

    ok = ok && (reduce_ct(digits_val(), 0, value_list<1, 2, 3>{}) ==
                fold(value_list<1, 2, 3>{}, 0, digits_val()));
#endif

    return ok;
}


/*
tests_ct_value_list_accumulator_may_evolve
  Unlike reduce_rt, the compile-time driver does not fix the accumulator type: the
  reducer's result becomes the next accumulator, so its TYPE may change at every
  step.
  Tests the following:
  - folding with append grows a value_list -- a different type at each step --
    ending as the copied list
  - folding with prepend REVERSES it, which pins the traversal order a second way
  - the empty case still yields the seed's type
*/
bool
tests_ct_value_list_accumulator_may_evolve()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // the accumulator's TYPE grows at every step.
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<>{}, value_list<1, 2, 3>{})),
        value_list<1, 2, 3> >::value, "append rebuilds the list");

    // prepend reverses it -- so the elements really did arrive left-to-right.
    static_assert(std::is_same<
        decltype(reduce_ct(prepend_val(), value_list<>{}, value_list<1, 2, 3>{})),
        value_list<3, 2, 1> >::value, "prepend REVERSES");

    // a seeded prefix.
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<0>{}, value_list<1, 2>{})),
        value_list<0, 1, 2> >::value, "seeded");

    ok = ok && (decltype(reduce_ct(append_val(), value_list<>{},
                                   value_list<1, 2, 3>{}))::size() == 3u);
#endif

    return ok;
}


/*
tests_ct_value_list_heterogeneous
  The pack is auto..., so the elements need not share a type; the reducer sees each
  one as a val_t over its own type.
  Tests the following:
  - a mixed int / char / bool list folds
  - the count is right, and the leaf sees each value at its own type
*/
bool
tests_ct_value_list_heterogeneous()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using mixed = value_list<1, '\x02', true>;      // 1, 2, 1

    static_assert(reduce_ct(count_all(), 0, mixed{}) == 3, "three elements");
    static_assert(reduce_ct(sum_with<val_leaf>(), 0, mixed{}) == 4,
                  "1 + 2 + 1");

    // the carried types survive into the fold.
    static_assert(std::is_same<decltype(value_list_at_v<1, mixed>),
                               const char>::value, "still a char");

    ok = ok && (reduce_ct(count_all(), 0, mixed{}) == 3);
#endif

    return ok;
}


/*
tests_ct_value_list_single_element
  The one-element case: exactly one turn of the recursion before the base case.
  Tests the following:
  - the reducer is applied once, to the seed and the sole element
*/
bool
tests_ct_value_list_single_element()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(sum_with<val_leaf>(), 0, value_list<5>{}) == 5,
                  "0 + 5");
    static_assert(reduce_ct(sum_with<val_leaf>(), 10, value_list<5>{}) == 15,
                  "10 + 5");
    static_assert(reduce_ct(count_all(), 0, value_list<5>{}) == 1, "one step");
    static_assert(std::is_same<
        decltype(reduce_ct(append_val(), value_list<>{}, value_list<5>{})),
        value_list<5> >::value, "a singleton rebuilds to itself");

    ok = ok && (reduce_ct(count_all(), 0, value_list<5>{}) == 1);
#endif

    return ok;
}


/*
tests_ct_value_list_visits_each_element_once
  The recursion peels exactly one head per step -- no element is skipped and none
  is folded twice. (reduce_ct is constexpr but perfectly runnable at runtime, so a
  recording reducer can observe the visits.)
  Tests the following:
  - the reducer is called once per element, for several lengths
  - an empty list calls it not at all
*/
bool
tests_ct_value_list_visits_each_element_once()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    int          calls = 0;
    counting_any counter{ &calls };

    reduce_ct(counter, 0, value_list<1, 2, 3>{});
    ok = ok && (calls == 3);

    calls = 0;
    reduce_ct(counter, 0, value_list<0, 1, 2, 3, 4, 5, 6, 7>{});
    ok = ok && (calls == 8);

    calls = 0;
    reduce_ct(counter, 0, value_list<>{});
    ok = ok && (calls == 0);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
