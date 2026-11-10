/*******************************************************************************
* djinterp [testing]                          container_traits_tests_storage.cpp
*
*   Section VII - this header's legacy storage classification (distinct from the
* container_storage_traits.hpp axis).  is_static_storage = a C/std array OR a
* fixed size OR a bounded capacity WITHOUT an allocator; is_dynamic_storage = a
* type WITH size() that is not static AND either has an allocator or an unbounded
* capacity.
*
*   Two edges anchor the coverage: DAllocBounded has a bounded capacity but ALSO
* an allocator, so the `!has_allocator_type` guard makes it dynamic, not static;
* and std::forward_list has no size() at all, so it is NEITHER static nor dynamic
* (the dynamic predicate requires a size accessor).  DEmptyCt shows the axes are
* independent - it is a compile-time container (Section V) yet dynamic storage.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_traits_tests_storage.cpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_static_storage
//   is_static_storage_container, as container_storage_traits.hpp classifies:
// inline cells (a bounded C array, a member extent, a tuple_size), and no
// allocator.  size(), max_size() and min_size() are not storage signals.
bool tests_is_static_storage()
{
    bool _ct_ok = true;

    // inline aggregates
    D_CT_CHECK( (is_static_storage_container<std::array<int, 4>>::value) );  // tuple_size
    D_CT_CHECK( (is_static_storage_container<int[4]>::value) );              // bounded C array

    // size shapes alone say nothing of siting
    D_CT_CHECK( (!is_static_storage_container<DFixed4>::value) );
    D_CT_CHECK( (!is_static_storage_container<DBoundedCap>::value) );
    D_CT_CHECK( (!is_static_storage_container<DMin5>::value) );
    D_CT_CHECK( (!is_static_storage_container<DUnboundedHuge>::value) );
    D_CT_CHECK( (!is_static_storage_container<DEmptyCt>::value) );
    // an allocator means out-of-line cells
    D_CT_CHECK( (!is_static_storage_container<DAllocBounded>::value) );
    D_CT_CHECK( (!is_static_storage_container<std::vector<int>>::value) );
    D_CT_CHECK( (!is_static_storage_container<std::deque<int>>::value) );
    D_CT_CHECK( (!is_static_storage_container<int>::value) );

    return _ct_ok;
}


// tests_is_dynamic_storage
//   is_dynamic_storage_container: an allocator_type or a reserve(n) - cells
// drawn from an acquired, out-of-line region.
bool tests_is_dynamic_storage()
{
    bool _ct_ok = true;

    // allocator-backed std containers, a forward_list among them
    D_CT_CHECK( (is_dynamic_storage_container<std::vector<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage_container<std::deque<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage_container<std::set<int>>::value) );
    D_CT_CHECK( (is_dynamic_storage_container<std::string>::value) );
    D_CT_CHECK( (is_dynamic_storage_container<std::forward_list<int>>::value) );
    // a fixture with an allocator alias
    D_CT_CHECK( (is_dynamic_storage_container<DAllocBounded>::value) );

    // no allocator and no reserve: not dynamic, whatever its size shape
    D_CT_CHECK( (!is_dynamic_storage_container<DMin5>::value) );
    D_CT_CHECK( (!is_dynamic_storage_container<DUnboundedHuge>::value) );
    D_CT_CHECK( (!is_dynamic_storage_container<DEmptyCt>::value) );
    D_CT_CHECK( (!is_dynamic_storage_container<DAdaptor>::value) );
    // inline aggregates are not dynamic
    D_CT_CHECK( (!is_dynamic_storage_container<std::array<int, 4>>::value) );
    D_CT_CHECK( (!is_dynamic_storage_container<int[4]>::value) );
    D_CT_CHECK( (!is_dynamic_storage_container<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
