/******************************************************************************
* djinterp [test]                                     cofree_tests_extend.cpp
*
*   Section II.3 (extend) of the cofree.hpp suite, plus the comonad face of the
* registration: cofree_extend re-decorates every node with a function of that
* node's WHOLE sub-tree, and the generic extract / extend / duplicate reach the
* head / co-bind through comonad_traits.  Covers extract, a context-dependent
* co-bind (suffix sum), an extend that changes the head type (suffix length),
* structure preservation, agreement between extend and cofree_extend, and the
* three comonad laws: extract . extend(f) == f, extend(extract) == id, and
* extract . duplicate == id (with duplicate's shape verified by mapping).
*
* path:      /tests/djinterp/core/functional/cofree_tests_extend.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_extract_head
  Tests the following:
  - extract reads the head of the focused node (via comonad_traits): extract of
    [3,2,1] is 3.
*/
static bool
tests_extract_head()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    return (::djinterp::extract(chain) == 3);
}

/*
tests_extract_leaf
  Tests the following:
  - extract of a single-node chain is that node's head.
*/
static bool
tests_extract_leaf()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t single = cf_t::make(5, cf_t::layer_type());

    return (::djinterp::extract(single) == 5);
}

/*
tests_extend_suffix_sum
  Tests the following:
  - cofree_extend re-decorates each node with a function of its whole sub-tree:
    the suffix sum over [3,2,1] gives [6,3,1] (3+2+1, 2+1, 1).
*/
static bool
tests_extend_suffix_sum()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > decorated =
        ::djinterp::extend(chain, suffix_sum());

    const std::vector<int> v = chain_to_vector(decorated);

    return ( (v.size() == 3) &&
             (v[0] == 6) && (v[1] == 3) && (v[2] == 1) );
}

/*
tests_extend_head_is_identity
  Tests the following:
  - the comonad LEFT IDENTITY law: extend(extract) == id.  Re-decorating every
    node with its own head leaves the head values unchanged.
*/
static bool
tests_extend_head_is_identity()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > decorated =
        ::djinterp::extend(chain, head_of_node());

    return (chain_to_vector(decorated) == chain_to_vector(chain));
}

/*
tests_extend_type_change
  Tests the following:
  - extend may change the head type: decorating with the sub-tree length gives a
    cofree<opt, size_t> holding [3,2,1] over [3,2,1].
*/
static bool
tests_extend_type_change()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, std::size_t > decorated =
        ::djinterp::extend(chain, suffix_len());

    const std::vector<std::size_t> v = chain_to_vector(decorated);

    return ( (v.size() == 3) &&
             (v[0] == static_cast<std::size_t>(3)) &&
             (v[1] == static_cast<std::size_t>(2)) &&
             (v[2] == static_cast<std::size_t>(1)) );
}

/*
tests_cofree_extend_direct
  Tests the following:
  - cofree_extend, called directly (not through the generic extend), produces
    the same suffix-sum decoration.
*/
static bool
tests_cofree_extend_direct()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const std::vector<int> v =
        chain_to_vector(::djinterp::cofree_extend(chain, suffix_sum()));

    return ( (v.size() == 3) &&
             (v[0] == 6) && (v[1] == 3) && (v[2] == 1) );
}

/*
tests_extend_matches_cofree_extend
  Tests the following:
  - the generic extend and cofree_extend agree.
*/
static bool
tests_extend_matches_cofree_extend()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(4, id_head(), descend());

    const std::vector<int> via_generic =
        chain_to_vector(::djinterp::extend(chain, suffix_sum()));
    const std::vector<int> via_direct =
        chain_to_vector(::djinterp::cofree_extend(chain, suffix_sum()));

    return (via_generic == via_direct);
}

/*
tests_extend_preserves_structure
  Tests the following:
  - extend neither adds nor drops nodes.
*/
static bool
tests_extend_preserves_structure()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(4, id_head(), descend());

    const ::djinterp::cofree< opt, int > decorated =
        ::djinterp::extend(chain, suffix_sum());

    return (chain_to_vector(decorated).size() == chain_to_vector(chain).size());
}

/*
tests_extract_extend_law
  Tests the following:
  - the comonad law extract . extend(f) == f: the head of an extended tree is f
    applied to the whole original tree.  suffix_sum([3,2,1]) is 6.
*/
static bool
tests_extract_extend_law()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const int head_of_extended =
        ::djinterp::extract(::djinterp::extend(chain, suffix_sum()));
    const int f_of_whole = suffix_sum()(chain);

    return ( (head_of_extended == f_of_whole) &&
             (head_of_extended == 6) );
}

/*
tests_duplicate_extract_law
  Tests the following:
  - the comonad law extract . duplicate == id: the head of the duplicated tree
    is the original tree (verified through its contents).
*/
static bool
tests_duplicate_extract_law()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::cofree< opt, int > recovered =
        ::djinterp::extract(::djinterp::duplicate(chain));

    return (chain_to_vector(recovered) == chain_to_vector(chain));
}

/*
tests_duplicate_structure
  Tests the following:
  - duplicate labels every node with its own sub-tree: mapping each label back
    to its head recovers the original head sequence [3,2,1].
*/
static bool
tests_duplicate_structure()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    // duplicate -> cofree<opt, cofree<opt,int>>; map each sub-tree to its head
    const ::djinterp::cofree< opt, int > heads =
        ::djinterp::functor_map(::djinterp::duplicate(chain), head_of_node());

    const std::vector<int> v = chain_to_vector(heads);

    return ( (v.size() == 3) &&
             (v[0] == 3) && (v[1] == 2) && (v[2] == 1) );
}

/*
tests_duplicate_result_type
  Tests the following:
  - the result type of duplicate is cofree<F, cofree<F, A>>.
*/
static bool
tests_duplicate_result_type()
{
    return std::is_same<
        decltype(::djinterp::duplicate(
            std::declval< const ::djinterp::cofree< opt, int >& >())),
        ::djinterp::cofree< opt, ::djinterp::cofree< opt, int > > >::value;
}

/*
tests_extend_result_type
  Tests the following:
  - the result type of extend is cofree<F, decorated_type>.
*/
static bool
tests_extend_result_type()
{
    return std::is_same<
        decltype(::djinterp::extend(
            std::declval< const ::djinterp::cofree< opt, int >& >(),
            std::declval< suffix_len >())),
        ::djinterp::cofree< opt, std::size_t > >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_extend_block()
{
    dt::block_spec block;

    block.name       = "II.3b cofree_extend / extract / duplicate";
    block.descriptor =
        "co-bind over whole sub-trees; extract; duplicate; comonad laws";

    block.tests.push_back(dt::test_spec{
        "extract: head",
        "extract reads the focused node's head",
        &tests_extract_head });

    block.tests.push_back(dt::test_spec{
        "extract: leaf",
        "extract of a single-node chain",
        &tests_extract_leaf });

    block.tests.push_back(dt::test_spec{
        "extend: suffix sum",
        "each node decorated by its whole sub-tree",
        &tests_extend_suffix_sum });

    block.tests.push_back(dt::test_spec{
        "extend(extract) == id",
        "comonad left identity",
        &tests_extend_head_is_identity });

    block.tests.push_back(dt::test_spec{
        "extend: head type change",
        "decorate with sub-tree length (size_t)",
        &tests_extend_type_change });

    block.tests.push_back(dt::test_spec{
        "cofree_extend direct",
        "direct call matches the decoration",
        &tests_cofree_extend_direct });

    block.tests.push_back(dt::test_spec{
        "extend == cofree_extend",
        "generic and direct agree",
        &tests_extend_matches_cofree_extend });

    block.tests.push_back(dt::test_spec{
        "extend preserves structure",
        "same node count after extend",
        &tests_extend_preserves_structure });

    block.tests.push_back(dt::test_spec{
        "extract . extend(f) == f",
        "head of extended tree is f(whole)",
        &tests_extract_extend_law });

    block.tests.push_back(dt::test_spec{
        "extract . duplicate == id",
        "duplicate's head is the original tree",
        &tests_duplicate_extract_law });

    block.tests.push_back(dt::test_spec{
        "duplicate structure",
        "each label maps back to its head",
        &tests_duplicate_structure });

    block.tests.push_back(dt::test_spec{
        "duplicate result type",
        "cofree<F, cofree<F, A>>",
        &tests_duplicate_result_type });

    block.tests.push_back(dt::test_spec{
        "extend result type",
        "cofree<F, decorated type>",
        &tests_extend_result_type });

    return block;
}


NS_END  // testing
NS_END  // djinterp
