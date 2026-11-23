/*******************************************************************************
* djinterp [cpp][container]                          arena_tests_tree_traits.cpp
*
*   tree/arena_tree_traits.hpp and tree/arena_tree_concepts.hpp: the arena tree
* protocol, the root-method probes, navigation by policy, arena_tree_class,
* and the concepts that forward to them.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_tree_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/tree/arena_tree_traits.hpp>    // under test
#include <djinterp/cpp/container/arena/tree/arena_tree_concepts.hpp>  // under test
#include <djinterp/cpp/container/arena/tree/arena_tree.hpp>           // their subjects


NS_DJINTERP
NS_TESTING

/*
tests_arena_tree_traits_identity
  an arena tree is an arena with root() and has_root()
  Tests the following:
  - each alias of arena_tree is an arena tree and a rooted arena
  - a plain arena is not (no root())
  - a structural look-alike with both accessors is one; int is not
*/
bool tests_arena_tree_traits_identity()
{
    D_AN_CHECK(is_arena_tree<DArenaNaryTree<int>>::value);
    D_AN_CHECK(is_arena_tree<DArenaLcrsTree<int>>::value);
    D_AN_CHECK(is_arena_tree<DArenaBinaryTree<int>>::value);
    D_AN_CHECK(!is_arena_tree<DNodeArena<int>>::value);
    D_AN_CHECK(is_arena_tree<an_rooted_like>::value);
    D_AN_CHECK(!is_arena_tree<an_arena_like>::value);
    D_AN_CHECK(!is_arena_tree<int>::value);
    D_AN_CHECK(is_rooted_arena<DArenaParentedTree<int>>::value);
    D_AN_CHECK(!is_rooted_arena<DNodeArena<int>>::value);
    D_AN_CHECK(is_arena_tree_v<DArenaNaryTree<int>>);
    D_AN_CHECK(is_rooted_arena_v<DArenaNaryTree<int>>);

    return true;
}

/*
tests_arena_tree_traits_methods
  set_root and create_root are detected
  Tests the following:
  - an arena tree has both
  - a plain arena has neither
  - the look-alike, which has root() and has_root() only, has neither
*/
bool tests_arena_tree_traits_methods()
{
    D_AN_CHECK(has_set_root_method<DArenaNaryTree<int>>::value);
    D_AN_CHECK(has_create_root_method<DArenaNaryTree<int>>::value);
    D_AN_CHECK(!has_set_root_method<DNodeArena<int>>::value);
    D_AN_CHECK(!has_create_root_method<DNodeArena<int>>::value);
    D_AN_CHECK(!has_set_root_method<an_rooted_like>::value);
    D_AN_CHECK(!has_create_root_method<an_rooted_like>::value);
    D_AN_CHECK(has_set_root_method_v<DArenaLcrsTree<int>>);
    D_AN_CHECK(has_create_root_method_v<DArenaLcrsTree<int>>);

    return true;
}

/*
tests_arena_tree_traits_navigation
  navigation follows the policy, and a non-arena has none
  Tests the following:
  - parent-navigable: n-ary, parented and binary trees; not LCRS
  - sibling-navigable (both directions): n-ary only
  - fully navigable (all five n-ary links): n-ary only
  - binary and n-ary arenas by policy
  - int: every navigation trait is false (the zero-link stand-in policy)
*/
bool tests_arena_tree_traits_navigation()
{
    using nary = DArenaNaryTree<int>;
    using lcrs = DArenaLcrsTree<int>;
    using par  = DArenaParentedTree<int>;
    using bin  = DArenaBinaryTree<int>;

    D_AN_CHECK(is_parent_navigable<nary>::value);
    D_AN_CHECK(is_parent_navigable<par>::value);
    D_AN_CHECK(is_parent_navigable<bin>::value);
    D_AN_CHECK(!is_parent_navigable<lcrs>::value);

    D_AN_CHECK(is_sibling_navigable<nary>::value);
    D_AN_CHECK(!is_sibling_navigable<par>::value);
    D_AN_CHECK(!is_sibling_navigable<lcrs>::value);

    D_AN_CHECK(is_fully_navigable<nary>::value);
    D_AN_CHECK(!is_fully_navigable<par>::value);

    D_AN_CHECK(is_binary_arena<bin>::value);
    D_AN_CHECK(!is_binary_arena<nary>::value);
    D_AN_CHECK(is_nary_arena<lcrs>::value);
    D_AN_CHECK(!is_nary_arena<bin>::value);

    D_AN_CHECK(!is_parent_navigable<int>::value);
    D_AN_CHECK(!is_sibling_navigable<int>::value);
    D_AN_CHECK(!is_fully_navigable<int>::value);
    D_AN_CHECK(!is_binary_arena<int>::value);
    D_AN_CHECK(!is_nary_arena<int>::value);

    return true;
}

/*
tests_arena_tree_traits_class
  arena_tree_class reports each alias's capabilities
  Tests the following:
  - n-ary: an arena tree with create_root, add_child and remove_subtree, O(1)
    detach and append, 5 links, flags 31
  - LCRS: no O(1) detach or append, 2 links
  - binary: binary, not n-ary, 3 links
  - int: not an arena, not a tree, 0 links
*/
bool tests_arena_tree_traits_class()
{
    using nary = DArenaTreeClass<DArenaNaryTree<int>>;
    using lcrs = DArenaTreeClass<DArenaLcrsTree<int>>;
    using bin  = DArenaTreeClass<DArenaBinaryTree<int>>;
    using none = DArenaTreeClass<int>;

    D_AN_CHECK(nary::is_arena_type);
    D_AN_CHECK(nary::is_tree);
    D_AN_CHECK(nary::is_nary);
    D_AN_CHECK(!nary::is_binary);
    D_AN_CHECK(nary::has_create_root);
    D_AN_CHECK(nary::has_add_child);
    D_AN_CHECK(nary::has_remove_subtree);
    D_AN_CHECK(nary::fully_navigable);
    D_AN_CHECK(nary::o1_detach);
    D_AN_CHECK(nary::o1_append);
    D_AN_CHECK(nary::num_links  == 5);
    D_AN_CHECK(nary::link_flags == 31u);

    D_AN_CHECK(lcrs::is_tree);
    D_AN_CHECK(!lcrs::o1_detach);
    D_AN_CHECK(!lcrs::o1_append);
    D_AN_CHECK(!lcrs::parent_navigable);
    D_AN_CHECK(lcrs::num_links == 2);

    D_AN_CHECK(bin::is_binary);
    D_AN_CHECK(!bin::is_nary);
    D_AN_CHECK(bin::parent_navigable);
    D_AN_CHECK(bin::num_links == 3);

    D_AN_CHECK(!none::is_arena_type);
    D_AN_CHECK(!none::is_tree);
    D_AN_CHECK(!none::has_create_root);
    D_AN_CHECK(!none::has_add_child);
    D_AN_CHECK(none::num_links  == 0);
    D_AN_CHECK(none::link_flags == 0u);

    return true;
}

/*
tests_arena_tree_concepts
  the arena tree concepts
  Tests the following:
  - the root concepts (query, presence, test, assign) hold for an arena tree
    and fail for a plain arena
  - arena_tree_like, rooted_arena and non_arena_tree_like split the fixtures
  - create, add and remove-subtree concepts, and mutable_arena_tree
  - the navigation, shape and O(1) concepts follow arena_tree_class
  - classified_arena_tree is their union
*/
bool tests_arena_tree_concepts()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20
    using nary = DArenaNaryTree<int>;
    using lcrs = DArenaLcrsTree<int>;
    using bin  = DArenaBinaryTree<int>;

    D_AN_CHECK(root_query_arena<nary>);
    D_AN_CHECK(root_presence_query_arena<const nary&>);
    D_AN_CHECK(root_testable_arena<nary>);
    D_AN_CHECK(root_assignable_arena<nary>);
    D_AN_CHECK(!root_query_arena<DNodeArena<int>>);
    D_AN_CHECK(!root_assignable_arena<DNodeArena<int>>);

    D_AN_CHECK(arena_tree_like<nary>);
    D_AN_CHECK(rooted_arena<lcrs>);
    D_AN_CHECK(non_arena_tree_like<DNodeArena<int>>);
    D_AN_CHECK(non_arena_tree_like<int>);

    D_AN_CHECK(create_root_arena<nary>);
    D_AN_CHECK(child_addable_arena<nary>);
    D_AN_CHECK(subtree_removable_arena<nary>);
    D_AN_CHECK(mutable_arena_tree<nary>);
    D_AN_CHECK(!mutable_arena_tree<an_rooted_like>);

    D_AN_CHECK(parent_navigable_arena<nary>);
    D_AN_CHECK(!parent_navigable_arena<lcrs>);
    D_AN_CHECK(sibling_navigable_arena<nary>);
    D_AN_CHECK(fully_navigable_arena<nary>);
    D_AN_CHECK(binary_arena_tree<bin>);
    D_AN_CHECK(nary_arena_tree<lcrs>);
    D_AN_CHECK(o1_detachable_arena_tree<nary>);
    D_AN_CHECK(!o1_appendable_arena_tree<lcrs>);
    D_AN_CHECK(classified_arena_tree<bin>);
    D_AN_CHECK(!classified_arena_tree<int>);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
