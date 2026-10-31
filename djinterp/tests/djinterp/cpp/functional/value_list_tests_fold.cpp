/*******************************************************************************
* djinterp [cpp]                                       value_list_tests_fold.cpp
*
*
* path:      /tests/djinterp/cpp/functional/value_list_tests_fold.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  value_list_tests_fold.cpp
//   Section VI -- fold: the left fold, and the value-domain driver reduce_ct
//   builds on.

// std
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_fold_reduces
  fold threads a binary carrier-callable reducer over the list, from a seed.
  Tests the following:
  - a sum over a short and a longer list
  - the result is itself a carrier, so it stays in the pipeline
  - a counting reducer, which ignores the element, gives the length
*/
bool tests_fold_reduces()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(fold(DValueList<1, 2, 3>{}, val<0>, DSumOp{}).value == 6,
                  "1 + 2 + 3");
    static_assert(fold(DValueList<1, 2, 3, 4, 5>{}, val<0>, DSumOp{}).value == 15,
                  "longer");
    static_assert(fold(DValueList<1, 2, 3>{}, val<10>, DSumOp{}).value == 16,
                  "the seed participates");

    // the result is a carrier.
    static_assert(std::is_same<decltype(fold(DValueList<1, 2, 3>{}, val<0>,
                                             DSumOp{})),
                               val_t<6> >::value, "yields val_t<6>");

    // a reducer that ignores its element counts instead.
    static_assert(fold(DValueList<9, 9, 9, 9>{}, val<0>, DCountOp{}).value == 4,
                  "counted");

    ok = ok && (fold(DValueList<1, 2, 3>{}, val<0>, DSumOp{}).value == 6);
#endif

    return ok;
}


/*
tests_fold_empty_returns_seed
  The base case: an empty list folds to the accumulator, untouched -- the reducer
  is never invoked.
  Tests the following:
  - the seed comes back unchanged, by value and by type
  - it holds for a carrier seed and for a DValueList seed
*/
bool tests_fold_empty_returns_seed()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(fold(DValueList<>{}, val<42>, DSumOp{}).value == 42,
                  "the seed, unchanged");
    static_assert(std::is_same<decltype(fold(DValueList<>{}, val<42>,
                                             DSumOp{})),
                               val_t<42> >::value, "and its type");

    // a list-shaped seed comes back untouched too.
    static_assert(std::is_same<decltype(fold(DValueList<>{}, DValueList<7>{},
                                             DAppendOp{})),
                               DValueList<7> >::value, "list seed");

    ok = ok && (fold(DValueList<>{}, val<42>, DSumOp{}).value == 42);
#endif

    return ok;
}


/*
tests_fold_is_left_associative
  fold is a LEFT fold: it folds the head into the accumulator and recurses on the
  tail, so the elements arrive first-to-last. A non-commutative reducer --
  (acc * 10) + v -- pins that order: 1, 2, 3 from a seed of 0 gives 123 under a
  left fold and could not under any other.
  Tests the following:
  - the digits reducer yields 123, 1234, and a single digit
  - a seed contributes at the far left, as the first accumulator
*/
bool tests_fold_is_left_associative()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(fold(DValueList<1, 2, 3>{}, val<0>, DDigitsOp{}).value == 123,
                  "((0*10 + 1)*10 + 2)*10 + 3");
    static_assert(fold(DValueList<1, 2, 3, 4>{}, val<0>, DDigitsOp{}).value
                  == 1234, "four digits");
    static_assert(fold(DValueList<7>{}, val<0>, DDigitsOp{}).value == 7,
                  "one digit");

    // the seed is the leftmost accumulator.
    static_assert(fold(DValueList<2, 3>{}, val<1>, DDigitsOp{}).value == 123,
                  "seed 1, then 2, then 3");

    // a right fold would have produced 321-ish; it does not.
    static_assert(fold(DValueList<1, 2, 3>{}, val<0>, DDigitsOp{}).value != 321,
                  "not a right fold");

    ok = ok && (fold(DValueList<1, 2, 3>{}, val<0>, DDigitsOp{}).value == 123);
#endif

    return ok;
}


/*
tests_fold_single_element
  The one-element case exercises exactly one turn of the recursion before the base
  case.
  Tests the following:
  - the reducer is applied once, to the seed and the sole element
*/
bool tests_fold_single_element()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(fold(DValueList<5>{}, val<0>, DSumOp{}).value == 5, "0 + 5");
    static_assert(fold(DValueList<5>{}, val<10>, DSumOp{}).value == 15, "10 + 5");
    static_assert(fold(DValueList<5>{}, val<0>, DCountOp{}).value == 1,
                  "one step");

    ok = ok && (fold(DValueList<5>{}, val<0>, DSumOp{}).value == 5);
#endif

    return ok;
}


/*
tests_fold_accumulator_may_be_a_list
  The seed is "any object" -- typically a carrier, but a DValueList serves just as
  well. Folding with append rebuilds the list element by element, so fold + append
  is the identity.
  Tests the following:
  - the accumulator's TYPE grows at every step, ending as the copied list
  - the empty list rebuilds to the empty list
*/
bool tests_fold_accumulator_may_be_a_list()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // append each element in turn: the list is rebuilt as it was.
    static_assert(std::is_same<decltype(fold(DValueList<1, 2, 3>{},
                                             DValueList<>{}, DAppendOp{})),
                               DValueList<1, 2, 3> >::value,
                  "fold + append = identity");

    static_assert(std::is_same<decltype(fold(DValueList<>{}, DValueList<>{},
                                             DAppendOp{})),
                               DValueList<> >::value, "empty rebuilds to empty");

    // a non-empty seed is the prefix.
    static_assert(std::is_same<decltype(fold(DValueList<2, 3>{},
                                             DValueList<1>{}, DAppendOp{})),
                               DValueList<1, 2, 3> >::value, "seeded prefix");

    // heterogeneity survives the round trip.
    static_assert(std::is_same<decltype(fold(DValueList<1, 'x', true>{},
                                             DValueList<>{}, DAppendOp{})),
                               DValueList<1, 'x', true> >::value,
                  "types preserved");

    ok = ok && (decltype(fold(DValueList<1, 2, 3>{}, DValueList<>{},
                              DAppendOp{}))::size() == 3u);
#endif

    return ok;
}


/*
tests_fold_reverses_with_prepend
  The same fold with prepend instead of append REVERSES the list -- each element
  arrives at the front, so the last one processed ends up first. This pins the
  traversal order a second, independent way (the first being DDigitsOp).
  Tests the following:
  - fold + prepend reverses, for several lengths
  - reversing twice restores the original
  - it is genuinely different from fold + append
*/
bool tests_fold_reverses_with_prepend()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(fold(DValueList<1, 2, 3>{},
                                             DValueList<>{}, DPrependOp{})),
                               DValueList<3, 2, 1> >::value, "reversed");

    static_assert(std::is_same<decltype(fold(DValueList<1, 2, 3, 4>{},
                                             DValueList<>{}, DPrependOp{})),
                               DValueList<4, 3, 2, 1> >::value, "four");

    static_assert(std::is_same<decltype(fold(DValueList<7>{}, DValueList<>{},
                                             DPrependOp{})),
                               DValueList<7> >::value, "a singleton reverses to itself");

    // reversing twice is the identity.
    using once  = decltype(fold(DValueList<1, 2, 3>{}, DValueList<>{},
                                DPrependOp{}));
    using twice = decltype(fold(once{}, DValueList<>{}, DPrependOp{}));
    static_assert(std::is_same<twice, DValueList<1, 2, 3> >::value,
                  "reverse . reverse = identity");

    // and it is not what append does.
    static_assert(!std::is_same<decltype(fold(DValueList<1, 2, 3>{},
                                              DValueList<>{}, DPrependOp{})),
                                decltype(fold(DValueList<1, 2, 3>{},
                                              DValueList<>{}, DAppendOp{}))
                               >::value, "prepend is not append");

    ok = ok && (std::is_same<once, DValueList<3, 2, 1> >::value);
#endif

    return ok;
}


/*
tests_fold_deep
  A long list exercises the recursion to depth.
  Tests the following:
  - a sum and a count over an eight-element list
  - a deep reverse
*/
bool tests_fold_deep()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using deep = DValueList<0, 1, 2, 3, 4, 5, 6, 7>;

    static_assert(fold(deep{}, val<0>, DSumOp{}).value == 28, "0..7 sums to 28");
    static_assert(fold(deep{}, val<0>, DCountOp{}).value == 8, "eight steps");

    static_assert(std::is_same<decltype(fold(deep{}, DValueList<>{},
                                             DPrependOp{})),
                               DValueList<7, 6, 5, 4, 3, 2, 1, 0> >::value,
                  "deep reverse");

    ok = ok && (fold(deep{}, val<0>, DSumOp{}).value == 28);
#endif

    return ok;
}


/*
tests_fold_and_transform_compose
  The two drivers compose: map, then reduce. This is the shape reduce_ct is meant
  to run.
  Tests the following:
  - fold(transform(l, double), 0, sum) is twice the plain sum
  - map-then-reduce over a heterogeneous list, via a truthiness map and a count
  - the composite is a constant expression throughout
*/
bool tests_fold_and_transform_compose()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = DValueList<1, 2, 3>;

    static_assert(fold(list{}, val<0>, DSumOp{}).value == 6, "plain sum");
    static_assert(fold(transform(list{}, DDoubleOp{}), val<0>, DSumOp{}).value
                  == 12, "map then reduce: twice the sum");

    // add three to each, then sum: (4 + 5 + 6) = 15.
    static_assert(fold(transform(list{}, DAdd3Op{}), val<0>, DSumOp{}).value
                  == 15, "another leaf");

    // rebuild a list, then reduce it -- the round trip is transparent.
    static_assert(fold(fold(list{}, DValueList<>{}, DAppendOp{}), val<0>,
                       DSumOp{}).value == 6, "rebuild then reduce");

    // reversing does not change a commutative reduction.
    static_assert(fold(fold(list{}, DValueList<>{}, DPrependOp{}), val<0>,
                       DSumOp{}).value == 6, "sum is order-insensitive");
    // ...but it does change a non-commutative one.
    static_assert(fold(fold(list{}, DValueList<>{}, DPrependOp{}), val<0>,
                       DDigitsOp{}).value == 321, "digits is not");

    ok = ok && (fold(transform(list{}, DDoubleOp{}), val<0>, DSumOp{}).value
                == 12);
#endif

    return ok;
}


/*
tests_fold_seed_may_be_a_plain_value
  The seed is "any object" -- not only a carrier or a list. That distinction has
  teeth: val_t and DValueList are EMPTY types (their value lives in the type), so
  a fold over them cannot tell an accumulator that is threaded through from one
  that is default-constructed afresh at the base case. A plain int accumulator can.
  Tests the following:
  - a plain-value fold reduces correctly, and yields a plain value, not a carrier
  - the empty list returns the SEED -- 42, not a fresh int{} -- which only a
    non-empty accumulator can witness
  - it folds left, and runs at runtime as well as in a constant expression
*/
bool tests_fold_seed_may_be_a_plain_value()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // an ordinary int threads through the fold.
    static_assert(fold(DValueList<1, 2, 3>{}, 0, DIntSumOp{}) == 6, "0 + 1+2+3");
    static_assert(fold(DValueList<1, 2, 3>{}, 10, DIntSumOp{}) == 16,
                  "the seed participates");

    // the result is a plain int -- no carrier in sight.
    static_assert(std::is_same<decltype(fold(DValueList<1, 2, 3>{}, 0,
                                             DIntSumOp{})), int>::value,
                  "yields int");

    // THE base case: the empty list must return the seed ITSELF, not a
    // default-constructed accumulator. With an empty carrier the two are
    // indistinguishable; with an int they are not.
    static_assert(fold(DValueList<>{}, 42, DIntSumOp{}) == 42,
                  "the seed comes back, not int{}");
    static_assert(fold(DValueList<>{}, -7, DIntSumOp{}) == -7, "and again");

    // heterogeneous elements all fold into the one accumulator.
    static_assert(fold(DValueList<1, '\x02', true>{}, 0, DIntSumOp{}) == 4,
                  "1 + 2 + 1");

    // and it runs at runtime, too -- the same driver, the other domain.
    int seed = 42;
    ok = ok && (fold(DValueList<1, 2, 3>{}, seed, DIntSumOp{}) == 48);
    ok = ok && (fold(DValueList<>{}, seed, DIntSumOp{}) == 42);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
