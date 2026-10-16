/*******************************************************************************
* djinterp [testing]                         container_traits_tests_lifetime.cpp
*
*   Section V - lifetime classification.  is_compile_time_container is this
* header's own narrow notion: an EMPTY, stateless type with a constexpr size()
* and NO allocator (a type that carries no runtime state).  is_not_compile_time_
* container is its negation.  The empty constexpr-sized fixtures qualify; a real
* std:: container (which holds storage, so is not empty) does not, and neither
* does an otherwise-empty type that declares an allocator or whose size() is not
* constexpr.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_lifetime.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_lifetime_classification
//   is_compile_time_container and its negation.
bool
tests_lifetime_classification()
{
    bool _ct_ok = true;

    // compile-time containers: empty, stateless, constexpr-sized, no allocator
    D_CT_CHECK( (is_compile_time_container<empty_ct>::value) );
    D_CT_CHECK( (is_compile_time_container<fixed4>::value) );
    D_CT_CHECK( (is_compile_time_container<bounded_cap>::value) );
    D_CT_CHECK( (is_compile_time_container<min5>::value) );
    D_CT_CHECK( (is_compile_time_container<unbounded_huge>::value) );

    // NOT compile-time: an allocator disqualifies (even though empty + constexpr)
    D_CT_CHECK( (!is_compile_time_container<alloc_bounded>::value) );
    // NOT compile-time: size() is not constexpr here
    D_CT_CHECK( (!is_compile_time_container<adaptor>::value) );
    // NOT compile-time: real containers hold storage, so are not empty
    D_CT_CHECK( (!is_compile_time_container<std::vector<int>>::value) );
    D_CT_CHECK( (!is_compile_time_container<std::array<int, 4>>::value) );
    D_CT_CHECK( (!is_compile_time_container<int>::value) );

    // the negation mirrors it exactly
    D_CT_CHECK( (!is_not_compile_time_container<empty_ct>::value) );
    D_CT_CHECK( (!is_not_compile_time_container<fixed4>::value) );
    D_CT_CHECK( (is_not_compile_time_container<alloc_bounded>::value) );
    D_CT_CHECK( (is_not_compile_time_container<std::vector<int>>::value) );
    D_CT_CHECK( (is_not_compile_time_container<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_not_compile_time_container<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
