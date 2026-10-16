/*******************************************************************************
* djinterp [testing]                          container_traits_tests_storage.cpp
*
*   Section VII - this header's legacy storage classification (distinct from the
* container_storage_traits.hpp axis).  is_static_storage = a C/std array OR a
* fixed size OR a bounded capacity WITHOUT an allocator; is_dynamic_storage = a
* type WITH size() that is not static AND either has an allocator or an unbounded
* capacity.
*
*   Two edges anchor the coverage: alloc_bounded has a bounded capacity but ALSO
* an allocator, so the `!has_allocator_type` guard makes it dynamic, not static;
* and std::forward_list has no size() at all, so it is NEITHER static nor dynamic
* (the dynamic predicate requires a size accessor).  empty_ct shows the axes are
* independent - it is a compile-time container (Section V) yet dynamic storage.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_storage.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_static_storage
//   arrays / fixed size / bounded-capacity-without-allocator.
bool
tests_is_static_storage()
{
    bool _ct_ok = true;

    // arrays and the fixed / bounded-capacity fixtures
    D_CT_CHECK( (is_static_storage<std::array<int, 4>>::value) );  // is_std_array
    D_CT_CHECK( (is_static_storage<int[4]>::value) );              // is_c_array
    D_CT_CHECK( (is_static_storage<fixed4>::value) );              // has_fixed_size
    D_CT_CHECK( (is_static_storage<bounded_cap>::value) );         // bounded cap, no allocator

    // bounded capacity BUT an allocator -> NOT static
    D_CT_CHECK( (!is_static_storage<alloc_bounded>::value) );
    // no upper signal at all -> not static
    D_CT_CHECK( (!is_static_storage<min5>::value) );
    D_CT_CHECK( (!is_static_storage<unbounded_huge>::value) );
    D_CT_CHECK( (!is_static_storage<empty_ct>::value) );
    // allocator-backed / non-constexpr containers
    D_CT_CHECK( (!is_static_storage<std::vector<int>>::value) );   // bounded cap but has allocator
    D_CT_CHECK( (!is_static_storage<std::deque<int>>::value) );
    D_CT_CHECK( (!is_static_storage<int>::value) );

    return _ct_ok;
}


// tests_is_dynamic_storage
//   has size(), not static, and (allocator OR unbounded capacity).
bool
tests_is_dynamic_storage()
{
    bool _ct_ok = true;

    // allocator-backed / growable std containers
    D_CT_CHECK( (is_dynamic_storage<std::vector<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage<std::deque<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage<std::set<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage<std::string>::value) );
    // fixtures: allocator OR unbounded capacity, all with size()
    D_CT_CHECK( (is_dynamic_storage<alloc_bounded>::value) );   // bounded cap + allocator
    D_CT_CHECK( (is_dynamic_storage<min5>::value) );            // no bounded cap
    D_CT_CHECK( (is_dynamic_storage<unbounded_huge>::value) );  // capacity unbounded
    D_CT_CHECK( (is_dynamic_storage<empty_ct>::value) );        // compile-time AND dynamic
    D_CT_CHECK( (is_dynamic_storage<adaptor>::value) );

    // static-storage types are not dynamic
    D_CT_CHECK( (!is_dynamic_storage<std::array<int, 4>>::value) );
    D_CT_CHECK( (!is_dynamic_storage<int[4]>::value) );
    D_CT_CHECK( (!is_dynamic_storage<fixed4>::value) );
    D_CT_CHECK( (!is_dynamic_storage<bounded_cap>::value) );
    // no size() -> NEITHER static nor dynamic
    D_CT_CHECK( (!is_dynamic_storage<std::forward_list<int>>::value) );
    D_CT_CHECK( (!is_dynamic_storage<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
