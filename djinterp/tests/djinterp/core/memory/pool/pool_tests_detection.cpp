/*******************************************************************************
* djinterp [testing]                                    pool_tests_detection.cpp
*
* pool detection: the pool_resource method and policy-constant detectors.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"

NS_DJINTERP
NS_TESTING
namespace dj = ::djinterp;

// tests_pool_method_surface
//   a pool exposes acquire/release/size/value_type/reset/reserve; a plain type
// exposes none, and a std container has value_type/size() but no acquire/release.
bool
tests_pool_method_surface()
{
    D_PO_CHECK(dj::has_acquire_v<pool_free_list>);
    D_PO_CHECK(dj::has_release_v<pool_free_list>);
    D_PO_CHECK(dj::has_size_accessor_v<pool_free_list>);
    D_PO_CHECK(dj::has_value_type_v<pool_free_list>);
    D_PO_CHECK(dj::has_pool_reset_v<pool_free_list>);
    D_PO_CHECK(dj::has_pool_reserve_v<pool_free_list>);

    D_PO_CHECK(!dj::has_acquire_v<int>);
    D_PO_CHECK(!dj::has_release_v<int>);
    D_PO_CHECK(!dj::has_size_accessor_v<int>);

    D_PO_CHECK(dj::has_value_type_v<std::vector<int>>);
    D_PO_CHECK(dj::has_size_accessor_v<std::vector<int>>);
    D_PO_CHECK(!dj::has_acquire_v<std::vector<int>>);
    D_PO_CHECK(!dj::has_release_v<std::vector<int>>);
    return true;
}

// tests_pool_policy_constants
//   the pointer-stable / individual-release / generational-sweep policy
// constants are visible on a pool and absent on a plain type.
bool
tests_pool_policy_constants()
{
    D_PO_CHECK(dj::has_pointer_stable_constant_v<pool_free_list>);
    D_PO_CHECK(dj::has_supports_individual_release_constant_v<pool_free_list>);
    D_PO_CHECK(dj::has_supports_generational_sweep_constant_v<pool_generational>);

    D_PO_CHECK(!dj::has_pointer_stable_constant_v<int>);
    D_PO_CHECK(!dj::has_supports_individual_release_constant_v<int>);
    D_PO_CHECK(!dj::has_supports_generational_sweep_constant_v<int>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
