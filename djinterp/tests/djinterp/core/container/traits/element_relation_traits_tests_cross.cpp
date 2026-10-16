/*******************************************************************************
* djinterp [testing]                     element_relation_traits_tests_cross.cpp
*
*   Section V - element compatibility across two DIFFERENT container types, the
* prerequisite for conversion and for comparing unlike containers element-wise.
* elements_same_type / _convertible / _mutually_convertible relate the two
* value_types; cross_elements_equality_comparable / _ordered ask whether an element
* of one can be == or < compared with an element of the other.
*
*   Edges: because a missing value_type reads as void, two NON-containers share an
* element type (void == void) and are trivially "convertible" (void -> void), so
* elements_same_type<int,int> and elements_convertible<int,int> are both true;
* convert_from_int gives a genuinely one-directional conversion, so mutual
* convertibility is false there; and the cross relation gate returns false the
* moment either side lacks an element type.  This file also gathers the `_v`
* companions.
*
*
* path:      /tests/djinterp/core/container/traits/element_relation_traits_tests_cross.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "element_relation_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_elements_same_type
//   the two containers share a value_type.
bool
tests_elements_same_type()
{
    bool _er_ok = true;

    D_ER_CHECK( (elements_same_type<box<int>, box<int>>::value) );
    D_ER_CHECK( (elements_same_type<std::vector<int>, box<int>>::value) );  // both int
    D_ER_CHECK( (!elements_same_type<box<int>, box<double>>::value) );

    // void-element edge: two non-containers "share" the void element type
    D_ER_CHECK( (elements_same_type<int, int>::value) );
    // a container vs a non-container: int-element vs void -> false
    D_ER_CHECK( (!elements_same_type<box<int>, int>::value) );

    return _er_ok;
}


// tests_elements_convertible
//   the left container's element converts to the right's.
bool
tests_elements_convertible()
{
    bool _er_ok = true;

    D_ER_CHECK( (elements_convertible<box<int>, box<int>>::value) );
    D_ER_CHECK( (elements_convertible<box<int>, box<double>>::value) );   // int -> double
    D_ER_CHECK( (elements_convertible<box<double>, box<int>>::value) );   // double -> int (narrowing, still convertible)

    // not convertible
    D_ER_CHECK( (!elements_convertible<box<int>, box<std::string>>::value) );

    // void-element edge: void -> void is convertible
    D_ER_CHECK( (elements_convertible<int, int>::value) );

    return _er_ok;
}


// tests_elements_mutually_convertible
//   the element types convert in BOTH directions.
bool
tests_elements_mutually_convertible()
{
    bool _er_ok = true;

    D_ER_CHECK( (elements_mutually_convertible<box<int>, box<double>>::value) );  // both ways

    // one-directional only: int -> convert_from_int, but not back
    D_ER_CHECK( (elements_convertible<box<int>, box<convert_from_int>>::value) );
    D_ER_CHECK( (!elements_convertible<box<convert_from_int>, box<int>>::value) );
    D_ER_CHECK( (!elements_mutually_convertible<box<int>, box<convert_from_int>>::value) );

    // neither direction
    D_ER_CHECK( (!elements_mutually_convertible<box<int>, box<std::string>>::value) );

    return _er_ok;
}


// tests_cross_elements_equality_comparable
//   an element of the left can be == compared with an element of the right.
bool
tests_cross_elements_equality_comparable()
{
    bool _er_ok = true;

    D_ER_CHECK( (cross_elements_equality_comparable<box<int>, box<int>>::value) );
    D_ER_CHECK( (cross_elements_equality_comparable<box<int>, box<double>>::value) );  // int == double

    // an element with no == against int -> false
    D_ER_CHECK( (!cross_elements_equality_comparable<box<int>, box<elem_none>>::value) );
    // the gate: either side lacking an element type -> false
    D_ER_CHECK( (!cross_elements_equality_comparable<int, box<int>>::value) );
    D_ER_CHECK( (!cross_elements_equality_comparable<box<int>, int>::value) );

    return _er_ok;
}


// tests_cross_elements_ordered
//   an element of the left can be < compared with an element of the right.
bool
tests_cross_elements_ordered()
{
    bool _er_ok = true;

    D_ER_CHECK( (cross_elements_ordered<box<int>, box<int>>::value) );
    D_ER_CHECK( (cross_elements_ordered<box<int>, box<double>>::value) );  // int < double

    // elements with == but no < -> not cross-ordered (even same type)
    D_ER_CHECK( (!cross_elements_ordered<box<elem_eq>, box<elem_eq>>::value) );
    // the gate
    D_ER_CHECK( (!cross_elements_ordered<int, box<int>>::value) );

    return _er_ok;
}


// tests_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on inline-
//   variable support exactly as the header emits them.
bool
tests_variable_templates()
{
    bool _er_ok = true;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    // element companions
    D_ER_CHECK( (has_equality_comparable_elements_v<box<elem_eq>>
                     == has_equality_comparable_elements<box<elem_eq>>::value) );
    D_ER_CHECK( (has_less_than_comparable_elements_v<box<elem_lt>>) );
    D_ER_CHECK( (has_totally_ordered_elements_v<box<elem_total>>) );
    D_ER_CHECK( (!has_totally_ordered_elements_v<box<elem_lt>>) );
    D_ER_CHECK( (element_comparison_degree_of_v<box<elem_eq>> == comparison_degree::equality) );

    // container companions
    D_ER_CHECK( (has_container_equality_v<cmp_eq> == has_container_equality<cmp_eq>::value) );
    D_ER_CHECK( (!has_container_less_v<cmp_eq>) );
    D_ER_CHECK( (container_comparison_degree_of_v<cmp_partial> == comparison_degree::partial_order) );
    D_ER_CHECK( (is_equality_comparable_container_v<cmp_eq>) );
    D_ER_CHECK( (!is_equality_comparable_container_v<int>) );          // the observational edge
    D_ER_CHECK( (is_ordered_comparable_container_v<cmp_total>) );

    // cross companions
    D_ER_CHECK( (elements_same_type_v<box<int>, box<int>>) );
    D_ER_CHECK( (elements_convertible_v<box<int>, box<double>>) );
    D_ER_CHECK( (!elements_mutually_convertible_v<box<int>, box<convert_from_int>>) );
    D_ER_CHECK( (cross_elements_equality_comparable_v<box<int>, box<double>>) );
    D_ER_CHECK( (!cross_elements_ordered_v<box<elem_eq>, box<elem_eq>>) );

  #if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    D_ER_CHECK( (has_three_way_comparable_elements_v<box<int>>) );
    D_ER_CHECK( (has_three_way_comparable_container_v<int>) );         // int declares <=>
  #endif

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    return _er_ok;
}


NS_END  // testing
NS_END  // djinterp
