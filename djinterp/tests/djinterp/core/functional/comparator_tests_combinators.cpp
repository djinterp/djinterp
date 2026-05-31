#include "comparator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_combinators_reversed
  Verifies reversed produces the inverse ordering of its inner comparator.
  Tests the following:
  - reversed(natural) reports a > b semantics (b before a)
  - both operand orders flip relative to the inner comparator
*/
bool
test_combinators_reversed(
)
{
    D_INTERNAL_CMP_CHECK(comparators::reversed(comparators::natural<int>())(2, 1)
                         == true);
    D_INTERNAL_CMP_CHECK(comparators::reversed(comparators::natural<int>())(1, 2)
                         == false);

    return true;
}


/*
test_combinators_reversed_strictness
  Confirms reversed preserves strict-weak-ordering irreflexivity. Because it
  swaps operands rather than negating, equal operands stay not-less.
  Tests the following:
  - reversed(natural)(x, x) is false
*/
bool
test_combinators_reversed_strictness(
)
{
    D_INTERNAL_CMP_CHECK(comparators::reversed(comparators::natural<int>())(4, 4)
                         == false);

    return true;
}


/*
test_combinators_then_tiebreak
  Verifies then falls back to the secondary comparator when the primary
  reports equivalence.
  Tests the following:
  - equal primary keys (rank) defer to the secondary (id)
  - the secondary ordering then decides the result
*/
bool
test_combinators_then_tiebreak(
)
{
    record a = { 5, 1 };
    record b = { 5, 2 };   // same rank, larger id

    // primary by rank (tie), secondary by id -> a before b.
    D_INTERNAL_CMP_CHECK(
        comparators::then(comparators::by_key(get_rank()),
                          comparators::by_key(get_id()))(a, b) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::then(comparators::by_key(get_rank()),
                          comparators::by_key(get_id()))(b, a) == false);

    return true;
}


/*
test_combinators_then_primary_wins
  Confirms then never consults the secondary when the primary already
  decides the ordering.
  Tests the following:
  - distinct primary keys settle the comparison regardless of the secondary
  - a secondary that would disagree does not change the outcome
*/
bool
test_combinators_then_primary_wins(
)
{
    record a = { 1, 9 };   // lower rank, higher id
    record b = { 2, 0 };   // higher rank, lower id

    // primary by rank says a < b; secondary by id would say b < a, but the
    // primary is decisive, so the chain reports a < b.
    D_INTERNAL_CMP_CHECK(
        comparators::then(comparators::by_key(get_rank()),
                          comparators::by_key(get_id()))(a, b) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::then(comparators::by_key(get_rank()),
                          comparators::by_key(get_id()))(b, a) == false);

    return true;
}


/*
test_combinators_then_nested
  Verifies a three-level tie-breaker chain by nesting then. The fixture ties
  on the first two keys so resolution reaches the innermost comparator.
  Tests the following:
  - then(rank, then(id, reversed(rank))) resolves correctly
  - ties on both rank and id defer to the third comparator
*/
bool
test_combinators_then_nested(
)
{
    // Tie on rank and id; the third comparator (by id, reversed) breaks it.
    record a = { 1, 5 };
    record b = { 1, 5 };

    // Equal on rank and id -> third level compares id reversed -> tie again,
    // so neither is less: chain reports false both ways (true equivalence).
    bool ab = comparators::then(
                  comparators::by_key(get_rank()),
                  comparators::then(comparators::by_key(get_id()),
                                    comparators::reversed(
                                        comparators::by_key(get_id()))))(a, b);
    bool ba = comparators::then(
                  comparators::by_key(get_rank()),
                  comparators::then(comparators::by_key(get_id()),
                                    comparators::reversed(
                                        comparators::by_key(get_id()))))(b, a);

    D_INTERNAL_CMP_CHECK(ab == false);
    D_INTERNAL_CMP_CHECK(ba == false);

    // Now distinct ids: tie on rank, second level (by id) decides.
    record c = { 1, 2 };
    record d = { 1, 7 };

    D_INTERNAL_CMP_CHECK(
        comparators::then(
            comparators::by_key(get_rank()),
            comparators::then(comparators::by_key(get_id()),
                              comparators::by_key(get_rank())))(c, d) == true);

    return true;
}


/*
test_combinators_lifted
  Verifies lifted composes a comparator with a key function so the result
  orders a different type by the comparator applied to extracted keys.
  Tests the following:
  - lifted(natural<int>, get_rank) orders records by ascending rank
*/
bool
test_combinators_lifted(
)
{
    record lo = { 1, 0 };
    record hi = { 6, 0 };

    D_INTERNAL_CMP_CHECK(
        comparators::lifted(comparators::natural<int>(), get_rank())(lo, hi)
        == true);
    D_INTERNAL_CMP_CHECK(
        comparators::lifted(comparators::natural<int>(), get_rank())(hi, lo)
        == false);

    return true;
}


/*
test_combinators_lifted_reversed
  Confirms lifted carries an arbitrary inner comparator, not just natural:
  lifting a reversed comparator yields descending order on the key.
  Tests the following:
  - lifted(reversed(natural), get_rank) orders records by descending rank
*/
bool
test_combinators_lifted_reversed(
)
{
    record lo = { 1, 0 };
    record hi = { 6, 0 };

    D_INTERNAL_CMP_CHECK(
        comparators::lifted(
            comparators::reversed(comparators::natural<int>()),
            get_rank())(hi, lo) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::lifted(
            comparators::reversed(comparators::natural<int>()),
            get_rank())(lo, hi) == false);

    return true;
}


/*
run_combinators_tests
  Aggregates every combinator-section test.
  Tests the following:
  - all reversed / then / lifted tests pass
*/
bool
run_combinators_tests(
)
{
    return ( test_combinators_reversed()            &&
             test_combinators_reversed_strictness() &&
             test_combinators_then_tiebreak()        &&
             test_combinators_then_primary_wins()    &&
             test_combinators_then_nested()          &&
             test_combinators_lifted()               &&
             test_combinators_lifted_reversed() );
}


NS_END  // testing
NS_END  // djinterp
