/*******************************************************************************
* djinterp [testing]                 element_relation_traits_tests_container.cpp
*
*   Section IV - the container as a whole, the observational reading.
* has_container_equality / has_container_less detect the operators the container
* DECLARES; has_three_way_comparable_container detects <=>.  But actual
* comparability is container_comparison_degree_of = weaker(declared degree, element
* degree): a std container lifts its elements' comparison, so its degree is capped
* by BOTH what it declares and what its elements support.  is_equality_comparable_
* container and is_ordered_comparable_container are the >= equality / >= partial_order
* projections.
*
*   Two edges anchor this.  A non-container (int) DECLARES a full comparison (int==int,
* int<=>int both compile, so has_container_equality<int> is true), yet its element
* degree is none, so container_comparison_degree_of<int> is none and
* is_equality_comparable_container<int> is FALSE - the observational reading needs
* actual elements.  And DCmpTotalEqelem declares all five relational operators but
* has only ==-comparable elements, so its degree is capped at equality by the
* elements, not the declaration.
*
*
* path:      /tests/djinterp/cpp/container/traits/element_relation_traits_tests_container.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "element_relation_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_has_container_equality_and_less
//   the DECLARED container operators (not yet gated by element support).
bool tests_has_container_equality_and_less()
{
    bool _er_ok = true;

    // == declared as a whole
    D_ER_CHECK( (has_container_equality<DCmpEq>::value) );
    D_ER_CHECK( (has_container_equality<DCmpTotal>::value) );
    D_ER_CHECK( (has_container_equality<std::vector<int>>::value) );
    D_ER_CHECK( (has_container_equality<int>::value) );          // int == int compiles
    D_ER_CHECK( (!has_container_equality<box<int>>::value) );    // box declares no ==

    // < declared as a whole
    D_ER_CHECK( (has_container_less<DCmpPartial>::value) );
    D_ER_CHECK( (has_container_less<DCmpTotal>::value) );
    D_ER_CHECK( (has_container_less<std::vector<int>>::value) );
    D_ER_CHECK( (has_container_less<int>::value) );              // int < int compiles
    D_ER_CHECK( (!has_container_less<DCmpEq>::value) );          // == only, no <
    D_ER_CHECK( (!has_container_less<box<int>>::value) );

    return _er_ok;
}


// tests_has_three_way_comparable_container
//   the container itself admits <=> (unreachable below C++20).
bool tests_has_three_way_comparable_container()
{
    bool _er_ok = true;

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    D_ER_CHECK( (has_three_way_comparable_container<cmp_3way>::value) );
    D_ER_CHECK( (has_three_way_comparable_container<std::vector<int>>::value) );
    D_ER_CHECK( (has_three_way_comparable_container<int>::value) );   // int <=> int in C++20
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON

    // a container declaring the relational set but no <=> is not three-way
    D_ER_CHECK( (!has_three_way_comparable_container<DCmpTotal>::value) );
    D_ER_CHECK( (!has_three_way_comparable_container<DCmpEq>::value) );
    D_ER_CHECK( (!has_three_way_comparable_container<box<int>>::value) );

    return _er_ok;
}


// tests_container_comparison_degree_of
//   weaker(declared degree, element degree) - the ACTUAL comparability.
bool tests_container_comparison_degree_of()
{
    bool _er_ok = true;

    // declaration tracks (int elements are the strongest, so they never cap here)
    D_ER_CHECK( (container_comparison_degree_of<DCmpEq>::value      == DComparisonDegree::equality) );
    D_ER_CHECK( (container_comparison_degree_of<DCmpPartial>::value == DComparisonDegree::partial_order) );
    D_ER_CHECK( (container_comparison_degree_of<DCmpTotal>::value   == DComparisonDegree::total_order) );

    // no declared container comparison -> none
    D_ER_CHECK( (container_comparison_degree_of<box<int>>::value == DComparisonDegree::none) );

    // ELEMENTS cap: declares all five, but DElemEq supports only == -> equality
    D_ER_CHECK( (container_comparison_degree_of<DCmpTotalEqelem>::value == DComparisonDegree::equality) );

    // a non-container declares a full comparison but has no elements -> none
    D_ER_CHECK( (container_comparison_degree_of<int>::value == DComparisonDegree::none) );

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    D_ER_CHECK( (container_comparison_degree_of<cmp_3way>::value       == DComparisonDegree::three_way) );
    D_ER_CHECK( (container_comparison_degree_of<std::vector<int>>::value == DComparisonDegree::three_way) );
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON

    return _er_ok;
}


// tests_is_equality_comparable_container
//   degree at least equality - two of these can actually be == compared.
bool tests_is_equality_comparable_container()
{
    bool _er_ok = true;

    D_ER_CHECK( (is_equality_comparable_container<DCmpEq>::value) );
    D_ER_CHECK( (is_equality_comparable_container<DCmpPartial>::value) );
    D_ER_CHECK( (is_equality_comparable_container<DCmpTotal>::value) );
    D_ER_CHECK( (is_equality_comparable_container<DCmpTotalEqelem>::value) );  // capped at equality, still == comparable
    D_ER_CHECK( (is_equality_comparable_container<std::vector<int>>::value) );

    // no container comparison -> false
    D_ER_CHECK( (!is_equality_comparable_container<box<int>>::value) );
    // the key edge: int declares ==, but has no elements -> NOT actually comparable
    D_ER_CHECK( (!is_equality_comparable_container<int>::value) );

    return _er_ok;
}


// tests_is_ordered_comparable_container
//   degree at least partial_order - two of these can actually be < compared.
bool tests_is_ordered_comparable_container()
{
    bool _er_ok = true;

    D_ER_CHECK( (is_ordered_comparable_container<DCmpPartial>::value) );
    D_ER_CHECK( (is_ordered_comparable_container<DCmpTotal>::value) );
    D_ER_CHECK( (is_ordered_comparable_container<std::vector<int>>::value) );

    // equality-only degree is not ordered-comparable
    D_ER_CHECK( (!is_ordered_comparable_container<DCmpEq>::value) );
    D_ER_CHECK( (!is_ordered_comparable_container<DCmpTotalEqelem>::value) );  // element-capped to equality
    D_ER_CHECK( (!is_ordered_comparable_container<box<int>>::value) );
    D_ER_CHECK( (!is_ordered_comparable_container<int>::value) );

    return _er_ok;
}


NS_END  // testing
NS_END  // djinterp
