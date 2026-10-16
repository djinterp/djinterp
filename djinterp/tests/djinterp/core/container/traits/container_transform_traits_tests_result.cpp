/*******************************************************************************
* djinterp [testing]                 container_transform_traits_tests_result.cpp
*
*   Section V - the result type a mapping builds, and the element rebind beneath
* it.  rebind_element<C, Sigma> replaces C's FIRST type argument with Sigma
* (Box<int> -> Box<double>), preserving the rest, and is void for a non type-pack
* template.  transform_result_type<C, Sigma> is the container a mapping into Sigma
* builds: a cleanly-rebindable source rebinds; any other source - an
* allocator-coupled standard container, or a non type-pack such as std::array -
* falls back to std::vector<Sigma>; a non-source has no result.
*
*   The confinement is the point: rebind_element applied to std::vector<int>
* replaces only the element, yielding std::vector<double, std::allocator<int>> with
* a MISTYPED allocator - which is exactly why the result helper does not rebind
* allocator-coupled containers and offers std::vector<Sigma> instead.
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests_result.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>   // std::allocator (for the mistyped-rebind witness)
#include <vector>
// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_rebind_element
//   first-argument replacement, rest-preserving; void for non type-pack templates.
bool
tests_rebind_element()
{
    bool _tx_ok = true;

    // clean single-parameter template
    D_TX_CHECK( (std::is_same<rebind_element_t<Box<int>, double>, Box<double>>::value) );
    // identity rebind (Sigma == tau)
    D_TX_CHECK( (std::is_same<rebind_element_t<Box<int>, int>, Box<int>>::value) );
    // rest preservation: only the FIRST argument changes
    D_TX_CHECK( (std::is_same<rebind_element_t<pair_box<int, char>, double>, pair_box<double, char>>::value) );

    // a standard container rebinds its element BUT NOT its allocator (mistyped) -
    // the very reason rebind is confined to the result helper
    D_TX_CHECK( (std::is_same<rebind_element_t<std::vector<int>, double>,
                              std::vector<double, std::allocator<int>>>::value) );

    // a non type-pack template (std::array has a non-type parameter) -> void
    D_TX_CHECK( (std::is_same<rebind_element_t<std::array<int, 4>, double>, void>::value) );

    return _tx_ok;
}


// tests_transform_result_type
//   cleanly-rebindable sources rebind; other sources fall back to vector<Sigma>;
//   non-sources yield void.
bool
tests_transform_result_type()
{
    bool _tx_ok = true;

    // cleanly rebindable: a clean element-parametric source rebinds to Sigma
    D_TX_CHECK( (std::is_same<transform_result_type_t<Box<int>, double>, Box<double>>::value) );
    D_TX_CHECK( (std::is_same<transform_result_type_t<Box<int>, int>, Box<int>>::value) );  // identity

    // allocator-coupled standard container -> std::vector<Sigma> (safe result)
    D_TX_CHECK( (std::is_same<transform_result_type_t<std::vector<int>, double>, std::vector<double>>::value) );

    // non type-pack source (std::array) -> std::vector<Sigma>
    D_TX_CHECK( (std::is_same<transform_result_type_t<std::array<int, 4>, double>, std::vector<double>>::value) );

    // a plain-struct source (even input-only, even native) -> std::vector<Sigma>
    D_TX_CHECK( (std::is_same<transform_result_type_t<read_only_seq, double>, std::vector<double>>::value) );
    D_TX_CHECK( (std::is_same<transform_result_type_t<native_box, double>, std::vector<double>>::value) );

    // a non-source has no result
    D_TX_CHECK( (std::is_same<transform_result_type_t<int, double>, void>::value) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
