/*******************************************************************************
* djinterp [testing]               hierarchical_container_traits_tests_depth.cpp
*
*   Section II - the value_type-chain nesting depth: 1 for a flat container, 2 for
* vector<vector<T>>, 0 for a non-container.  It measures the REGULAR (uniform)
* nesting visible in the value_type chain; composite hierarchy advertised only
* through node_type is NOT counted here (it is caught by is_hierarchical_container).
* _MaxDepth caps the recursion, and max_depth_of is a plain alias of container_depth.
*
*   Two things pinned: the depth is blind to the node_type - node_box reads depth 1
* even though it is hierarchical - and a flat tag does NOT change the depth (only the
* verdict), so tagged_flat_str still reads depth 2.  The cap truncates the count:
* vec3 is depth 3 uncapped but reads 2, 1, 0 as the cap tightens.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests_depth.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "hierarchical_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_depth
//   the uniform value_type-chain depth.
bool
tests_container_depth()
{
    bool _hc_ok = true;

    // depth 1: flat containers (a char/int leaf is not a container)
    D_HC_CHECK( (container_depth<vec1>::value == 1) );
    D_HC_CHECK( (container_depth<std::string>::value == 1) );
    D_HC_CHECK( (container_depth<flat_leaf>::value == 1) );
    D_HC_CHECK( (container_depth<strlike>::value == 1) );

    // depth 2 / 3: nested value_types
    D_HC_CHECK( (container_depth<vec2>::value == 2) );
    D_HC_CHECK( (container_depth<vec3>::value == 3) );
    D_HC_CHECK( (container_depth<nested_box>::value == 2) );
    D_HC_CHECK( (container_depth<std::vector<std::string>>::value == 2) );  // string element -> nested level

    // depth 0: non-containers
    D_HC_CHECK( (container_depth<int>::value == 0) );
    D_HC_CHECK( (container_depth<std::forward_list<int>>::value == 0) );    // not container-shaped

    // the depth is blind to node_type and to the flat tag
    D_HC_CHECK( (container_depth<node_box>::value == 1) );        // hierarchical via node_type, yet depth 1
    D_HC_CHECK( (container_depth<tagged_flat_str>::value == 2) ); // flat by tag, yet depth 2

    return _hc_ok;
}


// tests_container_depth_maxdepth_cap
//   the _MaxDepth argument truncates the recursion.
bool
tests_container_depth_maxdepth_cap()
{
    bool _hc_ok = true;

    // vec3 is depth 3; the cap tightens the reading
    D_HC_CHECK( (container_depth<vec3, 32>::value == 3) );
    D_HC_CHECK( (container_depth<vec3, 3>::value  == 3) );
    D_HC_CHECK( (container_depth<vec3, 2>::value  == 2) );   // capped
    D_HC_CHECK( (container_depth<vec3, 1>::value  == 1) );   // capped
    D_HC_CHECK( (container_depth<vec3, 0>::value  == 0) );   // capped to nothing

    // vec2 capped at 1
    D_HC_CHECK( (container_depth<vec2, 1>::value == 1) );
    D_HC_CHECK( (container_depth<vec2, 32>::value == 2) );

    return _hc_ok;
}


// tests_max_depth_of
//   an alias of container_depth (same values, including the cap).
bool
tests_max_depth_of()
{
    bool _hc_ok = true;

    D_HC_CHECK( (max_depth_of<vec1>::value == 1) );
    D_HC_CHECK( (max_depth_of<vec2>::value == 2) );
    D_HC_CHECK( (max_depth_of<vec3>::value == 3) );
    D_HC_CHECK( (max_depth_of<int>::value == 0) );

    // agrees with container_depth, and honours the cap
    D_HC_CHECK( (max_depth_of<vec3>::value == container_depth<vec3>::value) );
    D_HC_CHECK( (max_depth_of<vec3, 2>::value == 2) );

    return _hc_ok;
}


NS_END  // testing
NS_END  // djinterp
