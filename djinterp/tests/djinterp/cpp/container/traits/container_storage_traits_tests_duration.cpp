/*******************************************************************************
* djinterp [cpp][container]          container_storage_traits_tests_duration.cpp
*
*   Section II - storage_duration_of, the siting classifier, across all four
* sitings and both resolution paths.  Shape inference feeds make_storage_duration
* (static-only -> static, dynamic-only -> dynamic, BOTH -> hybrid, NEITHER ->
* unknown); an opt-in storage_duration_category overrides it entirely.
*
*   The edges: no standard container raises both signals, so hybrid is
* SYNTHESISED only by a custom type (DHybridShape); and the opt-in genuinely
* overrides shape - DOptinDynamicOverride has a static `extent` yet resolves to
* dynamic, and DOptinStaticNoSize (no shape at all) resolves to static.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_storage_traits_tests_duration.cpp
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
bool tests_storage_duration_of()
{
    bool _stg_ok = true;

    // - dynamic: allocator-backed / growable containers --
    D_STG_CHECK( (storage_duration_of<std::vector<int>>::value        == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::deque<int>>::value         == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::list<int>>::value          == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::forward_list<int>>::value  == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::set<int>>::value           == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::multiset<int>>::value      == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::unordered_set<int>>::value == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::map<int, int>>::value      == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<std::string>::value             == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of<reserve_only>::value            == DStorageDuration::dynamic_storage) );

    // - static: inline compile-time aggregates --
    D_STG_CHECK( (storage_duration_of<std::array<int, 4>>::value   == DStorageDuration::static_storage) );
    D_STG_CHECK( (storage_duration_of<std::pair<int, int>>::value  == DStorageDuration::static_storage) );
    D_STG_CHECK( (storage_duration_of<std::tuple<int, char>>::value == DStorageDuration::static_storage) );
    D_STG_CHECK( (storage_duration_of<extent_only>::value          == DStorageDuration::static_storage) );

    // - hybrid: SHAPE-synthesised (both signals) and via the opt-in --
    D_STG_CHECK( (storage_duration_of<DHybridShape>::value == DStorageDuration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of<DOptinHybrid>::value == DStorageDuration::hybrid_storage) );

    // - unknown: no signal at all --
    D_STG_CHECK( (storage_duration_of<int>::value       == DStorageDuration::unknown) );
    D_STG_CHECK( (storage_duration_of<no_signal>::value == DStorageDuration::unknown) );

    // - opt-in OVERRIDES shape --
    // static `extent` shape, but the opt-in pins dynamic
    D_STG_CHECK( (storage_duration_of<DOptinDynamicOverride>::value == DStorageDuration::dynamic_storage) );
    // no shape at all, the opt-in pins static
    D_STG_CHECK( (storage_duration_of<DOptinStaticNoSize>::value   == DStorageDuration::static_storage) );

    return _stg_ok;
}


// tests_storage_duration_of_companions
//   the ::type carrier and the `_v` value agree with ::value.
bool tests_storage_duration_of_companions()
{
    bool _stg_ok = true;

    // ::type carrier
    D_STG_CHECK( (storage_duration_of_t<std::vector<int>>::value == DStorageDuration::dynamic_storage) );
    D_STG_CHECK( (storage_duration_of_t<std::array<int, 4>>::value == DStorageDuration::static_storage) );
    D_STG_CHECK( (storage_duration_of_t<DHybridShape>::value     == DStorageDuration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of_t<int>::value             == DStorageDuration::unknown) );

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // _v companion
    D_STG_CHECK( (storage_duration_of_v<std::vector<int>>
                     == storage_duration_of<std::vector<int>>::value) );
    D_STG_CHECK( (storage_duration_of_v<std::array<int, 4>> == DStorageDuration::static_storage) );
    D_STG_CHECK( (storage_duration_of_v<DOptinHybrid>       == DStorageDuration::hybrid_storage) );
    D_STG_CHECK( (storage_duration_of_v<DOptinDynamicOverride> == DStorageDuration::dynamic_storage) );
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
