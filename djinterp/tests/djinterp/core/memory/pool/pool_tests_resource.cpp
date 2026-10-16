/*******************************************************************************
* djinterp [testing]                                     pool_tests_resource.cpp
*
* pool resource recognition: is_pool_resource across shapes and qualifiers.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_resource.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"

NS_DJINTERP
NS_TESTING
namespace dj = ::djinterp;

// tests_is_pool_resource
//   every pool shape is a pool_resource; ints, std containers, and pointers are not.
bool
tests_is_pool_resource()
{
    D_PO_CHECK(dj::is_pool_resource<pool_free_list>::value);
    D_PO_CHECK(dj::is_pool_resource<pool_monotonic>::value);
    D_PO_CHECK(dj::is_pool_resource<pool_contiguous>::value);
    D_PO_CHECK(dj::is_pool_resource<pool_generational>::value);

    D_PO_CHECK(!dj::is_pool_resource<int>::value);
    D_PO_CHECK(!dj::is_pool_resource<std::vector<int>>::value);
    D_PO_CHECK(!dj::is_pool_resource<int*>::value);
    return true;
}

// tests_pool_resource_cvref
//   recognition is agnostic to const / reference / cv-ref qualification.
bool
tests_pool_resource_cvref()
{
    D_PO_CHECK(dj::is_pool_resource<const pool_free_list>::value);
    D_PO_CHECK(dj::is_pool_resource<pool_free_list&>::value);
    D_PO_CHECK(dj::is_pool_resource<const pool_free_list&>::value);
    D_PO_CHECK(dj::is_pool_resource<pool_free_list&&>::value);
    return true;
}

NS_END  // testing
NS_END  // djinterp
