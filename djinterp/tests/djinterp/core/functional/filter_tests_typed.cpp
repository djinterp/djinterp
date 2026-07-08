#include "filter_tests.hpp"

// std
#include <vector>


NS_DJINTERP
NS_TESTING


/*
test_typed_identity
  Verifies the seed typed filter selects every element.
  Tests the following:
  - make_typed_filter<int>() applied to a vector returns all elements in order
*/
bool
test_typed_identity(
)
{
    std::vector<int> in = { 1, 2, 3 };

    filter_result<int> r = make_typed_filter<int>().apply(in);

    D_INTERNAL_FLT_CHECK(r.count() == 3);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 1);
    D_INTERNAL_FLT_CHECK(r.elements()[2] == 3);

    return true;
}


/*
test_typed_where
  Verifies a typed where stage keeps satisfying elements.
  Tests the following:
  - where(is_even) over {1,2,3,4,5} yields {2,4}
*/
bool
test_typed_where(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    filter_result<int> r =
        make_typed_filter<int>().where(is_even()).apply(in);

    D_INTERNAL_FLT_CHECK(r.count() == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 4);

    return true;
}


/*
test_typed_take_skip
  Verifies the typed take_first and skip_first stages.
  Tests the following:
  - take_first(2) keeps the leading run
  - skip_first(2) drops it
  - take_first past the surviving count clamps; skip_first past it empties
*/
bool
test_typed_take_skip(
)
{
    std::vector<int> in = { 10, 20, 30, 40 };

    filter_result<int> tk =
        make_typed_filter<int>().take_first(2).apply(in);
    D_INTERNAL_FLT_CHECK(tk.count() == 2);
    D_INTERNAL_FLT_CHECK(tk.elements()[0] == 10);
    D_INTERNAL_FLT_CHECK(tk.elements()[1] == 20);

    filter_result<int> sk =
        make_typed_filter<int>().skip_first(2).apply(in);
    D_INTERNAL_FLT_CHECK(sk.count() == 2);
    D_INTERNAL_FLT_CHECK(sk.elements()[0] == 30);

    filter_result<int> tkBig =
        make_typed_filter<int>().take_first(100).apply(in);
    D_INTERNAL_FLT_CHECK(tkBig.count() == 4);

    filter_result<int> skBig =
        make_typed_filter<int>().skip_first(100).apply(in);
    D_INTERNAL_FLT_CHECK(skBig.count() == 0);

    return true;
}


/*
test_typed_composed
  Verifies multi-stage typed composition preserves left-to-right semantics,
  matching the erased builder.
  Tests the following:
  - where(is_even).take_first(2) over {1,2,3,4,6,8}: evens {2,4,6,8} then take
    2 -> {2,4}
  - skip_first(1).where(is_even) over {2,3,4,6}: skip -> {3,4,6}; evens ->
    {4,6}
*/
bool
test_typed_composed(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 6, 8 };

    filter_result<int> r = make_typed_filter<int>()
                               .where(is_even())
                               .take_first(2)
                               .apply(in);
    D_INTERNAL_FLT_CHECK(r.count() == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r.elements()[1] == 4);

    std::vector<int> in2 = { 2, 3, 4, 6 };
    filter_result<int> r2 = make_typed_filter<int>()
                                .skip_first(1)
                                .where(is_even())
                                .apply(in2);
    D_INTERNAL_FLT_CHECK(r2.count() == 2);
    D_INTERNAL_FLT_CHECK(r2.elements()[0] == 4);
    D_INTERNAL_FLT_CHECK(r2.elements()[1] == 6);

    return true;
}


/*
test_typed_to_chain
  Verifies to_chain lowers a typed filter into an erased filter_chain that
  can feed the set-theoretic combinators, and that chain() exposes the
  composed functor.
  Tests the following:
  - a typed where(is_even) lowered via to_chain applies identically on its own
  - two lowered typed chains compose correctly under filter_union
  - the lowered chain's stored functor satisfies the filter_op_fn protocol
    (compile-time, via is_filter_operation on chain_type)
*/
bool
test_typed_to_chain(
)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    filter_chain<int> evens =
        make_typed_filter<int>().where(is_even()).to_chain();
    filter_result<int> r_single = evens.apply(in);
    D_INTERNAL_FLT_CHECK(r_single.count() == 2);
    D_INTERNAL_FLT_CHECK(r_single.elements()[0] == 2);

    // feed two lowered typed chains into a union: evens {2,4} OR gt3 {4,5}
    std::vector<filter_chain<int> > chains;
    chains.push_back(make_typed_filter<int>().where(is_even()).to_chain());
    chains.push_back(make_typed_filter<int>().where(gt3()).to_chain());
    filter_result<int> r_union = filter_union(chains, in);
    D_INTERNAL_FLT_CHECK(r_union.count() == 3);
    D_INTERNAL_FLT_CHECK(r_union.elements()[0] == 2);
    D_INTERNAL_FLT_CHECK(r_union.elements()[1] == 4);
    D_INTERNAL_FLT_CHECK(r_union.elements()[2] == 5);

    // the composed typed functor models the filter_op_fn protocol
    typedef decltype(make_typed_filter<int>().where(is_even())) typed_t;
    D_INTERNAL_FLT_CHECK(
        (is_filter_operation<typed_t::chain_type, int>::value));

    return true;
}


/*
run_typed_tests
  Aggregates every typed fast-path test.
  Tests the following:
  - all identity / where / take-skip / composed / to_chain tests pass
*/
bool
run_typed_tests(
)
{
    return ( test_typed_identity()   &&
             test_typed_where()       &&
             test_typed_take_skip()   &&
             test_typed_composed()    &&
             test_typed_to_chain() );
}


NS_END  // testing
NS_END  // djinterp
