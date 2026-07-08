/******************************************************************************
* djinterp [test]                                foldable_tests_predicates.cpp
*
*   Section II.7 of the foldable.hpp suite: fold_any (existential) and fold_all
* (universal).  Covers a matching element, no match, the empty foldable (any is
* false; all is vacuously true), the bool-conversion of a non-bool predicate
* result, the non-std seq<T> fixture, and the visitation behaviour.
*
*   A NOTE ON "DOES NOT SHORT-CIRCUIT":
*   The header states these folds do not short-circuit -- meaning the underlying
* fold_left LOOP visits every element (there is no early loop exit).  The
* PREDICATE, however, is guarded by the reducer's `||` / `&&`, which are
* short-circuiting operators: once the disjunction is true (any) or the
* conjunction is false (all), the predicate is no longer evaluated for the
* remaining elements.  So the observable predicate-call count settles the moment
* the result is decided, yet stays equal to the element count while the result
* is still undetermined.  Both facets are pinned down below.
*
* path:      /tests/djinterp/core/functional/foldable_tests_predicates.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                fold_any                                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_any_true_when_one_matches
  Tests the following:
  - fold_any is true when at least one element satisfies the predicate.
*/
static bool
tests_any_true_when_one_matches()
{
    std::vector<int> xs;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    return ::djinterp::fold_any(
        xs,
        [](int _x) -> bool
        {
            return _x > 3;
        });
}

/*
tests_any_false_when_none_match
  Tests the following:
  - fold_any is false when no element satisfies the predicate.
*/
static bool
tests_any_false_when_none_match()
{
    std::vector<int> xs;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    return (!::djinterp::fold_any(
        xs,
        [](int _x) -> bool
        {
            return _x > 10;
        }));
}

/*
tests_any_false_on_empty
  Tests the following:
  - fold_any of an empty foldable is false (the false seed is never disturbed).
*/
static bool
tests_any_false_on_empty()
{
    std::vector<int> xs;

    return (!::djinterp::fold_any(
        xs,
        [](int) -> bool
        {
            return true;
        }));
}

/*
tests_any_bool_conversion
  Tests the following:
  - a predicate returning a non-bool, bool-convertible value (int) is coerced
    via static_cast<bool>: (x % 2) over [2,4,5] is true (5 is odd).
*/
static bool
tests_any_bool_conversion()
{
    std::vector<int> xs;

    xs.push_back(2);
    xs.push_back(4);
    xs.push_back(5);

    return ::djinterp::fold_any(
        xs,
        [](int _x) -> int
        {
            return _x % 2;
        });
}


///////////////////////////////////////////////////////////////////////////////
///                fold_all                                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_all_true_when_all_match
  Tests the following:
  - fold_all is true when every element satisfies the predicate.
*/
static bool
tests_all_true_when_all_match()
{
    std::vector<int> xs;

    xs.push_back(2);
    xs.push_back(4);
    xs.push_back(6);

    return ::djinterp::fold_all(
        xs,
        [](int _x) -> bool
        {
            return (_x % 2) == 0;
        });
}

/*
tests_all_false_when_one_fails
  Tests the following:
  - fold_all is false when some element fails the predicate.
*/
static bool
tests_all_false_when_one_fails()
{
    std::vector<int> xs;

    xs.push_back(2);
    xs.push_back(4);
    xs.push_back(5);

    return (!::djinterp::fold_all(
        xs,
        [](int _x) -> bool
        {
            return (_x % 2) == 0;
        }));
}

/*
tests_all_true_on_empty_vacuous
  Tests the following:
  - fold_all of an empty foldable is vacuously true (the true seed survives).
*/
static bool
tests_all_true_on_empty_vacuous()
{
    std::vector<int> xs;

    return ::djinterp::fold_all(
        xs,
        [](int) -> bool
        {
            return false;
        });
}

/*
tests_all_bool_conversion
  Tests the following:
  - a predicate returning int is coerced via static_cast<bool>: (x % 2) over
    [1,3,5] is true (every element is odd -> non-zero -> true).
*/
static bool
tests_all_bool_conversion()
{
    std::vector<int> xs;

    xs.push_back(1);
    xs.push_back(3);
    xs.push_back(5);

    return ::djinterp::fold_all(
        xs,
        [](int _x) -> int
        {
            return _x % 2;
        });
}


///////////////////////////////////////////////////////////////////////////////
///                EDGE CASES + GENERICITY                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_any_all_single
  Tests the following:
  - single-element edges: any([5], x>9) is false; all([5], x>0) is true.
*/
static bool
tests_any_all_single()
{
    std::vector<int> xs;
    bool             any_false;
    bool             all_true;

    xs.push_back(5);

    any_false = ::djinterp::fold_any(
        xs,
        [](int _x) -> bool { return _x > 9; });

    all_true = ::djinterp::fold_all(
        xs,
        [](int _x) -> bool { return _x > 0; });

    return ( (!any_false) &&
             all_true );
}

/*
tests_any_all_over_seq
  Tests the following:
  - both folds dispatch over the non-std seq<T> fixture: any(>2) is true and
    all(>0) is true over [3,4,5].
*/
static bool
tests_any_all_over_seq()
{
    seq<int> s;
    bool     any_ok;
    bool     all_ok;

    s.data.push_back(3);
    s.data.push_back(4);
    s.data.push_back(5);

    any_ok = ::djinterp::fold_any(
        s,
        [](int _x) -> bool { return _x > 2; });

    all_ok = ::djinterp::fold_all(
        s,
        [](int _x) -> bool { return _x > 0; });

    return (any_ok && all_ok);
}


///////////////////////////////////////////////////////////////////////////////
///                VISITATION  (loop vs predicate short-circuit)             ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_any_visits_all_while_undetermined
  Tests the following:
  - while no element matches, fold_any evaluates the predicate on EVERY element
    (call count == element count) and returns false.
*/
static bool
tests_any_visits_all_while_undetermined()
{
    std::vector<int> xs;
    int              calls;
    bool             result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    calls = 0;

    result = ::djinterp::fold_any(
        xs,
        [&calls](int _x) -> bool
        {
            ++calls;

            return _x > 10;   // never true -> result stays undetermined
        });

    return ( (result == false) &&
             (calls == 4) );
}

/*
tests_all_visits_all_while_undetermined
  Tests the following:
  - while every element passes, fold_all evaluates the predicate on EVERY
    element (call count == element count) and returns true.
*/
static bool
tests_all_visits_all_while_undetermined()
{
    std::vector<int> xs;
    int              calls;
    bool             result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    calls = 0;

    result = ::djinterp::fold_all(
        xs,
        [&calls](int _x) -> bool
        {
            ++calls;

            return _x > 0;    // always true -> result stays undetermined
        });

    return ( (result == true) &&
             (calls == 4) );
}

/*
tests_any_predicate_short_circuits_after_match
  Tests the following:
  - once fold_any's disjunction becomes true the predicate is no longer
    evaluated (the reducer's `||` guards it): a match at the third element of
    four yields exactly 3 predicate calls, and a true result.
*/
static bool
tests_any_predicate_short_circuits_after_match()
{
    std::vector<int> xs;
    int              calls;
    bool             result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    calls = 0;

    result = ::djinterp::fold_any(
        xs,
        [&calls](int _x) -> bool
        {
            ++calls;

            return _x == 3;   // matches at index 2 (the third element)
        });

    return ( (result == true) &&
             (calls == 3) );
}

/*
tests_all_predicate_short_circuits_after_failure
  Tests the following:
  - once fold_all's conjunction becomes false the predicate is no longer
    evaluated (the reducer's `&&` guards it): a failure at the third element of
    four yields exactly 3 predicate calls, and a false result.
*/
static bool
tests_all_predicate_short_circuits_after_failure()
{
    std::vector<int> xs;
    int              calls;
    bool             result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    calls = 0;

    result = ::djinterp::fold_all(
        xs,
        [&calls](int _x) -> bool
        {
            ++calls;

            return _x != 3;   // fails at index 2 (the third element)
        });

    return ( (result == false) &&
             (calls == 3) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_predicate_block()
{
    dt::block_spec block;

    block.name       = "II.7 fold_any / fold_all";
    block.descriptor =
        "existential / universal reductions, empties, and visitation";

    block.tests.push_back(dt::test_spec{
        "any: one matches",
        "true when some element satisfies the predicate",
        &tests_any_true_when_one_matches });

    block.tests.push_back(dt::test_spec{
        "any: none match",
        "false when no element satisfies the predicate",
        &tests_any_false_when_none_match });

    block.tests.push_back(dt::test_spec{
        "any: empty",
        "false for an empty foldable",
        &tests_any_false_on_empty });

    block.tests.push_back(dt::test_spec{
        "any: bool conversion",
        "non-bool (int) predicate result coerced via static_cast",
        &tests_any_bool_conversion });

    block.tests.push_back(dt::test_spec{
        "all: all match",
        "true when every element satisfies the predicate",
        &tests_all_true_when_all_match });

    block.tests.push_back(dt::test_spec{
        "all: one fails",
        "false when some element fails the predicate",
        &tests_all_false_when_one_fails });

    block.tests.push_back(dt::test_spec{
        "all: empty (vacuous)",
        "vacuously true for an empty foldable",
        &tests_all_true_on_empty_vacuous });

    block.tests.push_back(dt::test_spec{
        "all: bool conversion",
        "non-bool (int) predicate result coerced via static_cast",
        &tests_all_bool_conversion });

    block.tests.push_back(dt::test_spec{
        "single-element edges",
        "any([5],>9) false; all([5],>0) true",
        &tests_any_all_single });

    block.tests.push_back(dt::test_spec{
        "over seq fixture",
        "any/all dispatch over seq<T>",
        &tests_any_all_over_seq });

    block.tests.push_back(dt::test_spec{
        "any: visits all while undetermined",
        "no match -> predicate called on every element",
        &tests_any_visits_all_while_undetermined });

    block.tests.push_back(dt::test_spec{
        "all: visits all while undetermined",
        "all pass -> predicate called on every element",
        &tests_all_visits_all_while_undetermined });

    block.tests.push_back(dt::test_spec{
        "any: predicate guarded after match",
        "|| stops predicate calls once true (3 of 4)",
        &tests_any_predicate_short_circuits_after_match });

    block.tests.push_back(dt::test_spec{
        "all: predicate guarded after failure",
        "&& stops predicate calls once false (3 of 4)",
        &tests_all_predicate_short_circuits_after_failure });

    return block;
}


NS_END  // testing
NS_END  // djinterp
