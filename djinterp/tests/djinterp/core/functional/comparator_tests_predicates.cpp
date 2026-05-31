#include "comparator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_predicates_equal_under
  Verifies equal_under derives an equivalence predicate from a comparator via
  the strict-weak-ordering identity a == b iff !(a<b) && !(b<a).
  Tests the following:
  - equal operands are reported equivalent
  - unequal operands are reported not-equivalent in both directions
*/
bool
test_predicates_equal_under(
)
{
    D_INTERNAL_CMP_CHECK(
        comparators::equal_under(comparators::natural<int>())(4, 4) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::equal_under(comparators::natural<int>())(4, 5) == false);
    D_INTERNAL_CMP_CHECK(
        comparators::equal_under(comparators::natural<int>())(5, 4) == false);

    return true;
}


/*
test_predicates_equal_under_by_key
  Confirms equal_under composes over a key-based comparator, yielding
  equivalence on the key rather than the whole object.
  Tests the following:
  - records with equal rank are equivalent under by_key(rank) even when other
    fields differ
  - records with differing rank are not equivalent
*/
bool
test_predicates_equal_under_by_key(
)
{
    record a = { 5, 1 };
    record b = { 5, 99 };   // same rank, different id
    record c = { 6, 1 };    // different rank

    D_INTERNAL_CMP_CHECK(
        comparators::equal_under(comparators::by_key(get_rank()))(a, b)
        == true);
    D_INTERNAL_CMP_CHECK(
        comparators::equal_under(comparators::by_key(get_rank()))(a, c)
        == false);

    return true;
}


/*
test_predicates_less_than
  Verifies less_than binds the second operand of a comparator, yielding the
  unary predicate v -> cmp(v, bound).
  Tests the following:
  - values below the bound satisfy the predicate
  - the bound itself and values above it do not
*/
bool
test_predicates_less_than(
)
{
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(comparators::natural<int>(), 10)(3) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(comparators::natural<int>(), 10)(10) == false);
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(comparators::natural<int>(), 10)(11) == false);

    return true;
}


/*
test_predicates_greater_than
  Verifies greater_than binds the first operand of a comparator, yielding the
  unary predicate v -> cmp(bound, v).
  Tests the following:
  - values above the bound satisfy the predicate
  - the bound itself and values below it do not
*/
bool
test_predicates_greater_than(
)
{
    D_INTERNAL_CMP_CHECK(
        comparators::greater_than(comparators::natural<int>(), 10)(11) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::greater_than(comparators::natural<int>(), 10)(10) == false);
    D_INTERNAL_CMP_CHECK(
        comparators::greater_than(comparators::natural<int>(), 10)(3) == false);

    return true;
}


/*
test_predicates_boundary
  Exercises the exact boundary of the one-sided binders: neither less_than
  nor greater_than includes the bound (both derive from a strict ordering).
  Tests the following:
  - at v == bound, both less_than and greater_than report false
  - the two predicates partition the strict-greater and strict-less regions
*/
bool
test_predicates_boundary(
)
{
    int bound = 0;

    D_INTERNAL_CMP_CHECK(
        comparators::less_than(comparators::natural<int>(), bound)(bound)
        == false);
    D_INTERNAL_CMP_CHECK(
        comparators::greater_than(comparators::natural<int>(), bound)(bound)
        == false);

    // disjoint, complementary away from the bound.
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(comparators::natural<int>(), bound)(-1) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::greater_than(comparators::natural<int>(), bound)(-1)
        == false);

    return true;
}


/*
test_predicates_with_reversed
  Confirms the binders carry whatever ordering the comparator defines: bound
  against a reversed comparator flips the satisfied region.
  Tests the following:
  - less_than(reversed(natural), bound) is satisfied by values ABOVE the
    bound, since "less" under the reversed order means numerically greater
*/
bool
test_predicates_with_reversed(
)
{
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(
            comparators::reversed(comparators::natural<int>()), 5)(9) == true);
    D_INTERNAL_CMP_CHECK(
        comparators::less_than(
            comparators::reversed(comparators::natural<int>()), 5)(1) == false);

    return true;
}


/*
run_predicates_tests
  Aggregates every derived-predicate test.
  Tests the following:
  - all equal_under / less_than / greater_than tests pass
*/
bool
run_predicates_tests(
)
{
    return ( test_predicates_equal_under()        &&
             test_predicates_equal_under_by_key()  &&
             test_predicates_less_than()           &&
             test_predicates_greater_than()        &&
             test_predicates_boundary()            &&
             test_predicates_with_reversed() );
}


NS_END  // testing
NS_END  // djinterp
