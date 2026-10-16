/*******************************************************************************
* djinterp [testing]               iterable_container_traits_tests_detection.cpp
*
*   Section I - the raw SFINAE probes.  has_begin_method / has_end_method are taken
* on a NON-CONST lvalue, so they detect a traversal whatever its qualification: a
* const-only begin() still fires (a const member is callable on a non-const lvalue),
* which is what keeps the presence question separate from the const/non-const MODE
* of section III.  has_iterator_alias / has_const_iterator_alias / has_value_type_alias
* detect nested type aliases, independently of any traversal.
*
*   The edges: the two method probes are INDEPENDENT (begin_only and end_only fire
* on one each), a built-in array has no MEMBER begin() at all (the free std::begin
* is not what is probed), and alias_only carries iterator aliases while being
* neither iterable nor container-shaped - so alias presence implies nothing about
* traversal, and vice versa (mutable_only_iter traverses with no aliases at all).
*
*
* path:      /tests/djinterp/core/container/traits/iterable_container_traits_tests_detection.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "iterable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_has_begin_end_methods
//   the begin()/end() member probes, taken on a non-const lvalue.
bool
tests_has_begin_end_methods()
{
    bool _ic_ok = true;

    // standard containers expose both
    D_IC_CHECK( (has_begin_method<std::vector<int>>::value) );
    D_IC_CHECK( (has_end_method<std::vector<int>>::value) );
    D_IC_CHECK( (has_begin_method<std::forward_list<int>>::value) );
    D_IC_CHECK( (has_end_method<std::forward_list<int>>::value) );
    D_IC_CHECK( (has_begin_method<std::set<int>>::value) );
    D_IC_CHECK( (has_end_method<std::map<int, int>>::value) );
    D_IC_CHECK( (has_begin_method<std::array<int, 4>>::value) );
    D_IC_CHECK( (has_begin_method<std::string>::value) );

    // a CONST-only begin()/end() still fires - the probe uses a non-const lvalue,
    // on which const members remain callable
    D_IC_CHECK( (has_begin_method<const_only_iter>::value) );
    D_IC_CHECK( (has_end_method<const_only_iter>::value) );
    // ... as does a non-const-only one
    D_IC_CHECK( (has_begin_method<mutable_only_iter>::value) );
    D_IC_CHECK( (has_end_method<mutable_only_iter>::value) );

    // the two probes are independent
    D_IC_CHECK( (has_begin_method<begin_only>::value) );
    D_IC_CHECK( (!has_end_method<begin_only>::value) );
    D_IC_CHECK( (!has_begin_method<end_only>::value) );
    D_IC_CHECK( (has_end_method<end_only>::value) );

    // no traversal at all
    D_IC_CHECK( (!has_begin_method<positional_only>::value) );
    D_IC_CHECK( (!has_end_method<positional_only>::value) );
    D_IC_CHECK( (!has_begin_method<alias_only>::value) );      // aliases are not a traversal
    D_IC_CHECK( (!has_begin_method<int>::value) );
    // a built-in array has no MEMBER begin() (the free std::begin is not probed)
    D_IC_CHECK( (!has_begin_method<int[4]>::value) );
    D_IC_CHECK( (!has_end_method<int[4]>::value) );

    // cv-ref is stripped before the probe
    D_IC_CHECK( (has_begin_method<const std::vector<int>&>::value) );
    D_IC_CHECK( (has_end_method<std::vector<int>&&>::value) );

    return _ic_ok;
}


// tests_alias_detection
//   the nested iterator / const_iterator / value_type alias probes.
bool
tests_alias_detection()
{
    bool _ic_ok = true;

    // standard containers carry all three
    D_IC_CHECK( (has_iterator_alias<std::vector<int>>::value) );
    D_IC_CHECK( (has_const_iterator_alias<std::vector<int>>::value) );
    D_IC_CHECK( (has_value_type_alias<std::vector<int>>::value) );
    D_IC_CHECK( (has_iterator_alias<std::set<int>>::value) );
    D_IC_CHECK( (has_const_iterator_alias<std::map<int, int>>::value) );
    D_IC_CHECK( (has_value_type_alias<std::array<int, 4>>::value) );
    D_IC_CHECK( (has_value_type_alias<std::string>::value) );

    // aliases without any traversal
    D_IC_CHECK( (has_iterator_alias<alias_only>::value) );
    D_IC_CHECK( (has_const_iterator_alias<alias_only>::value) );
    D_IC_CHECK( (!has_value_type_alias<alias_only>::value) );   // not container-shaped

    // traversal without any aliases - the converse independence
    D_IC_CHECK( (!has_iterator_alias<mutable_only_iter>::value) );
    D_IC_CHECK( (!has_const_iterator_alias<mutable_only_iter>::value) );
    D_IC_CHECK( (has_value_type_alias<mutable_only_iter>::value) );

    // value_type is the container-hood marker: present on the positional-only type,
    // absent from the traversal-only one
    D_IC_CHECK( (has_value_type_alias<positional_only>::value) );
    D_IC_CHECK( (!has_value_type_alias<no_value_type_iter>::value) );

    // non-containers carry none of the three
    D_IC_CHECK( (!has_iterator_alias<int>::value) );
    D_IC_CHECK( (!has_const_iterator_alias<int>::value) );
    D_IC_CHECK( (!has_value_type_alias<int>::value) );
    D_IC_CHECK( (!has_value_type_alias<int[4]>::value) );

    // cv-ref is stripped before the lookup
    D_IC_CHECK( (has_value_type_alias<const std::vector<int>&>::value) );

    return _ic_ok;
}


NS_END  // testing
NS_END  // djinterp
