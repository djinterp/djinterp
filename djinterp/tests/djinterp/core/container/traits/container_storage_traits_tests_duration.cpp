/*******************************************************************************
* djinterp [testing]                 container_storage_traits_tests_duration.cpp
*
*   Section II - storage_duration_of, the siting classifier, across all four
* sitings and both resolution paths.  Shape inference feeds make_storage_duration
* (static-only -> static, dynamic-only -> dynamic, BOTH -> hybrid, NEITHER ->
* unknown); an opt-in storage_duration_category overrides it entirely.
*
*   The edges: no standard container raises both signals, so hybrid is
* SYNTHESISED only by a custom type (hybrid_shape); and the opt-in genuinely
* overrides shape - optin_dynamic_override has a static `extent` yet resolves to
* dynamic, and optin_static_no_size (no shape at all) resolves to static.
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests_duration.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_storage_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_storage_duration_of
//   the resolved siting across every value and both paths.
bool
tests_storage_duration_of()
{
    bool _stg_ok = true;

    // -- dynamic: allocator-backed / growable containers --
    D_STG_CHECK( (storage_duration_of<std::vector<int>>::value        == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::deque<int>>::value         == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::list<int>>::value          == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::forward_list<int>>::value  == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::set<int>>::value           == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::multiset<int>>::value      == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::unordered_set<int>>::value == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::map<int, int>>::value      == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::string>::value             == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<reserve_only>::value            == storage_duration::dynamic_storage) );

    // -- static: inline compile-time aggregates --
    D_STG_CHECK( (storage_duration_of<std::array<int, 4>>::value   == storage_duration::static_storage) );
    D_STG_CHECK( (storage_duration_of<std::pair<int, int>>::value  == storage_duration::static_storage) );
    D_STG_CHECK( (storage_duration_of<std::tuple<int, char>>::value == storage_duration::static_storage) );
    D_STG_CHECK( (storage_duration_of<extent_only>::value          == storage_duration::static_storage) );

    // -- hybrid: SHAPE-synthesised (both signals) and via the opt-in --
    D_STG_CHECK( (storage_duration_of<hybrid_shape>::value == storage_duration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of<optin_hybrid>::value == storage_duration::hybrid_storage) );

    // -- unknown: no signal at all --
    D_STG_CHECK( (storage_duration_of<int>::value       == storage_duration::unknown) );
    D_STG_CHECK( (storage_duration_of<no_signal>::value == storage_duration::unknown) );

    // -- opt-in OVERRIDES shape --
    // static `extent` shape, but the opt-in pins dynamic
    D_STG_CHECK( (storage_duration_of<optin_dynamic_override>::value == storage_duration::dynamic_storage) );
    // no shape at all, the opt-in pins static
    D_STG_CHECK( (storage_duration_of<optin_static_no_size>::value   == storage_duration::static_storage) );

    return _stg_ok;
}


// tests_storage_duration_of_companions
//   the ::type carrier and the `_v` value agree with ::value.
bool
tests_storage_duration_of_companions()
{
    bool _stg_ok = true;

    // ::type carrier
    D_STG_CHECK( (storage_duration_of_t<std::vector<int>>::value == storage_duration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of_t<std::array<int, 4>>::value == storage_duration::static_storage) );
    D_STG_CHECK( (storage_duration_of_t<hybrid_shape>::value     == storage_duration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of_t<int>::value             == storage_duration::unknown) );

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // _v companion
    D_STG_CHECK( (storage_duration_of_v<std::vector<int>>
                     == storage_duration_of<std::vector<int>>::value) );
    D_STG_CHECK( (storage_duration_of_v<std::array<int, 4>> == storage_duration::static_storage) );
    D_STG_CHECK( (storage_duration_of_v<optin_hybrid>       == storage_duration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of_v<optin_dynamic_override> == storage_duration::dynamic_storage) );
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
