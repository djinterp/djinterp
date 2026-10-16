/*******************************************************************************
* djinterp [testing]                       container_traits_tests_underlying.cpp
*
*   Sections XIII-XV - underlying-container relationships and interval
* extraction.  is_underlying_container is true when a type delegates storage via
* an underlying_container_type alias; is_fundamental_container is its complement,
* WIDENED so that C/std arrays are always fundamental (is_c_array || is_std_array
* || !is_underlying).  underlying_container_type_of extracts that alias (else
* void), and size/depth/multiplicity_interval_of extract the interval aliases -
* but only when they are VALID intervals, so a bogus alias extracts to void.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_underlying.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_underlying_container
//   an underlying_container_type alias marks a delegating container.
bool
tests_is_underlying_container()
{
    bool _ct_ok = true;

    D_CT_CHECK( (is_underlying_container<adaptor>::value) );

    D_CT_CHECK( (!is_underlying_container<std::vector<int>>::value) );
    D_CT_CHECK( (!is_underlying_container<std::array<int, 4>>::value) );
    D_CT_CHECK( (!is_underlying_container<fixed4>::value) );
    D_CT_CHECK( (!is_underlying_container<int>::value) );

    return _ct_ok;
}


// tests_is_fundamental_container
//   provides own storage: an array, or anything without an underlying type.
bool
tests_is_fundamental_container()
{
    bool _ct_ok = true;

    // arrays are always fundamental; so is anything with no underlying alias
    D_CT_CHECK( (is_fundamental_container<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_fundamental_container<int[4]>::value) );
    D_CT_CHECK( (is_fundamental_container<std::vector<int>>::value) );
    D_CT_CHECK( (is_fundamental_container<std::set<int>>::value) );
    D_CT_CHECK( (is_fundamental_container<fixed4>::value) );
    D_CT_CHECK( (is_fundamental_container<int>::value) );

    // the delegating adaptor is the sole non-fundamental
    D_CT_CHECK( (!is_fundamental_container<adaptor>::value) );

    // exact complement on the adaptor (which is not an array)
    D_CT_CHECK( (is_fundamental_container<adaptor>::value != is_underlying_container<adaptor>::value) );

    return _ct_ok;
}


// tests_underlying_container_type_of
//   yields the underlying_container_type, or void when absent.
bool
tests_underlying_container_type_of()
{
    bool _ct_ok = true;

    D_CT_CHECK( (std::is_same<underlying_container_type_of_t<adaptor>, std::vector<int>>::value) );

    // no underlying alias -> void
    D_CT_CHECK( (std::is_same<underlying_container_type_of_t<std::vector<int>>, void>::value) );
    D_CT_CHECK( (std::is_same<underlying_container_type_of_t<std::array<int, 4>>, void>::value) );
    D_CT_CHECK( (std::is_same<underlying_container_type_of_t<int>, void>::value) );

    return _ct_ok;
}


// tests_interval_extraction
//   size / depth / multiplicity_interval_of - void unless a VALID interval alias
//   is present.  A bogus (non-interval) alias extracts to void, as do absent ones.
bool
tests_interval_extraction()
{
    bool _ct_ok = true;

    // intervals_bogus declares the aliases, but they are invalid -> void
    D_CT_CHECK( (std::is_same<size_interval_of_t<intervals_bogus>, void>::value) );
    D_CT_CHECK( (std::is_same<depth_interval_of_t<intervals_bogus>, void>::value) );
    D_CT_CHECK( (std::is_same<multiplicity_interval_of_t<intervals_bogus>, void>::value) );

    // no alias at all -> void
    D_CT_CHECK( (std::is_same<size_interval_of_t<std::vector<int>>, void>::value) );
    D_CT_CHECK( (std::is_same<depth_interval_of_t<std::set<int>>, void>::value) );
    D_CT_CHECK( (std::is_same<multiplicity_interval_of_t<int>, void>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
