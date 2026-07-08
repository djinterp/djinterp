#include "filter_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


// build_where_chain
//   helper (file-local): builds a single-op chain from a predicate functor.
// Keeps the combinator tests readable.
template<typename _Pred>
static filter_chain<int>
build_where_chain(
    _Pred _pred
)
{
    return filter_builder<int>::build().where(_pred).build_chain();
}


/*
test_combinators_union
  Verifies filter_union includes an element if it passes ANY chain, in input
  order, with original indices.
  Tests the following:
  - even-chain {2,4} unioned with gt3-chain {4,5} over {1,2,3,4,5} yields
    {2,4,5} at indices {1,3,4}
*/
bool
test_combinators_union(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    std::vector<filter_chain<int> > chains;
    chains.push_back(build_where_chain(is_even()));
    chains.push_back(build_where_chain(gt3()));

    filter_result<int> r = filter_union(chains, in);

    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 4);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 5);
    D_INTERNAL_FLT_CHECK(r.indices()[0] == 1);
    D_INTERNAL_FLT_CHECK(r.indices()[2] == 4);

    return true;
}


/*
test_combinators_union_empty_list
  Verifies a union over no chains includes nothing.
  Tests the following:
  - filter_union with an empty chain list yields an empty result
*/
bool
test_combinators_union_empty_list(
)
{
    std::vector<int>                in = { 1, 2, 3 };
    std::vector<filter_chain<int> > none;

    filter_result<int> r = filter_union(none, in);
    D_INTERNAL_FLT_CHECK(r.count() == 0);
    D_INTERNAL_FLT_CHECK(r.empty());

    return true;
}


/*
test_combinators_intersection
  Verifies filter_intersection includes an element only if it passes EVERY
  chain.
  Tests the following:
  - even-chain intersected with gt3-chain over {1,2,3,4,5} yields {4}
    (the only element both even and > 3) at index 3
*/
bool
test_combinators_intersection(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    std::vector<filter_chain<int> > chains;
    chains.push_back(build_where_chain(is_even()));
    chains.push_back(build_where_chain(gt3()));

    filter_result<int> r = filter_intersection(chains, in);

    D_INTERNAL_FLT_CHECK(r.count() == 1);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 4);
    D_INTERNAL_FLT_CHECK(r.indices()[0] == 3);

    return true;
}


/*
test_combinators_intersection_zero_chains
  Documents and pins the zero-chains edge: with no chains, every element's
  hit count (0) equals the chain count (0), so all elements are included.
  Tests the following:
  - filter_intersection with an empty chain list includes all elements
*/
bool
test_combinators_intersection_zero_chains(
)
{
    std::vector<int>                in = { 7, 8, 9 };
    std::vector<filter_chain<int> > none;

    filter_result<int> r = filter_intersection(none, in);
    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 7);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 9);

    return true;
}


/*
test_combinators_difference
  Verifies filter_difference includes elements passing the include chain but
  not the exclude chain.
  Tests the following:
  - include = all (empty chain), exclude = even, over {1,2,3,4,5} yields the
    odds {1,3,5}
*/
bool
test_combinators_difference(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    filter_chain<int> include;                     // empty -> keeps all
    filter_chain<int> exclude = build_where_chain(is_even());

    filter_result<int> r = filter_difference(include, exclude, in);

    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 5);

    return true;
}


/*
run_combinators_tests
  Aggregates every combinator test.
  Tests the following:
  - all union / intersection / difference tests pass, including the empty and
    zero-chains edges
*/
bool
run_combinators_tests(
)
{
    return ( test_combinators_union()                  &&
             test_combinators_union_empty_list()       &&
             test_combinators_intersection()           &&
             test_combinators_intersection_zero_chains() &&
             test_combinators_difference() );
}


NS_END  // testing
NS_END  // djinterp
