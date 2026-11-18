/*******************************************************************************
* djinterp [cpp]                                     monoid_tests_operations.cpp
*
*
* path:      /tests/djinterp/cpp/functional/monoid_tests_operations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  monoid_tests_operations.cpp
//   Section IV -- GENERIC MONOID OPERATIONS (mempty / mconcat / fold_monoid).

// std
#include <cstddef>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_mempty
  Exercises mempty<T>() for every instance.
  Tests the following:
  - each identity element (sum 0, product 1, all true, any false, min max(),
    max lowest(), string "", vector {})
  - mempty returns the monoid type itself
  - the numeric identities are constant expressions
*/
bool tests_mempty()
{
    bool ok = true;

    ok = ok && (mempty<DSum<int> >().value     == 0);
    ok = ok && (mempty<DProduct<int> >().value == 1);
    ok = ok && (mempty<DAll>().value           == true);
    ok = ok && (mempty<DAny>().value           == false);
    ok = ok && (mempty<DMin<int> >().value ==
                (std::numeric_limits<int>::max)());
    ok = ok && (mempty<DMax<int> >().value ==
                (std::numeric_limits<int>::lowest)());
    ok = ok && (mempty<std::string>().empty());
    ok = ok && (mempty<std::vector<int> >().empty());

    // mempty returns the monoid type.
    static_assert(std::is_same<decltype(mempty<DSum<int> >()),
                               DSum<int> >::value, "mempty type");

    // constexpr numeric identities.
    static_assert(mempty<DSum<int> >().value == 0,     "sum empty ce");
    static_assert(mempty<DProduct<int> >().value == 1, "product empty ce");
    static_assert(mempty<DAll>().value == true,        "all empty ce");
    static_assert(mempty<DAny>().value == false,       "any empty ce");

    return ok;
}


/*
tests_mconcat_multi
  Exercises mconcat over multi-element foldables of each monoid.
  Tests the following:
  - additive and multiplicative folds
  - conjunctive and disjunctive folds (including the short-circuit-value cases)
  - minimum and maximum across the sequence
  - string concatenation
*/
bool tests_mconcat_multi()
{
    bool ok = true;

    // additive / multiplicative.
    ok = ok && (mconcat(std::vector<DSum<int> >{
                    DSum<int>(1), DSum<int>(2), DSum<int>(3) }).value == 6);
    ok = ok && (mconcat(std::vector<DProduct<int> >{
                    DProduct<int>(2), DProduct<int>(3),
                    DProduct<int>(4) }).value == 24);

    // boolean conjunction / disjunction.
    ok = ok && (mconcat(std::vector<DAll>{
                    DAll(true), DAll(true), DAll(true) }).value == true);
    ok = ok && (mconcat(std::vector<DAll>{
                    DAll(true), DAll(false), DAll(true) }).value == false);
    ok = ok && (mconcat(std::vector<DAny>{
                    DAny(false), DAny(false) }).value == false);
    ok = ok && (mconcat(std::vector<DAny>{
                    DAny(false), DAny(true), DAny(false) }).value == true);

    // minimum / maximum across the sequence.
    ok = ok && (mconcat(std::vector<DMin<int> >{
                    DMin<int>(5), DMin<int>(3), DMin<int>(8),
                    DMin<int>(1) }).value == 1);
    ok = ok && (mconcat(std::vector<DMax<int> >{
                    DMax<int>(5), DMax<int>(3), DMax<int>(8),
                    DMax<int>(1) }).value == 8);

    // string concatenation.
    ok = ok && (mconcat(std::vector<std::string>{ "a", "b", "c" }) == "abc");

    return ok;
}


/*
tests_mconcat_single
  Exercises mconcat over one-element foldables.
  Tests the following:
  - a single element collapses to itself for several monoids
*/
bool tests_mconcat_single()
{
    bool ok = true;

    ok = ok && (mconcat(std::vector<DSum<int> >{ DSum<int>(42) }).value == 42);
    ok = ok && (mconcat(std::vector<std::string>{ "x" }) == "x");
    ok = ok && (mconcat(std::vector<DMin<int> >{ DMin<int>(7) }).value == 7);
    ok = ok && (mconcat(std::vector<DMax<int> >{ DMax<int>(7) }).value == 7);

    return ok;
}


/*
tests_mconcat_empty
  Exercises the empty-foldable path of mconcat (the identity seed).
  Tests the following:
  - an empty foldable yields mempty for every monoid
*/
bool tests_mconcat_empty()
{
    bool ok = true;

    ok = ok && (mconcat(std::vector<DSum<int> >{}).value == 0);
    ok = ok && (mconcat(std::vector<DProduct<int> >{}).value == 1);
    ok = ok && (mconcat(std::vector<DAll>{}).value == true);
    ok = ok && (mconcat(std::vector<DAny>{}).value == false);
    ok = ok && (mconcat(std::vector<DMin<int> >{}).value ==
                (std::numeric_limits<int>::max)());
    ok = ok && (mconcat(std::vector<DMax<int> >{}).value ==
                (std::numeric_limits<int>::lowest)());
    ok = ok && (mconcat(std::vector<std::string>{}).empty());

    return ok;
}


/*
tests_mconcat_nested_vector
  Exercises mconcat where the element type is itself a monoid (vector<int>),
  so the fold flattens.
  Tests the following:
  - a multi-element outer vector flattens in order
  - an empty outer vector yields the empty vector
  - a single inner vector collapses to itself
*/
bool tests_mconcat_nested_vector()
{
    bool ok = true;

    typedef std::vector<int> vi;

    const vi flat = mconcat(std::vector<vi>{
        vi{ 1, 2 }, vi{ 3 }, vi{ 4, 5 } });
    ok = ok && (flat == (vi{ 1, 2, 3, 4, 5 }));

    // empty outer -> empty vector.
    ok = ok && (mconcat(std::vector<vi>{}).empty());

    // single inner -> that inner.
    ok = ok && (mconcat(std::vector<vi>{ vi{ 9 } }) == (vi{ 9 }));

    return ok;
}


/*
tests_mconcat_return_type
  Confirms mconcat's return type is the foldable's element (monoid) type.
  Tests the following:
  - decltype(mconcat(vector<sum<int>>)) is sum<int>
  - foldable_value_type_t of vector<string> is string
*/
bool tests_mconcat_return_type()
{
    static_assert(
        std::is_same<
            decltype(mconcat(std::declval<const std::vector<DSum<int> >&>())),
            DSum<int> >::value,
        "mconcat returns the monoid type");

    static_assert(
        std::is_same<foldable_value_type_t<std::vector<std::string> >,
                     std::string>::value,
        "foldable value type of vector<string> is string");

    return true;
}


/*
tests_fold_monoid_multi
  Exercises fold_monoid (map each element into a monoid, then combine).
  Tests the following:
  - map int -> sum / product
  - map int -> max / min (extremum of the sequence)
  - map int -> a predicate monoid (all elements positive?)
  - map string -> length, then sum the lengths
*/
bool tests_fold_monoid_multi()
{
    bool ok = true;

    // map each int into a sum -> total.
    ok = ok && (fold_monoid(std::vector<int>{ 1, 2, 3 },
                    [](int _x){ return DSum<int>(_x); }).value == 6);

    // map into a product.
    ok = ok && (fold_monoid(std::vector<int>{ 1, 2, 3, 4 },
                    [](int _x){ return DProduct<int>(_x); }).value == 24);

    // map into max / min -> extremum.
    ok = ok && (fold_monoid(std::vector<int>{ 5, 2, 9, 1 },
                    [](int _x){ return DMax<int>(_x); }).value == 9);
    ok = ok && (fold_monoid(std::vector<int>{ 5, 2, 9, 1 },
                    [](int _x){ return DMin<int>(_x); }).value == 1);

    // map into a predicate monoid -> are all elements positive?
    ok = ok && (fold_monoid(std::vector<int>{ 1, 2, 3 },
                    [](int _x){ return DAll(_x > 0); }).value == true);
    ok = ok && (fold_monoid(std::vector<int>{ 1, -2, 3 },
                    [](int _x){ return DAll(_x > 0); }).value == false);

    // map string -> length, then sum the lengths.
    ok = ok && (fold_monoid(std::vector<std::string>{ "a", "bb", "ccc" },
                    [](const std::string& _s){
                        return DSum<std::size_t>(_s.size());
                    }).value == static_cast<std::size_t>(6));

    return ok;
}


/*
tests_fold_monoid_empty
  Exercises the empty-foldable path of fold_monoid.
  Tests the following:
  - an empty foldable yields the identity of the MAPPED monoid, even though the
    mapping function is never invoked
*/
bool tests_fold_monoid_empty()
{
    bool ok = true;

    ok = ok && (fold_monoid(std::vector<int>{},
                    [](int _x){ return DSum<int>(_x); }).value == 0);
    ok = ok && (fold_monoid(std::vector<int>{},
                    [](int _x){ return DProduct<int>(_x); }).value == 1);
    ok = ok && (fold_monoid(std::vector<int>{},
                    [](int _x){ return std::to_string(_x); }).empty());

    return ok;
}


/*
tests_fold_monoid_type_deduction
  Confirms the resulting monoid is deduced from the mapping function's result.
  Tests the following:
  - a function returning sum<long> yields a sum<long> result
  - a function returning std::string yields a std::string result
  - the deduced values are correct
*/
bool tests_fold_monoid_type_deduction()
{
    bool ok = true;

    // deduced monoid is sum<long>.
    auto s = fold_monoid(std::vector<int>{ 1, 2, 3 },
                         [](int _x){ return DSum<long>(_x); });
    ok = ok && (std::is_same<decltype(s), DSum<long> >::value);
    ok = ok && (s.value == 6L);

    // deduced monoid is std::string.
    auto str = fold_monoid(std::vector<int>{ 1, 2, 3 },
                          [](int _x){ return std::to_string(_x); });
    ok = ok && (std::is_same<decltype(str), std::string>::value);
    ok = ok && (str == "123");

    return ok;
}


/*
tests_fold_monoid_string_concat
  Exercises fold_monoid with a string-producing mapping function.
  Tests the following:
  - the mapped strings concatenate in fold order
  - an empty foldable yields the empty-string identity
  - a single element maps and stands alone
*/
bool tests_fold_monoid_string_concat()
{
    bool ok = true;

    ok = ok && (fold_monoid(std::vector<int>{ 1, 2, 3 },
                    [](int _x){ return std::to_string(_x); }) == "123");

    ok = ok && (fold_monoid(std::vector<int>{},
                    [](int _x){ return std::to_string(_x); }).empty());

    ok = ok && (fold_monoid(std::vector<int>{ 7 },
                    [](int _x){ return std::to_string(_x); }) == "7");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
