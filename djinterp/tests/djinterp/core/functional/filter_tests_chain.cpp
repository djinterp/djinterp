#include "filter_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_chain_empty_passthrough
  Verifies an empty chain passes every element through unchanged (it seeds
  with all indices and applies no operation).
  Tests the following:
  - an empty chain over a non-empty input yields all elements, in order,
    with a success status
*/
bool
test_chain_empty_passthrough(
)
{
    filter_chain<int>  chain;
    std::vector<int>   in = { 1, 2, 3 };
    filter_result<int> r  = chain.apply(in);

    D_INTERNAL_FLT_CHECK(chain.is_empty());
    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.ok());
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 3);

    return true;
}


/*
test_chain_single_op
  Verifies a chain with one operation applies it and maps indices back to the
  original input.
  Tests the following:
  - a take_first(2) op keeps the first two elements with original indices 0,1
*/
bool
test_chain_single_op(
)
{
    filter_chain<int> chain;
    chain.add(internal::make_take_first_op<int>(2));

    std::vector<int>   in = { 5, 6, 7, 8 };
    filter_result<int> r  = chain.apply(in);

    D_INTERNAL_FLT_CHECK(r.count() == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 5);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 6);
    D_INTERNAL_FLT_CHECK(r.indices()[0] == 0);
    D_INTERNAL_FLT_CHECK(r.indices()[1] == 1);

    return true;
}


/*
test_chain_sequential_ops
  Verifies operations compose left-to-right with correct original-index
  mapping through the intermediate sub-vectors.
  Tests the following:
  - skip_first(1) then take_first(2) over {10,20,30,40} yields {20,30} at
    original indices 1,2
*/
bool
test_chain_sequential_ops(
)
{
    filter_chain<int> chain;
    chain.add(internal::make_skip_first_op<int>(1));
    chain.add(internal::make_take_first_op<int>(2));

    std::vector<int>   in = { 10, 20, 30, 40 };
    filter_result<int> r  = chain.apply(in);

    D_INTERNAL_FLT_CHECK(r.count() == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 20);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 30);
    D_INTERNAL_FLT_CHECK(r.indices()[0] == 1);
    D_INTERNAL_FLT_CHECK(r.indices()[1] == 2);

    return true;
}


/*
test_chain_length_and_clear
  Verifies the chain bookkeeping methods.
  Tests the following:
  - length grows as ops are added; is_empty reflects emptiness
  - clear empties the chain
*/
bool
test_chain_length_and_clear(
)
{
    filter_chain<int> chain;
    D_INTERNAL_FLT_CHECK(chain.length() == 0);
    D_INTERNAL_FLT_CHECK(chain.is_empty());

    chain.add(internal::make_take_first_op<int>(1));
    chain.add(internal::make_reverse_op<int>());
    D_INTERNAL_FLT_CHECK(chain.length() == 2);
    D_INTERNAL_FLT_CHECK(!chain.is_empty());

    chain.clear();
    D_INTERNAL_FLT_CHECK(chain.length() == 0);
    D_INTERNAL_FLT_CHECK(chain.is_empty());

    return true;
}


/*
test_chain_op_factories_positional
  Verifies the positional op factories directly (take/skip first/last, nth)
  by applying each as a single-op chain.
  Tests the following:
  - take_last(2) keeps the final two; skip_last(2) keeps the leading rest
  - take_nth(2) keeps indices 0,2,4
*/
bool
test_chain_op_factories_positional(
)
{
    std::vector<int> in = { 0, 1, 2, 3, 4 };

    filter_chain<int> last2;
    last2.add(internal::make_take_last_op<int>(2));
    filter_result<int> r_last2 = last2.apply(in);
    D_INTERNAL_FLT_CHECK(r_last2.count() == 2);
    D_INTERNAL_FLT_CHECK(r_last2.elements()[0] == 3);
    D_INTERNAL_FLT_CHECK(r_last2.elements()[1] == 4);

    filter_chain<int> sl2;
    sl2.add(internal::make_skip_last_op<int>(2));
    filter_result<int> r_sl2 = sl2.apply(in);
    D_INTERNAL_FLT_CHECK(r_sl2.count() == 3);
    D_INTERNAL_FLT_CHECK(r_sl2.elements()[2] == 2);

    filter_chain<int> nth;
    nth.add(internal::make_take_nth_op<int>(2));
    filter_result<int> r_nth = nth.apply(in);
    D_INTERNAL_FLT_CHECK(r_nth.count() == 3);
    D_INTERNAL_FLT_CHECK(r_nth.indices()[0] == 0);
    D_INTERNAL_FLT_CHECK(r_nth.indices()[1] == 2);
    D_INTERNAL_FLT_CHECK(r_nth.indices()[2] == 4);

    return true;
}


/*
test_chain_op_factories_edges
  Exercises the boundary behaviour of the positional / range / slice ops.
  Tests the following:
  - take_first(0) and take_nth(0) and slice with step 0 all yield empty
  - take_first / skip_first with n past the size clamp (all / none)
  - range with start past end yields empty; range end past size clamps
*/
bool
test_chain_op_factories_edges(
)
{
    std::vector<int> in = { 1, 2, 3, 4 };

    filter_chain<int> tf0;  tf0.add(internal::make_take_first_op<int>(0));
    D_INTERNAL_FLT_CHECK(tf0.apply(in).count() == 0);

    filter_chain<int> nth0; nth0.add(internal::make_take_nth_op<int>(0));
    D_INTERNAL_FLT_CHECK(nth0.apply(in).count() == 0);

    filter_chain<int> sl0;  sl0.add(internal::make_slice_op<int>(0, 4, 0));
    D_INTERNAL_FLT_CHECK(sl0.apply(in).count() == 0);

    filter_chain<int> tfBig; tfBig.add(internal::make_take_first_op<int>(100));
    D_INTERNAL_FLT_CHECK(tfBig.apply(in).count() == 4);

    filter_chain<int> sfBig; sfBig.add(internal::make_skip_first_op<int>(100));
    D_INTERNAL_FLT_CHECK(sfBig.apply(in).count() == 0);

    filter_chain<int> badRange; badRange.add(internal::make_range_op<int>(3, 1));
    D_INTERNAL_FLT_CHECK(badRange.apply(in).count() == 0);

    filter_chain<int> bigRange; bigRange.add(internal::make_range_op<int>(1, 100));
    D_INTERNAL_FLT_CHECK(bigRange.apply(in).count() == 3);

    filter_chain<int> sliceStep; sliceStep.add(internal::make_slice_op<int>(0, 4, 2));
    filter_result<int> r_slice = sliceStep.apply(in);
    D_INTERNAL_FLT_CHECK(r_slice.count() == 2);
    D_INTERNAL_FLT_CHECK(r_slice.indices()[0] == 0);
    D_INTERNAL_FLT_CHECK(r_slice.indices()[1] == 2);

    return true;
}


/*
test_chain_op_where_and_indices
  Verifies the predicate and index op factories.
  Tests the following:
  - make_where_op keeps satisfying elements; make_where_not_op keeps the rest
  - make_indices_op keeps the requested in-range indices and drops out-of-range
*/
bool
test_chain_op_where_and_indices(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    filter_chain<int> evens;
    evens.add(internal::make_where_op<int>(
        std::function<bool(const int&)>(is_even())));
    filter_result<int> r_even = evens.apply(in);
    D_INTERNAL_FLT_CHECK(r_even.count() == 2);
    D_INTERNAL_FLT_CHECK(r_even.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r_even.elements()[1] == 4);

    filter_chain<int> odds;
    odds.add(internal::make_where_not_op<int>(
        std::function<bool(const int&)>(is_even())));
    filter_result<int> r_odd = odds.apply(in);
    D_INTERNAL_FLT_CHECK(r_odd.count() == 3);
    D_INTERNAL_FLT_CHECK(r_odd.elements()[0] == 1);

    filter_chain<int> picks;
    std::vector<std::size_t> want = { 0, 2, 100 };  // 100 is out of range
    picks.add(internal::make_indices_op<int>(want));
    filter_result<int> r_pick = picks.apply(in);
    D_INTERNAL_FLT_CHECK(r_pick.count() == 2);
    D_INTERNAL_FLT_CHECK(r_pick.indices()[0] == 0);
    D_INTERNAL_FLT_CHECK(r_pick.indices()[1] == 2);

    return true;
}


/*
test_chain_op_distinct_reverse
  Verifies the distinct and reverse op factories.
  Tests the following:
  - make_distinct_op (int equality) drops repeats, keeping first occurrences
  - make_reverse_op reverses order
  - reverse over an empty input yields empty
*/
bool
test_chain_op_distinct_reverse(
)
{
    std::vector<int> dup = { 1, 1, 2, 3, 3, 2 };

    filter_chain<int> dis;
    dis.add(internal::make_distinct_op<int>(
        std::function<bool(const int&, const int&)>(int_eq())));
    filter_result<int> r_dis = dis.apply(dup);
    D_INTERNAL_FLT_CHECK(r_dis.count() == 3);
    D_INTERNAL_FLT_CHECK(r_dis.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(r_dis.elements()[1] == 2);
    D_INTERNAL_FLT_CHECK(r_dis.elements()[2] == 3);

    std::vector<int>  seq = { 1, 2, 3 };
    filter_chain<int> rev;
    rev.add(internal::make_reverse_op<int>());
    filter_result<int> r_rev = rev.apply(seq);
    D_INTERNAL_FLT_CHECK(r_rev.count() == 3);
    D_INTERNAL_FLT_CHECK(r_rev.elements()[0] == 3);
    D_INTERNAL_FLT_CHECK(r_rev.elements()[2] == 1);

    std::vector<int>  empty;
    filter_result<int> r_emptyrev = rev.apply(empty);
    D_INTERNAL_FLT_CHECK(r_emptyrev.count() == 0);

    return true;
}


/*
run_chain_tests
  Aggregates every chain / op-factory test.
  Tests the following:
  - all empty-passthrough / single / sequential / bookkeeping / op-factory
    tests pass
*/
bool
run_chain_tests(
)
{
    return ( test_chain_empty_passthrough()        &&
             test_chain_single_op()                &&
             test_chain_sequential_ops()           &&
             test_chain_length_and_clear()         &&
             test_chain_op_factories_positional()  &&
             test_chain_op_factories_edges()       &&
             test_chain_op_where_and_indices()     &&
             test_chain_op_distinct_reverse() );
}


NS_END  // testing
NS_END  // djinterp
