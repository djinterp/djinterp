/*******************************************************************************
* djinterp [testing]                    element_relation_traits_tests_degree.cpp
*
*   Section III - the comparison degree, the strongest rung a type reaches:
* none < equality < partial_order < total_order < three_way.  comparison_degree_name
* spells it; comparison_degree_rank is the integer strength (the enum is declared in
* order, so the cast is the rank); comparison_degree_weaker returns the strongest
* rung BOTH inputs reach.  element_comparison_degree_of resolves the rung for a
* container's value_type.
*
*   The custom element types map one-to-one onto the ladder (none / equality /
* partial_order / total_order), with three_way reserved for <=> under C++20; a
* non-container resolves to none.  The pure degree functions are enum-level and so
* are exercised unconditionally, three_way included.
*
*
* path:      /tests/djinterp/core/container/traits/element_relation_traits_tests_degree.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "element_relation_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_comparison_degree_name
//   the stable spellings.
bool
tests_comparison_degree_name()
{
    bool _er_ok = true;

    D_ER_CHECK( (er_cstr_eq(comparison_degree_name(comparison_degree::none),          "none")) );
    D_ER_CHECK( (er_cstr_eq(comparison_degree_name(comparison_degree::equality),      "equality")) );
    D_ER_CHECK( (er_cstr_eq(comparison_degree_name(comparison_degree::partial_order), "partial_order")) );
    D_ER_CHECK( (er_cstr_eq(comparison_degree_name(comparison_degree::total_order),   "total_order")) );
    D_ER_CHECK( (er_cstr_eq(comparison_degree_name(comparison_degree::three_way),     "three_way")) );

    D_ER_CHECK( (!er_cstr_eq(comparison_degree_name(comparison_degree::equality),
                             "partial_order")) );

    return _er_ok;
}


// tests_comparison_degree_rank_and_weaker
//   rank is the monotone strength; weaker picks the lower-ranked of two.
bool
tests_comparison_degree_rank_and_weaker()
{
    bool _er_ok = true;

    // rank is strictly increasing along the declared order
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::none)          == 0) );
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::equality)      == 1) );
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::partial_order) == 2) );
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::total_order)   == 3) );
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::three_way)     == 4) );

    // monotonicity spot-check
    D_ER_CHECK( (comparison_degree_rank(comparison_degree::equality)
                     < comparison_degree_rank(comparison_degree::three_way)) );

    // weaker: the lower-ranked wins, either argument order
    D_ER_CHECK( (comparison_degree_weaker(comparison_degree::equality, comparison_degree::total_order)
                     == comparison_degree::equality) );
    D_ER_CHECK( (comparison_degree_weaker(comparison_degree::total_order, comparison_degree::equality)
                     == comparison_degree::equality) );
    D_ER_CHECK( (comparison_degree_weaker(comparison_degree::three_way, comparison_degree::none)
                     == comparison_degree::none) );
    D_ER_CHECK( (comparison_degree_weaker(comparison_degree::none, comparison_degree::three_way)
                     == comparison_degree::none) );
    // idempotent on equal inputs
    D_ER_CHECK( (comparison_degree_weaker(comparison_degree::partial_order, comparison_degree::partial_order)
                     == comparison_degree::partial_order) );

    return _er_ok;
}


// tests_element_comparison_degree_of
//   the rung a container's value_type reaches.
bool
tests_element_comparison_degree_of()
{
    bool _er_ok = true;

    D_ER_CHECK( (element_comparison_degree_of<box<elem_none>>::value  == comparison_degree::none) );
    D_ER_CHECK( (element_comparison_degree_of<box<elem_eq>>::value    == comparison_degree::equality) );
    D_ER_CHECK( (element_comparison_degree_of<box<elem_lt>>::value    == comparison_degree::partial_order) );
    D_ER_CHECK( (element_comparison_degree_of<box<elem_total>>::value == comparison_degree::total_order) );

    // a non-container: no elements -> none
    D_ER_CHECK( (element_comparison_degree_of<int>::value == comparison_degree::none) );

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    // under C++20 the <=> rung is reached by three-way-capable elements
    D_ER_CHECK( (element_comparison_degree_of<box<elem_3way>>::value == comparison_degree::three_way) );
    D_ER_CHECK( (element_comparison_degree_of<box<int>>::value       == comparison_degree::three_way) );
    D_ER_CHECK( (element_comparison_degree_of<std::vector<int>>::value == comparison_degree::three_way) );
#else
    // below C++20 int reaches only the full relational set
    D_ER_CHECK( (element_comparison_degree_of<box<int>>::value == comparison_degree::total_order) );
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON

    return _er_ok;
}


NS_END  // testing
NS_END  // djinterp
