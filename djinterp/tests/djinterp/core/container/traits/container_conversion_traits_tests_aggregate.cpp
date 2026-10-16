/*******************************************************************************
* djinterp [testing]             container_conversion_traits_tests_aggregate.cpp
*
*   Section IV (aggregate + shorthands) - conversion_class, the assembled
* classification (tier, element compatibility, construction-path availability,
* losslessness, and the shared content level the two admit), and the `_v`
* variable-template companions of the tier predicates.
*
*   A deliberate subtlety pinned down here: conversion_class::has_path reports
* has_constructive_path DIRECTLY - a structural fact about constructor / insert
* availability - INDEPENDENT of element compatibility.  So an element-
* incompatible pair whose target merely OWNS a range constructor (vector<int> ->
* vector<opaque>) still reports has_path == true even though its tier is `none`,
* whereas a pair whose target exposes no range build (vector<int> -> sorted_vec)
* reports has_path == false.
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_conversion_class
//   the five assembled members, over one pair per tier plus the two contrasting
//   `none` shapes.
bool
tests_conversion_class()
{
    bool _ccv_ok = true;

    // -- view: vector -> vector --
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<int>>::tier
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<int>>::elements_ok) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<int>>::has_path) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<int>>::lossless) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<int>>::shared_content_level
                      == content_level::seq) );

    // -- constructive: vector -> list --
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::list<int>>::tier
                      == conversion_tier::constructive) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::list<int>>::elements_ok) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::list<int>>::has_path) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::list<int>>::lossless) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::list<int>>::shared_content_level
                      == content_level::seq) );

    // -- lossy: vector -> set (shared level is the coarser: set) --
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::set<int>>::tier
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::set<int>>::elements_ok) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::set<int>>::has_path) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, std::set<int>>::lossless) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::set<int>>::shared_content_level
                      == content_level::set) );

    // -- lossy: multiset -> set (shared level set) --
    D_CCV_CHECK( (conversion_class<std::multiset<int>, std::set<int>>::tier
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_class<std::multiset<int>, std::set<int>>::shared_content_level
                      == content_level::set) );

    // -- constructive: set -> vector (shared level still set) --
    D_CCV_CHECK( (conversion_class<std::set<int>, std::vector<int>>::tier
                      == conversion_tier::constructive) );
    D_CCV_CHECK( (conversion_class<std::set<int>, std::vector<int>>::shared_content_level
                      == content_level::set) );

    // -- structural: vector -> vector<vector<int>> (elements NOT ok, but the
    //    range ctor is signature-viable so has_path is true; shared level seq) --
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<std::vector<int>>>::tier
                      == conversion_tier::structural) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, std::vector<std::vector<int>>>::elements_ok) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<std::vector<int>>>::has_path) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, std::vector<std::vector<int>>>::lossless) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<std::vector<int>>>::shared_content_level
                      == content_level::seq) );

    // -- none via INCOMPATIBLE elements: has_path is STILL true (the target owns
    //    a range constructor; its body is never instantiated) --
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<opaque>>::tier
                      == conversion_tier::none) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, std::vector<opaque>>::elements_ok) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, std::vector<opaque>>::has_path) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, std::vector<opaque>>::lossless) );

    // -- none via NO construction path: elements ARE ok, but sorted_vec exposes
    //    no range build, so has_path is false (contrast the case above) --
    D_CCV_CHECK( (conversion_class<std::vector<int>, sorted_vec>::tier
                      == conversion_tier::none) );
    D_CCV_CHECK( (conversion_class<std::vector<int>, sorted_vec>::elements_ok) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, sorted_vec>::has_path) );
    D_CCV_CHECK( (!conversion_class<std::vector<int>, sorted_vec>::lossless) );

    return _ccv_ok;
}


// tests_conversion_variable_templates
//   the `_v` companions agree with their `::value` / member counterparts.
//   Gated on variable-template support exactly as the header emits them, so the
//   suite still builds on a C++11 baseline (where the companions are absent).
bool
tests_conversion_variable_templates()
{
    bool _ccv_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // conversion_tier_of_v mirrors the class value, and reads the right tier
    D_CCV_CHECK( (conversion_tier_of_v<std::vector<int>, std::vector<int>>
                      == conversion_tier_of<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (conversion_tier_of_v<std::vector<int>, std::vector<int>>
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of_v<std::vector<int>, std::set<int>>
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of_v<std::vector<int>, std::list<int>>
                      == conversion_tier::constructive) );

    // each boolean companion equals its ::value form
    D_CCV_CHECK( (is_convertible_between_v<std::vector<int>, std::vector<int>>
                      == is_convertible_between<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (is_view_conversion_v<std::vector<int>, std::vector<int>>
                      == is_view_conversion<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (is_lossless_conversion_v<std::vector<int>, std::list<int>>
                      == is_lossless_conversion<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (is_lossy_conversion_v<std::vector<int>, std::set<int>>
                      == is_lossy_conversion<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (conversion_requires_restructuring_v<std::vector<int>, std::vector<std::vector<int>>>
                      == conversion_requires_restructuring<std::vector<int>, std::vector<std::vector<int>>>::value) );

    // and read the expected booleans directly
    D_CCV_CHECK( (is_view_conversion_v<sorted_vec, std::vector<int>>) );
    D_CCV_CHECK( (!is_view_conversion_v<std::vector<int>, std::list<int>>) );
    D_CCV_CHECK( (is_lossless_conversion_v<std::vector<int>, std::list<int>>) );
    D_CCV_CHECK( (is_lossy_conversion_v<std::vector<int>, std::set<int>>) );
    D_CCV_CHECK( (conversion_requires_restructuring_v<std::vector<int>, std::vector<std::vector<int>>>) );
    D_CCV_CHECK( (is_convertible_between_v<std::vector<int>, std::vector<int>>) );
    D_CCV_CHECK( (!is_convertible_between_v<std::vector<int>, std::vector<opaque>>) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _ccv_ok;
}


NS_END  // testing
NS_END  // djinterp
