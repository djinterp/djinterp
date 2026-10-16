/*******************************************************************************
* djinterp [testing]              flat_container_traits_tests_classification.cpp
*
*   Section I - the flat verdict.  is_flat_container is container-shaped AND NOT
* hierarchical; is_strictly_flat_container adds that the value_type-chain depth is
* exactly 1.  For an ordinary flat container the two coincide; they diverge only
* where flatness rests on a TAG rather than the depth measure.
*
*   Every hierarchy signal is exercised through the complement: a container-shaped
* value_type (a string element) reads as nested and so NOT flat; a container-shaped
* node_type keeps a type hierarchical even at value_type-depth 1, and a flat tag
* cannot un-nest it; a hierarchical tag forces hierarchy at depth 1; a node-HANDLE
* node_type (std::set / std::map) is not container-shaped and so leaves the type
* flat; and a non-container (std::forward_list, which has no size(); a bare int) is
* not flat at all.  The lone divergence: tagged_flat_str is flat by its tag yet has
* depth 2, so is_flat is true while is_strictly_flat is false.
*
*
* path:      /tests/djinterp/core/container/traits/flat_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "flat_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_flat_container
//   container-shaped and not hierarchical.
bool
tests_is_flat_container()
{
    bool _fc_ok = true;

    // ordinary flat containers (depth 1, no nesting signal)
    D_FC_CHECK( (is_flat_container<std::vector<int>>::value) );
    D_FC_CHECK( (is_flat_container<std::string>::value) );          // char element -> depth 1
    D_FC_CHECK( (is_flat_container<std::array<int, 4>>::value) );
    D_FC_CHECK( (is_flat_container<std::set<int>>::value) );        // node-handle node_type -> still flat
    D_FC_CHECK( (is_flat_container<std::map<int, int>>::value) );   // pair element is not container-shaped
    D_FC_CHECK( (is_flat_container<flat_leaf>::value) );
    // flat by TAG: the tag suppresses the depth heuristic on a container-shaped element
    D_FC_CHECK( (is_flat_container<tagged_flat_str>::value) );

    // nested by the depth heuristic -> not flat
    D_FC_CHECK( (!is_flat_container<std::vector<std::vector<int>>>::value) );
    D_FC_CHECK( (!is_flat_container<std::vector<std::string>>::value) );  // string element reads as nested
    D_FC_CHECK( (!is_flat_container<nested_box>::value) );
    // strong node_type signal -> hierarchical -> not flat (a flat tag cannot override it)
    D_FC_CHECK( (!is_flat_container<node_box>::value) );
    D_FC_CHECK( (!is_flat_container<node_box_flat_tagged>::value) );
    // hierarchical tag -> not flat even at depth 1
    D_FC_CHECK( (!is_flat_container<hier_tagged>::value) );
    // non-containers are not flat
    D_FC_CHECK( (!is_flat_container<std::forward_list<int>>::value) );  // no size() -> not container-shaped
    D_FC_CHECK( (!is_flat_container<int>::value) );

    return _fc_ok;
}


// tests_is_strictly_flat_container
//   flat AND value_type-chain depth exactly 1.
bool
tests_is_strictly_flat_container()
{
    bool _fc_ok = true;

    // flat with depth 1 -> strictly flat
    D_FC_CHECK( (is_strictly_flat_container<std::vector<int>>::value) );
    D_FC_CHECK( (is_strictly_flat_container<std::string>::value) );
    D_FC_CHECK( (is_strictly_flat_container<std::array<int, 4>>::value) );
    D_FC_CHECK( (is_strictly_flat_container<std::set<int>>::value) );
    D_FC_CHECK( (is_strictly_flat_container<std::map<int, int>>::value) );
    D_FC_CHECK( (is_strictly_flat_container<flat_leaf>::value) );

    // THE divergence: flat by tag, but the depth measure still reads 2 -> NOT strictly flat
    D_FC_CHECK( (is_flat_container<tagged_flat_str>::value) );          // flat ...
    D_FC_CHECK( (!is_strictly_flat_container<tagged_flat_str>::value) ); // ... but not strictly

    // not flat at all -> not strictly flat
    D_FC_CHECK( (!is_strictly_flat_container<std::vector<std::vector<int>>>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<std::vector<std::string>>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<nested_box>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<node_box>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<hier_tagged>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<std::forward_list<int>>::value) );
    D_FC_CHECK( (!is_strictly_flat_container<int>::value) );

    return _fc_ok;
}


NS_END  // testing
NS_END  // djinterp
