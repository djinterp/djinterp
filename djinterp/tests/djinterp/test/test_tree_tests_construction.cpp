// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_default_construct_is_empty
  Verifies the default constructor yields an empty tree: no nodes, an empty
  kind set, and a null (not-yet-created) conjunctive root.
  Tests the following:
  - test_tree() default constructor
  - size() / empty() on a fresh tree
  - root() is nullptr before the first add_root
  - kinds() starts empty
*/
bool
tests_tree_default_construct_is_empty()
{
    test_tree<basic_test> t;

    return ( (t.size()         == 0)       &&
             (t.empty()        == true)    &&
             (t.root()         == nullptr) &&
             (t.kinds().size() == 0) );
}

/*
tests_tree_construct_from_kinds
  Verifies the kind-container constructor stores the supplied kinds and that
  they actually govern subsequent rank-checked insertion.
  Tests the following:
  - explicit test_tree(kind_container_type) (move)
  - kinds() reflects the moved-in records
  - forest is still empty after construction
  - the stored kinds drive append_child (interior parent admits in-rank child)
*/
bool
tests_tree_construct_from_kinds()
{
    std::vector<test_kind> ks;
    ks.push_back(make_interior_kind(10, 5));
    ks.push_back(make_leaf_kind(20, 1));

    test_tree<basic_test> t(std::move(ks));

    const bool stored =
        ( (t.kinds().size() == 2) &&
          (t.empty()        == true) );

    // the kinds must be in force: interior parent (id 10, rank 5) admits a
    // rank-1 child (id 20)
    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(10, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(20, true));

    const bool in_force =
        ( (p != nullptr) &&
          (c != nullptr) );

    return ( stored &&
             in_force );
}

/*
tests_tree_construct_from_kinds_and_forest
  Verifies the (kinds, forest) constructor adopts a pre-built backing forest
  wholesale.
  Tests the following:
  - test_tree(kind_container_type, underlying_container_type) (move both)
  - kinds() and the forest are both taken over
  - size() / root() reflect the supplied forest (no extra conjunctive root is
    fabricated for an already-rooted forest)
  - the run surface sees the adopted nodes
*/
bool
tests_tree_construct_from_kinds_and_forest()
{
    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));
    f.append_child(f.root(), make_status_test(1, test_status::passed));

    std::vector<test_kind> ks;
    ks.push_back(make_interior_kind(0, 9));

    test_tree<basic_test> t(std::move(ks), std::move(f));

    return ( (t.kinds().size() == 1)       &&
             (t.size()         == 2)       &&
             (t.root()         != nullptr) &&
             (t.count_passed() == 2) );
}

/*
tests_tree_type_aliases
  Verifies the public type aliases resolve to the expected types for the
  default backing / kind container.
  Tests the following:
  - value_type, underlying_container_type, kind_container_type, size_type
  - node_type forwards from the backing
*/
bool
tests_tree_type_aliases()
{
    using tree = test_tree<basic_test>;

    static_assert(
        std::is_same<tree::value_type, basic_test>::value,
        "value_type should be the element type");

    static_assert(
        std::is_same<tree::underlying_container_type,
                     nary_tree<basic_test>>::value,
        "underlying_container_type should default to nary_tree<_Element>");

    static_assert(
        std::is_same<tree::kind_container_type,
                     std::vector<test_kind>>::value,
        "kind_container_type should default to std::vector<test_kind>");

    static_assert(
        std::is_same<tree::size_type, std::size_t>::value,
        "size_type should be std::size_t");

    static_assert(
        std::is_same<tree::node_type,
                     nary_tree<basic_test>::node_type>::value,
        "node_type should forward from the backing");

    return true;
}

/*
tests_tree_kinds_accessor
  Verifies both overloads of kinds() refer to the one stored container.
  Tests the following:
  - kinds() (non-const) returns a mutable reference (mutation sticks)
  - kinds() const observes the same container
*/
bool
tests_tree_kinds_accessor()
{
    test_tree<basic_test> t;

    // mutate through the non-const accessor
    t.kinds().push_back(make_leaf_kind(7, 2));

    const test_tree<basic_test>& ct = t;

    return ( (t.kinds().size()  == 1) &&
             (ct.kinds().size() == 1) );
}

/*
tests_tree_underlying_accessor
  Verifies both overloads of underlying() expose the backing forest.
  Tests the following:
  - underlying() (non-const) reflects the current node count
  - underlying() const observes the same forest
*/
bool
tests_tree_underlying_accessor()
{
    test_tree<basic_test> t;
    t.add_root(make_test(1, true));

    const test_tree<basic_test>& ct = t;

    // one root child plus the implied conjunctive root
    return ( (t.underlying().size()  == 2) &&
             (ct.underlying().size() == 2) );
}


NS_END  // testing
NS_END  // djinterp
