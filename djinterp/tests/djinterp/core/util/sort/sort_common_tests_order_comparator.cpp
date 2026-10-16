/*******************************************************************************
* djinterp [core]                         sort_common_tests_order_comparator.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests_order_comparator.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_order_comparator_ascending
bool tests_sort_common_order_comparator_ascending()
{
    verdict result;

    ::djinterp::internal::order_comparator<less<int> > ascending(
        less<int>(), sort_order::ascending);

    //   sort_order::ascending is the pass-through case: the wrapped
    // comparator is consulted with the operands in the order they arrived.
    result.check(ascending(1, 2));
    result.check(!ascending(2, 1));
    result.check(!ascending(2, 2));

    return result.ok();
}


// tests_sort_common_order_comparator_descending
bool tests_sort_common_order_comparator_descending()
{
    verdict     result;
    even_first  parity;
    std::size_t i;
    std::size_t j;
    int         values[5];

    ::djinterp::internal::order_comparator<less<int> > descending(
        less<int>(), sort_order::descending);

    ::djinterp::internal::order_comparator<even_first> flipped_parity(
        parity, sort_order::descending);

    //   sort_order::descending SWAPS THE OPERANDS rather than negating the
    // result.  The distinction matters: negation would report equivalent
    // elements as ordered and destroy the strict weak ordering, which in
    // turn would break every algorithm downstream.
    result.check(descending(2, 1));
    result.check(!descending(1, 2));
    result.check(!descending(2, 2));

    //   Equivalence is preserved in both directions -- the property a
    // negating implementation would lose.
    values[0] = -4;
    values[1] = 0;
    values[2] = 3;
    values[3] = 3;
    values[4] = 8;

    for (i = 0; i < 5; ++i)
    {
        result.check(!descending(values[i], values[i]));
    }

    //   The flip applies to an ARBITRARY strict weak ordering, not just a
    // natural one.  even_first makes every even value precede every odd one
    // and treats same-parity values as equivalent; reversed, it must put odd
    // first while still calling the parities equivalent.
    result.check(parity(2, 3));
    result.check(!flipped_parity(2, 3));
    result.check(flipped_parity(3, 2));

    for (i = 0; i < 5; ++i)
    {
        for (j = 0; j < 5; ++j)
        {
            result.check(flipped_parity(values[i], values[j]) ==
                         parity(values[j], values[i]));
        }
    }

    return result.ok();
}


// tests_sort_common_order_comparator_none
bool tests_sort_common_order_comparator_none()
{
    verdict result;

    ::djinterp::internal::order_comparator<less<int> > unspecified(
        less<int>(), sort_order::none);

    ::djinterp::internal::order_comparator<less<int> > ascending(
        less<int>(), sort_order::ascending);

    //   The branch tests for descending ONLY, so sort_order::none falls
    // through to the ascending path.  That is worth pinning: "none" reads
    // like "leave the order alone", but it sorts -- ascending.
    result.check(unspecified(1, 2));
    result.check(!unspecified(2, 1));
    result.check(!unspecified(2, 2));

    result.check(unspecified(1, 2) == ascending(1, 2));
    result.check(unspecified(2, 1) == ascending(2, 1));
    result.check(unspecified(2, 2) == ascending(2, 2));

    return result.ok();
}


// tests_sort_common_order_comparator_heterogeneous
bool tests_sort_common_order_comparator_heterogeneous()
{
    verdict result;

    ::djinterp::internal::order_comparator<cross_type_less> ascending(
        cross_type_less(), sort_order::ascending);

    ::djinterp::internal::order_comparator<cross_type_less> descending(
        cross_type_less(), sort_order::descending);

    //   order_comparator's call operator is a template over BOTH operand
    // types, so a heterogeneous comparator survives the wrapping.  Mixed
    // int/long and int/double calls exercise that.
    result.check(ascending(1, 2L));
    result.check(!ascending(2L, 1));

    result.check(descending(2L, 1));
    result.check(!descending(1, 2L));

    result.check(ascending(1, 1.5));
    result.check(descending(1.5, 1));

    //   The wrapper is copyable and carries its order with it, which is what
    // lets the *_sort_ordered entry points pass it by value into an
    // algorithm.
    ::djinterp::internal::order_comparator<cross_type_less> copy(descending);

    result.check(copy(2L, 1));
    result.check(!copy(1, 2L));

    return result.ok();
}


// tests_sort_common_order_comparator_drives_a_sort
bool tests_sort_common_order_comparator_drives_a_sort()
{
    verdict          result;
    std::vector<int> ascending_out;
    std::vector<int> descending_out;
    std::vector<int> none_out;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    ascending_out       = shape_random(64);
    descending_out      = ascending_out;
    none_out            = ascending_out;
    expected_ascending  = ascending_out;
    expected_descending = ascending_out;

    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    //   End to end: the wrapper is a comparator like any other, so it drops
    // straight into an algorithm.  This is exactly what the *_sort_ordered
    // entry points do internally.
    insertion_sort(
        ascending_out.begin(),
        ascending_out.end(),
        ::djinterp::internal::order_comparator<less<int> >(
            less<int>(), sort_order::ascending));

    insertion_sort(
        descending_out.begin(),
        descending_out.end(),
        ::djinterp::internal::order_comparator<less<int> >(
            less<int>(), sort_order::descending));

    insertion_sort(
        none_out.begin(),
        none_out.end(),
        ::djinterp::internal::order_comparator<less<int> >(
            less<int>(), sort_order::none));

    result.check(ranges_equal(ascending_out, expected_ascending));
    result.check(ranges_equal(descending_out, expected_descending));
    result.check(ranges_equal(none_out, expected_ascending));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
