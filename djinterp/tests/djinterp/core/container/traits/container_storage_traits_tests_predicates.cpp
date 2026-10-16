/*******************************************************************************
* djinterp [testing]               container_storage_traits_tests_predicates.cpp
*
*   Section III - the boolean projections of storage_duration_of.  The exclusive
* predicates (is_static / is_dynamic / is_hybrid) ask whether the siting is
* PURELY that value; the component predicates (has_static_component /
* has_dynamic_component) ask whether the siting INCLUDES that part.  They diverge
* precisely on hybrid, which carries BOTH components, and on unknown, which
* carries NEITHER - so each is exercised across static / dynamic / hybrid /
* unknown representatives.
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests_predicates.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_storage_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_static_storage_container
//   true iff the siting is static exclusively.
bool
tests_is_static_storage_container()
{
    bool _stg_ok = true;

    D_STG_CHECK( (is_static_storage_container<std::array<int, 4>>::value) );
    D_STG_CHECK( (is_static_storage_container<std::pair<int, int>>::value) );
    D_STG_CHECK( (is_static_storage_container<std::tuple<int, char>>::value) );
    D_STG_CHECK( (is_static_storage_container<extent_only>::value) );
    D_STG_CHECK( (is_static_storage_container<optin_static_no_size>::value) );

    D_STG_CHECK( (!is_static_storage_container<std::vector<int>>::value) );  // dynamic
    D_STG_CHECK( (!is_static_storage_container<hybrid_shape>::value) );      // hybrid (not exclusive)
    D_STG_CHECK( (!is_static_storage_container<int>::value) );               // unknown

    return _stg_ok;
}


// tests_is_dynamic_storage_container
//   true iff the siting is dynamic exclusively.
bool
tests_is_dynamic_storage_container()
{
    bool _stg_ok = true;

    D_STG_CHECK( (is_dynamic_storage_container<std::vector<int>>::value) );
    D_STG_CHECK( (is_dynamic_storage_container<std::map<int, int>>::value) );
    D_STG_CHECK( (is_dynamic_storage_container<reserve_only>::value) );
    D_STG_CHECK( (is_dynamic_storage_container<optin_dynamic_override>::value) );

    D_STG_CHECK( (!is_dynamic_storage_container<std::array<int, 4>>::value) );  // static
    D_STG_CHECK( (!is_dynamic_storage_container<hybrid_shape>::value) );        // hybrid (not exclusive)
    D_STG_CHECK( (!is_dynamic_storage_container<int>::value) );                 // unknown

    return _stg_ok;
}


// tests_is_hybrid_storage_container
//   true iff the siting spans both (small-storage / SBO).
bool
tests_is_hybrid_storage_container()
{
    bool _stg_ok = true;

    D_STG_CHECK( (is_hybrid_storage_container<hybrid_shape>::value) );   // synthesised from both signals
    D_STG_CHECK( (is_hybrid_storage_container<optin_hybrid>::value) );   // via opt-in

    D_STG_CHECK( (!is_hybrid_storage_container<std::array<int, 4>>::value) );  // static
    D_STG_CHECK( (!is_hybrid_storage_container<std::vector<int>>::value) );    // dynamic
    D_STG_CHECK( (!is_hybrid_storage_container<int>::value) );                 // unknown

    return _stg_ok;
}


// tests_has_static_storage_component_container
//   true iff the siting includes a static part (static OR hybrid).
bool
tests_has_static_storage_component_container()
{
    bool _stg_ok = true;

    D_STG_CHECK( (has_static_storage_component_container<std::array<int, 4>>::value) );  // static
    D_STG_CHECK( (has_static_storage_component_container<hybrid_shape>::value) );        // hybrid
    D_STG_CHECK( (has_static_storage_component_container<optin_hybrid>::value) );        // hybrid
    D_STG_CHECK( (has_static_storage_component_container<optin_static_no_size>::value) );// static

    D_STG_CHECK( (!has_static_storage_component_container<std::vector<int>>::value) );   // dynamic
    D_STG_CHECK( (!has_static_storage_component_container<int>::value) );                // unknown

    return _stg_ok;
}


// tests_has_dynamic_storage_component_container
//   true iff the siting includes a dynamic part (dynamic OR hybrid).
bool
tests_has_dynamic_storage_component_container()
{
    bool _stg_ok = true;

    D_STG_CHECK( (has_dynamic_storage_component_container<std::vector<int>>::value) );  // dynamic
    D_STG_CHECK( (has_dynamic_storage_component_container<reserve_only>::value) );      // dynamic
    D_STG_CHECK( (has_dynamic_storage_component_container<hybrid_shape>::value) );      // hybrid
    D_STG_CHECK( (has_dynamic_storage_component_container<optin_hybrid>::value) );      // hybrid

    D_STG_CHECK( (!has_dynamic_storage_component_container<std::array<int, 4>>::value) ); // static
    D_STG_CHECK( (!has_dynamic_storage_component_container<int>::value) );               // unknown

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
