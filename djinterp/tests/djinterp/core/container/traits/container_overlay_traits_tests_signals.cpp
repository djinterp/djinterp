/*******************************************************************************
* djinterp [testing]                  container_overlay_traits_tests_signals.cpp
*
*   Section II - is_keyed_container (the eta restriction): the value type is a
* pair Key x Val, detected by BOTH first_type and second_type aliases.  eta
* asserts only the pair SHAPE, so it is independent of uniqueness - a sequence of
* pairs (std::vector<std::pair<...>>) is keyed yet wears no mu_1.  The
* conjunction is pinned: a value type carrying only first_type is NOT keyed.
*
*   (The sigma signal is an internal opt-in helper with no public trait of its
* own; it is exercised through overlay_of::sorted and is_order_blind_overlay in
* the section-IV translation unit.)
*
*
* path:      /tests/djinterp/core/container/traits/container_overlay_traits_tests_signals.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_overlay_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_keyed_container
//   a pair-shaped value_type (first_type AND second_type).
bool
tests_is_keyed_container()
{
    bool _ovl_ok = true;

    // keyed: pair value_type
    D_OVL_CHECK( (is_keyed_container<std::map<int, int>>::value) );
    D_OVL_CHECK( (is_keyed_container<std::multimap<int, int>>::value) );
    // eta is about SHAPE, not uniqueness: a sequence of pairs is keyed
    D_OVL_CHECK( (is_keyed_container<std::vector<std::pair<int, int>>>::value) );
    D_OVL_CHECK( (is_keyed_container<bounded_multimap_seq>::value) );

    // not keyed: scalar value_type
    D_OVL_CHECK( (!is_keyed_container<std::set<int>>::value) );
    D_OVL_CHECK( (!is_keyed_container<std::multiset<int>>::value) );
    D_OVL_CHECK( (!is_keyed_container<std::unordered_set<int>>::value) );
    D_OVL_CHECK( (!is_keyed_container<std::vector<int>>::value) );
    D_OVL_CHECK( (!is_keyed_container<std::array<int, 4>>::value) );

    // the conjunction: first_type present but second_type absent -> NOT keyed
    D_OVL_CHECK( (!is_keyed_container<keyed_half>::value) );

    // no value_type at all
    D_OVL_CHECK( (!is_keyed_container<int>::value) );

    return _ovl_ok;
}


NS_END  // testing
NS_END  // djinterp
