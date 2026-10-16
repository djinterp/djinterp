/*******************************************************************************
* djinterp [testing]                   element_relation_traits_tests_element.cpp
*
*   Section II - the element relation, read of a container's value_type.
* element_type_of extracts that type (void where there is none); the has_*_elements
* traits ask which comparisons it admits: == (equality), < (an order),
* the full relational set (totally ordered), and <=> (three-way, C++20).
*
*   The reading is of the ELEMENTS, not the container: a non-container (int) has no
* value_type, so every element trait is false even though int itself is comparable.
* The custom element types climb the rungs exactly - elem_eq stops at ==, elem_lt
* adds < but not the full set, elem_total has all five relational operators but no
* <=>, and (under C++20) elem_3way reaches <=>.
*
*
* path:      /tests/djinterp/core/container/traits/element_relation_traits_tests_element.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "element_relation_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_element_type_of
//   the value_type, or void where there is none.
bool
tests_element_type_of()
{
    bool _er_ok = true;

    D_ER_CHECK( (std::is_same<element_type_of_t<box<int>>, int>::value) );
    D_ER_CHECK( (std::is_same<element_type_of_t<box<elem_eq>>, elem_eq>::value) );
    D_ER_CHECK( (std::is_same<element_type_of_t<std::vector<double>>, double>::value) );
    D_ER_CHECK( (std::is_same<element_type_of_t<std::string>, char>::value) );

    // a non-container has no element type -> void
    D_ER_CHECK( (std::is_same<element_type_of_t<int>, void>::value) );

    // cv-ref is stripped before the lookup
    D_ER_CHECK( (std::is_same<element_type_of_t<const std::vector<int>&>, int>::value) );

    return _er_ok;
}


// tests_has_equality_comparable_elements
//   the value_type admits == (the =tau of the content equalities).
bool
tests_has_equality_comparable_elements()
{
    bool _er_ok = true;

    D_ER_CHECK( (has_equality_comparable_elements<box<elem_eq>>::value) );
    D_ER_CHECK( (has_equality_comparable_elements<box<elem_lt>>::value) );
    D_ER_CHECK( (has_equality_comparable_elements<box<elem_total>>::value) );
    D_ER_CHECK( (has_equality_comparable_elements<box<int>>::value) );
    D_ER_CHECK( (has_equality_comparable_elements<std::vector<int>>::value) );

    // no == on the element -> false
    D_ER_CHECK( (!has_equality_comparable_elements<box<elem_none>>::value) );
    // a non-container: no element type at all -> false (even though int has ==)
    D_ER_CHECK( (!has_equality_comparable_elements<int>::value) );

    return _er_ok;
}


// tests_has_less_than_comparable_elements
//   the value_type admits < (an order to sort or order by).
bool
tests_has_less_than_comparable_elements()
{
    bool _er_ok = true;

    D_ER_CHECK( (has_less_than_comparable_elements<box<elem_lt>>::value) );
    D_ER_CHECK( (has_less_than_comparable_elements<box<elem_total>>::value) );
    D_ER_CHECK( (has_less_than_comparable_elements<box<int>>::value) );
    D_ER_CHECK( (has_less_than_comparable_elements<std::vector<int>>::value) );

    // == but no < -> false
    D_ER_CHECK( (!has_less_than_comparable_elements<box<elem_eq>>::value) );
    D_ER_CHECK( (!has_less_than_comparable_elements<box<elem_none>>::value) );
    D_ER_CHECK( (!has_less_than_comparable_elements<int>::value) );

    return _er_ok;
}


// tests_has_totally_ordered_elements
//   the value_type admits the full relational set (==, <, <=, >, >=).
bool
tests_has_totally_ordered_elements()
{
    bool _er_ok = true;

    D_ER_CHECK( (has_totally_ordered_elements<box<elem_total>>::value) );
    D_ER_CHECK( (has_totally_ordered_elements<box<int>>::value) );          // int has all five (any standard)
    D_ER_CHECK( (has_totally_ordered_elements<std::vector<int>>::value) );

    // < present but not the full set -> false
    D_ER_CHECK( (!has_totally_ordered_elements<box<elem_lt>>::value) );
    D_ER_CHECK( (!has_totally_ordered_elements<box<elem_eq>>::value) );
    D_ER_CHECK( (!has_totally_ordered_elements<box<elem_none>>::value) );
    D_ER_CHECK( (!has_totally_ordered_elements<int>::value) );

    return _er_ok;
}


// tests_has_three_way_comparable_elements
//   the value_type admits <=> (unreachable below C++20).
bool
tests_has_three_way_comparable_elements()
{
    bool _er_ok = true;

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    D_ER_CHECK( (has_three_way_comparable_elements<box<elem_3way>>::value) );
    D_ER_CHECK( (has_three_way_comparable_elements<box<int>>::value) );      // int admits <=> in C++20
    D_ER_CHECK( (has_three_way_comparable_elements<std::vector<int>>::value) );
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON

    // the full relational set does NOT synthesise <=>; below C++20 this is always false
    D_ER_CHECK( (!has_three_way_comparable_elements<box<elem_total>>::value) );
    D_ER_CHECK( (!has_three_way_comparable_elements<box<elem_eq>>::value) );
    D_ER_CHECK( (!has_three_way_comparable_elements<box<elem_none>>::value) );
    D_ER_CHECK( (!has_three_way_comparable_elements<int>::value) );

    return _er_ok;
}


NS_END  // testing
NS_END  // djinterp
