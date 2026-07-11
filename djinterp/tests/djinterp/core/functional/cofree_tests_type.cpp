/******************************************************************************
* djinterp [test]                                       cofree_tests_type.cpp
*
*   Section I of the cofree.hpp suite: the cofree<F, A> type itself.  Covers the
* public default constructor (needed because duplicate yields a cofree whose
* head is itself a cofree), the make factory, the head / unwrap accessors and
* their reference-returning signatures, the value_type / layer_type member
* typedefs, and building a small multi-node chain by hand through make.
*
* path:      /tests/djinterp/core/functional/cofree_tests_type.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_default_ctor
  Tests the following:
  - a default-constructed node has a value-initialized head and an empty
    (leaf) layer.
*/
static bool
tests_default_ctor()
{
    ::djinterp::cofree< opt, int > n;

    return ( (n.head() == 0) &&
             (!n.unwrap().has) );
}

/*
tests_make_head
  Tests the following:
  - make sets the node's head.
*/
static bool
tests_make_head()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t n = cf_t::make(5, cf_t::layer_type());

    return (n.head() == 5);
}

/*
tests_make_empty_layer_is_leaf
  Tests the following:
  - a node made with an empty layer is a leaf (no children).
*/
static bool
tests_make_empty_layer_is_leaf()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t n = cf_t::make(5, cf_t::layer_type());

    return (!n.unwrap().has);
}

/*
tests_make_with_child
  Tests the following:
  - make accepts a layer holding a child node, and the resulting two-node chain
    reads back head-first as [2, 1].
*/
static bool
tests_make_with_child()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t        leaf  = cf_t::make(1, cf_t::layer_type());
    cf_t::layer_type  layer(std::make_shared<cf_t>(leaf));
    const cf_t        node  = cf_t::make(2, layer);

    const std::vector<int> v = chain_to_vector(node);

    return ( (v.size() == 2) &&
             (v[0] == 2)     &&
             (v[1] == 1) );
}

/*
tests_value_type
  Tests the following:
  - cofree<F,A>::value_type is A.
*/
static bool
tests_value_type()
{
    return std::is_same<
        ::djinterp::cofree< opt, int >::value_type,
        int >::value;
}

/*
tests_layer_type
  Tests the following:
  - cofree<F,A>::layer_type is F<shared_ptr<cofree<F,A>>>.
*/
static bool
tests_layer_type()
{
    return std::is_same<
        ::djinterp::cofree< opt, int >::layer_type,
        opt< std::shared_ptr< ::djinterp::cofree< opt, int > > > >::value;
}

/*
tests_head_returns_const_ref
  Tests the following:
  - head() returns a const reference to the stored value (not a copy).
*/
static bool
tests_head_returns_const_ref()
{
    return std::is_same<
        decltype(std::declval< const ::djinterp::cofree< opt, int >& >().head()),
        const int& >::value;
}

/*
tests_unwrap_returns_const_ref
  Tests the following:
  - unwrap() returns a const reference to the stored layer (not a copy).
*/
static bool
tests_unwrap_returns_const_ref()
{
    return std::is_same<
        decltype(std::declval< const ::djinterp::cofree< opt, int >& >().unwrap()),
        const ::djinterp::cofree< opt, int >::layer_type& >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_type_block()
{
    dt::block_spec block;

    block.name       = "I. the cofree type";
    block.descriptor =
        "default ctor, make, head/unwrap accessors, member typedefs";

    block.tests.push_back(dt::test_spec{
        "default constructor",
        "value-initialized head, empty layer",
        &tests_default_ctor });

    block.tests.push_back(dt::test_spec{
        "make: head",
        "make sets the node head",
        &tests_make_head });

    block.tests.push_back(dt::test_spec{
        "make: empty layer is leaf",
        "a node with an empty layer has no children",
        &tests_make_empty_layer_is_leaf });

    block.tests.push_back(dt::test_spec{
        "make: with child",
        "two-node chain reads back [2, 1]",
        &tests_make_with_child });

    block.tests.push_back(dt::test_spec{
        "value_type",
        "cofree<F,A>::value_type is A",
        &tests_value_type });

    block.tests.push_back(dt::test_spec{
        "layer_type",
        "cofree<F,A>::layer_type is F<shared_ptr<cofree>>",
        &tests_layer_type });

    block.tests.push_back(dt::test_spec{
        "head() returns const ref",
        "accessor yields a reference, not a copy",
        &tests_head_returns_const_ref });

    block.tests.push_back(dt::test_spec{
        "unwrap() returns const ref",
        "accessor yields a reference, not a copy",
        &tests_unwrap_returns_const_ref });

    return block;
}


NS_END  // testing
NS_END  // djinterp
