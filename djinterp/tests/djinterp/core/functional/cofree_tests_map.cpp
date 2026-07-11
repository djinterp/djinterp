/******************************************************************************
* djinterp [test]                                        cofree_tests_map.cpp
*
*   Section II.3 (map) of the cofree.hpp suite: cofree_map, the functorial map
* that applies A -> B to every node's head while preserving the F-layers, and
* the generic functor_map that reaches it through the functor_traits
* registration.  Covers mapping all heads, structure preservation, a head-type
* change, the functor identity and composition laws, the single-node (leaf)
* case, agreement between cofree_map and functor_map, and the result type.
*
* path:      /tests/djinterp/core/functional/cofree_tests_map.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_map_doubles_all_heads
  Tests the following:
  - cofree_map applies the function to every node's head: mapping *2 over
    [3,2,1] yields [6,4,2].
*/
static bool
tests_map_doubles_all_heads()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > mapped =
        ::djinterp::cofree_map(chain, dbl());

    const std::vector<int> v = chain_to_vector(mapped);

    return ( (v.size() == 3) &&
             (v[0] == 6) && (v[1] == 4) && (v[2] == 2) );
}

/*
tests_map_preserves_structure
  Tests the following:
  - cofree_map neither adds nor drops nodes: the mapped chain has the same
    length as the original.
*/
static bool
tests_map_preserves_structure()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(4, id_head(), descend());

    const ::djinterp::cofree< opt, int > mapped =
        ::djinterp::cofree_map(chain, inc());

    return (chain_to_vector(mapped).size() == chain_to_vector(chain).size());
}

/*
tests_map_type_change
  Tests the following:
  - cofree_map may change the head type: mapping int->string over [3,2,1]
    yields ["3","2","1"] in a cofree<opt, std::string>.
*/
static bool
tests_map_type_change()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, std::string > mapped =
        ::djinterp::cofree_map(chain, show());

    const std::vector<std::string> v = chain_to_vector(mapped);

    return ( (v.size() == 3) &&
             (v[0] == "3") && (v[1] == "2") && (v[2] == "1") );
}

/*
tests_map_identity_law
  Tests the following:
  - the functor identity law: mapping the identity leaves every head unchanged.
*/
static bool
tests_map_identity_law()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > mapped =
        ::djinterp::cofree_map(chain, [](int _x) -> int { return _x; });

    return (chain_to_vector(mapped) == chain_to_vector(chain));
}

/*
tests_map_composition_law
  Tests the following:
  - the functor composition law: map(g) . map(f) == map(g . f).  Mapping *2 then
    +1 equals mapping x -> 2x+1: both give [7,5,3] over [3,2,1].
*/
static bool
tests_map_composition_law()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > stepwise =
        ::djinterp::cofree_map(::djinterp::cofree_map(chain, dbl()), inc());

    const ::djinterp::cofree< opt, int > fused =
        ::djinterp::cofree_map(chain, [](int _x) -> int { return (_x * 2) + 1; });

    return (chain_to_vector(stepwise) == chain_to_vector(fused));
}

/*
tests_functor_map_via_registration
  Tests the following:
  - the generic functor_map reaches cofree_map through the functor_traits
    registration: functor_map(chain, *2) yields [6,4,2].
*/
static bool
tests_functor_map_via_registration()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > mapped =
        ::djinterp::functor_map(chain, dbl());

    const std::vector<int> v = chain_to_vector(mapped);

    return ( (v.size() == 3) &&
             (v[0] == 6) && (v[1] == 4) && (v[2] == 2) );
}

/*
tests_functor_map_matches_cofree_map
  Tests the following:
  - functor_map and cofree_map produce the same result.
*/
static bool
tests_functor_map_matches_cofree_map()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(5, id_head(), descend());

    const std::vector<int> via_generic =
        chain_to_vector(::djinterp::functor_map(chain, inc()));
    const std::vector<int> via_direct =
        chain_to_vector(::djinterp::cofree_map(chain, inc()));

    return (via_generic == via_direct);
}

/*
tests_map_leaf
  Tests the following:
  - mapping a single-node chain (a leaf) maps just its head: *2 over [5] gives
    [10].
*/
static bool
tests_map_leaf()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t single = cf_t::make(5, cf_t::layer_type());

    const std::vector<int> v = chain_to_vector(::djinterp::cofree_map(single, dbl()));

    return ( (v.size() == 1) &&
             (v[0] == 10) );
}

/*
tests_map_result_type
  Tests the following:
  - the result type of cofree_map is cofree<F, mapped_head_type>.
*/
static bool
tests_map_result_type()
{
    return std::is_same<
        decltype(::djinterp::cofree_map(
            std::declval< const ::djinterp::cofree< opt, int >& >(),
            std::declval< show >())),
        ::djinterp::cofree< opt, std::string > >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_map_block()
{
    dt::block_spec block;

    block.name       = "II.3a cofree_map / functor_map";
    block.descriptor =
        "map every head; structure preserved; functor laws; type change";

    block.tests.push_back(dt::test_spec{
        "maps all heads",
        "*2 over [3,2,1] -> [6,4,2]",
        &tests_map_doubles_all_heads });

    block.tests.push_back(dt::test_spec{
        "preserves structure",
        "mapped chain has the same length",
        &tests_map_preserves_structure });

    block.tests.push_back(dt::test_spec{
        "head type change",
        "int->string over the whole chain",
        &tests_map_type_change });

    block.tests.push_back(dt::test_spec{
        "functor identity law",
        "map(id) leaves heads unchanged",
        &tests_map_identity_law });

    block.tests.push_back(dt::test_spec{
        "functor composition law",
        "map(g).map(f) == map(g.f)",
        &tests_map_composition_law });

    block.tests.push_back(dt::test_spec{
        "functor_map via registration",
        "generic functor_map reaches cofree_map",
        &tests_functor_map_via_registration });

    block.tests.push_back(dt::test_spec{
        "functor_map == cofree_map",
        "generic and direct agree",
        &tests_functor_map_matches_cofree_map });

    block.tests.push_back(dt::test_spec{
        "leaf",
        "mapping a single-node chain",
        &tests_map_leaf });

    block.tests.push_back(dt::test_spec{
        "result type",
        "cofree<F, mapped head type>",
        &tests_map_result_type });

    return block;
}


NS_END  // testing
NS_END  // djinterp
