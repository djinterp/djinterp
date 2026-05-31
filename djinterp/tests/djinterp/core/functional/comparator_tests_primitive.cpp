#include "comparator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_primitive_natural_orders
  Verifies the natural (operator<) comparator over a built-in type.
  Tests the following:
  - a < b reports true
  - b < a reports false
  - the comparator works on a second built-in type (double)
*/
bool
test_primitive_natural_orders(
)
{
    D_INTERNAL_CMP_CHECK(comparators::natural<int>()(1, 2) == true);
    D_INTERNAL_CMP_CHECK(comparators::natural<int>()(2, 1) == false);
    D_INTERNAL_CMP_CHECK(comparators::natural<double>()(1.5, 2.5) == true);
    D_INTERNAL_CMP_CHECK(comparators::natural<double>()(2.5, 1.5) == false);

    return true;
}


/*
test_primitive_natural_strictness
  Confirms the natural comparator is irreflexive (a strict weak ordering
  never reports an element as less than itself).
  Tests the following:
  - natural(x, x) is false for equal operands
*/
bool
test_primitive_natural_strictness(
)
{
    D_INTERNAL_CMP_CHECK(comparators::natural<int>()(7, 7) == false);
    D_INTERNAL_CMP_CHECK(comparators::natural<int>()(0, 0) == false);

    return true;
}


/*
test_primitive_by_key_functor
  Verifies by_key builds a comparator from a key-extracting functor.
  Tests the following:
  - records order by their extracted rank
  - equal ranks compare as not-less in both directions
*/
bool
test_primitive_by_key_functor(
)
{
    record lo = { 1, 0 };
    record hi = { 2, 0 };
    record eq = { 1, 9 };

    D_INTERNAL_CMP_CHECK(comparators::by_key(get_rank())(lo, hi) == true);
    D_INTERNAL_CMP_CHECK(comparators::by_key(get_rank())(hi, lo) == false);
    D_INTERNAL_CMP_CHECK(comparators::by_key(get_rank())(lo, eq) == false);
    D_INTERNAL_CMP_CHECK(comparators::by_key(get_rank())(eq, lo) == false);

    return true;
}


/*
test_primitive_by_key_function_pointer
  Confirms by_key accepts a plain function (pointer) as its key, not just a
  functor. The address-of form keeps _KeyFn a storable function pointer on
  both the C++11 and C++98 paths.
  Tests the following:
  - a function-pointer key produces the same ordering as the functor key
*/
bool
test_primitive_by_key_function_pointer(
)
{
    record lo = { 3, 0 };
    record hi = { 8, 0 };

    D_INTERNAL_CMP_CHECK(comparators::by_key(&rank_of)(lo, hi) == true);
    D_INTERNAL_CMP_CHECK(comparators::by_key(&rank_of)(hi, lo) == false);

    return true;
}


/*
test_primitive_by_member_primary
  Verifies by_member builds a comparator from a pointer-to-data-member.
  Tests the following:
  - records order by the pointed-to member (rank)
  - reversed operands reverse the result
*/
bool
test_primitive_by_member_primary(
)
{
    record lo = { 1, 5 };
    record hi = { 4, 5 };

    D_INTERNAL_CMP_CHECK(comparators::by_member(&record::rank)(lo, hi) == true);
    D_INTERNAL_CMP_CHECK(comparators::by_member(&record::rank)(hi, lo) == false);

    return true;
}


/*
test_primitive_by_member_secondary
  Confirms by_member targets the named member specifically, ordering by id
  while ignoring rank.
  Tests the following:
  - ordering follows id when rank would disagree
*/
bool
test_primitive_by_member_secondary(
)
{
    record a = { 9, 1 };   // higher rank, lower id
    record b = { 0, 2 };   // lower rank, higher id

    D_INTERNAL_CMP_CHECK(comparators::by_member(&record::id)(a, b) == true);
    D_INTERNAL_CMP_CHECK(comparators::by_member(&record::id)(b, a) == false);

    return true;
}


/*
test_primitive_by_function
  Verifies by_function wraps an arbitrary binary callable as a comparator.
  Tests the following:
  - a descending raw functor, wrapped, orders larger-before-smaller
*/
bool
test_primitive_by_function(
)
{
    D_INTERNAL_CMP_CHECK(comparators::by_function(desc_int())(2, 1) == true);
    D_INTERNAL_CMP_CHECK(comparators::by_function(desc_int())(1, 2) == false);
    D_INTERNAL_CMP_CHECK(comparators::by_function(desc_int())(2, 2) == false);

    return true;
}


/*
test_primitive_by_function_equiv
  Confirms by_function is a transparent pass-through: the wrapped comparator
  yields exactly the underlying callable's result.
  Tests the following:
  - wrapped(asc_int) agrees with asc_int across ordered, reversed, and equal
    operand pairs
*/
bool
test_primitive_by_function_equiv(
)
{
    asc_int raw;

    D_INTERNAL_CMP_CHECK(comparators::by_function(asc_int())(1, 2) == raw(1, 2));
    D_INTERNAL_CMP_CHECK(comparators::by_function(asc_int())(2, 1) == raw(2, 1));
    D_INTERNAL_CMP_CHECK(comparators::by_function(asc_int())(5, 5) == raw(5, 5));

    return true;
}


/*
run_primitive_tests
  Aggregates every primitive-section test.
  Tests the following:
  - all natural / by_key / by_member / by_function tests pass
*/
bool
run_primitive_tests(
)
{
    return ( test_primitive_natural_orders()          &&
             test_primitive_natural_strictness()      &&
             test_primitive_by_key_functor()          &&
             test_primitive_by_key_function_pointer() &&
             test_primitive_by_member_primary()        &&
             test_primitive_by_member_secondary()      &&
             test_primitive_by_function()              &&
             test_primitive_by_function_equiv() );
}


NS_END  // testing
NS_END  // djinterp
