/******************************************************************************
* djinterp [test]                                     cofree_tests_unfold.cpp
*
*   Section II.3 (unfold) of the cofree.hpp suite: unfold_cofree, which
* coiteratively grows a cofree from a seed -- head_fn : S -> A gives each node's
* value, layer_fn : S -> F<S> gives its children's seeds, terminating when the
* F-layer is empty.  Covers building a descending chain, the single-node case
* (layer_fn empty immediately), longer chains, the distinct roles of head_fn
* (the value that lands in each node) and layer_fn (which controls length /
* termination), a head-type change, agreement with a hand-built chain, and the
* result type.
*
* path:      /tests/djinterp/core/functional/cofree_tests_unfold.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_unfold_builds_chain
  Tests the following:
  - unfold grows a descending chain: seed 3 with head=seed and child=seed-1 (to
    1) yields [3,2,1].
*/
static bool
tests_unfold_builds_chain()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const std::vector<int> v = chain_to_vector(chain);

    return ( (v.size() == 3) &&
             (v[0] == 3) && (v[1] == 2) && (v[2] == 1) );
}

/*
tests_unfold_leaf
  Tests the following:
  - when layer_fn yields an empty layer immediately, unfold builds a single
    node: seed 1 (descend gives nothing) yields [1].
*/
static bool
tests_unfold_leaf()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(1, id_head(), descend());

    const std::vector<int> v = chain_to_vector(chain);

    return ( (v.size() == 1) &&
             (v[0] == 1) );
}

/*
tests_unfold_longer
  Tests the following:
  - a longer seed grows a longer chain: seed 5 yields [5,4,3,2,1].
*/
static bool
tests_unfold_longer()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(5, id_head(), descend());

    const std::vector<int> v = chain_to_vector(chain);

    return ( (v.size() == 5) &&
             (v[0] == 5) && (v[1] == 4) && (v[2] == 3) &&
             (v[3] == 2) && (v[4] == 1) );
}

/*
tests_unfold_head_fn_role
  Tests the following:
  - head_fn determines each node's value: seed 3 with head = seed*10 yields
    [30,20,10] (the chain shape follows the seeds, the values follow head_fn).
*/
static bool
tests_unfold_head_fn_role()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, times_ten(), descend());

    const std::vector<int> v = chain_to_vector(chain);

    return ( (v.size() == 3) &&
             (v[0] == 30) && (v[1] == 20) && (v[2] == 10) );
}

/*
tests_unfold_head_type
  Tests the following:
  - the node type follows head_fn's result: a string-valued head_fn yields a
    cofree<opt, std::string> holding ["3","2","1"].
*/
static bool
tests_unfold_head_type()
{
    const ::djinterp::cofree< opt, std::string > chain =
        ::djinterp::unfold_cofree< opt >(3, show(), descend());

    const std::vector<std::string> v = chain_to_vector(chain);

    return ( (v.size() == 3) &&
             (v[0] == "3") && (v[1] == "2") && (v[2] == "1") );
}

/*
tests_unfold_length_follows_seed
  Tests the following:
  - layer_fn controls length / termination: seed 2 yields a two-node chain,
    seed 3 a three-node chain.
*/
static bool
tests_unfold_length_follows_seed()
{
    const std::size_t len2 =
        chain_to_vector(::djinterp::unfold_cofree< opt >(2, id_head(), descend())).size();
    const std::size_t len3 =
        chain_to_vector(::djinterp::unfold_cofree< opt >(3, id_head(), descend())).size();

    return ( (len2 == static_cast<std::size_t>(2)) &&
             (len3 == static_cast<std::size_t>(3)) );
}

/*
tests_unfold_matches_manual
  Tests the following:
  - unfold builds the same structure as a hand-assembled chain: unfold from seed
    2 equals make(2, just(make(1, nothing))) by contents.
*/
static bool
tests_unfold_matches_manual()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t unfolded =
        ::djinterp::unfold_cofree< opt >(2, id_head(), descend());

    const cf_t       leaf = cf_t::make(1, cf_t::layer_type());
    cf_t::layer_type layer(std::make_shared<cf_t>(leaf));
    const cf_t       manual = cf_t::make(2, layer);

    return (chain_to_vector(unfolded) == chain_to_vector(manual));
}

/*
tests_unfold_result_type
  Tests the following:
  - the result type of unfold is cofree<F, head_fn result type>.
*/
static bool
tests_unfold_result_type()
{
    return std::is_same<
        decltype(::djinterp::unfold_cofree< opt >(
            std::declval< const int& >(),
            std::declval< id_head >(),
            std::declval< descend >())),
        ::djinterp::cofree< opt, int > >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_unfold_block()
{
    dt::block_spec block;

    block.name       = "II.3c unfold_cofree";
    block.descriptor =
        "coiterative build: head_fn / layer_fn roles, termination, type";

    block.tests.push_back(dt::test_spec{
        "builds a chain",
        "seed 3 -> [3,2,1]",
        &tests_unfold_builds_chain });

    block.tests.push_back(dt::test_spec{
        "leaf (empty layer)",
        "seed 1 -> [1]",
        &tests_unfold_leaf });

    block.tests.push_back(dt::test_spec{
        "longer chain",
        "seed 5 -> [5,4,3,2,1]",
        &tests_unfold_longer });

    block.tests.push_back(dt::test_spec{
        "head_fn role",
        "head = seed*10 -> [30,20,10]",
        &tests_unfold_head_fn_role });

    block.tests.push_back(dt::test_spec{
        "head type follows head_fn",
        "string head_fn -> cofree<opt,string>",
        &tests_unfold_head_type });

    block.tests.push_back(dt::test_spec{
        "length follows seed",
        "layer_fn controls termination",
        &tests_unfold_length_follows_seed });

    block.tests.push_back(dt::test_spec{
        "matches manual build",
        "unfold == hand-assembled chain",
        &tests_unfold_matches_manual });

    block.tests.push_back(dt::test_spec{
        "result type",
        "cofree<F, head_fn result>",
        &tests_unfold_result_type });

    return block;
}


NS_END  // testing
NS_END  // djinterp
