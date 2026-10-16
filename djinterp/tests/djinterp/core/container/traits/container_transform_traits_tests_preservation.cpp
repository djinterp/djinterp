/*******************************************************************************
* djinterp [testing]           container_transform_traits_tests_preservation.cpp
*
*   Section IV - preservation, the weight of the axis and the exact complement of
* selection's.  Mapping keeps every position and its arrangement but REWRITES the
* value: so it preserves the value-independent anatomy (arrangement and size) and
* may break the value-dependent (sortedness, uniqueness).
*
*   Concretely: transform_preserves_order == is_ordered_container (the result is
* ordered exactly when the source is - true for sequences, false for associatives);
* transform_preserves_size / _capacity_ceiling / _capacity_floor are
* UNCONDITIONALLY true (no count changes, so both the ceiling and the floor
* survive); and transform_preserves_sortedness / _uniqueness are UNCONDITIONALLY
* false (the type cannot constrain an arbitrary map to be monotone or injective).
* The unconditional traits are checked across sequences, associatives, and a
* non-container alike, to confirm they do not depend on the type.
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests_preservation.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_transform_preserves_order
//   equals is_ordered_container: sequences keep order, associatives have none.
bool
tests_transform_preserves_order()
{
    bool _tx_ok = true;

    // sequences (positional identity) preserve order
    D_TX_CHECK( (transform_preserves_order<std::vector<int>>::value) );
    D_TX_CHECK( (transform_preserves_order<std::deque<int>>::value) );
    D_TX_CHECK( (transform_preserves_order<std::list<int>>::value) );
    D_TX_CHECK( (transform_preserves_order<std::forward_list<int>>::value) );
    D_TX_CHECK( (transform_preserves_order<std::array<int, 4>>::value) );
    D_TX_CHECK( (transform_preserves_order<std::string>::value) );
    D_TX_CHECK( (transform_preserves_order<Box<int>>::value) );

    // associatives have no positional order to preserve
    D_TX_CHECK( (!transform_preserves_order<std::set<int>>::value) );
    D_TX_CHECK( (!transform_preserves_order<std::multiset<int>>::value) );
    D_TX_CHECK( (!transform_preserves_order<std::map<int, int>>::value) );
    D_TX_CHECK( (!transform_preserves_order<std::unordered_set<int>>::value) );
    D_TX_CHECK( (!transform_preserves_order<int>::value) );

    return _tx_ok;
}


// tests_transform_preserves_size_and_capacity
//   size, capacity ceiling, and capacity floor are all unconditionally preserved.
bool
tests_transform_preserves_size_and_capacity()
{
    bool _tx_ok = true;

    // size preserved - regardless of shape (a sequence, an associative, a non-container)
    D_TX_CHECK( (transform_preserves_size<std::vector<int>>::value) );
    D_TX_CHECK( (transform_preserves_size<std::set<int>>::value) );
    D_TX_CHECK( (transform_preserves_size<std::array<int, 4>>::value) );
    D_TX_CHECK( (transform_preserves_size<int>::value) );

    // capacity ceiling preserved (size unchanged, so a bound always survives)
    D_TX_CHECK( (transform_preserves_capacity_ceiling<std::vector<int>>::value) );
    D_TX_CHECK( (transform_preserves_capacity_ceiling<std::array<int, 4>>::value) );
    D_TX_CHECK( (transform_preserves_capacity_ceiling<int>::value) );

    // capacity FLOOR preserved too - the axis's key contrast with selection, which
    // can empty a container; mapping keeps every position
    D_TX_CHECK( (transform_preserves_capacity_floor<std::vector<int>>::value) );
    D_TX_CHECK( (transform_preserves_capacity_floor<std::set<int>>::value) );
    D_TX_CHECK( (transform_preserves_capacity_floor<int>::value) );

    return _tx_ok;
}


// tests_transform_preserves_sortedness_and_uniqueness
//   both unconditionally NOT preserved - a map may invert an order or collide values.
bool
tests_transform_preserves_sortedness_and_uniqueness()
{
    bool _tx_ok = true;

    // sortedness not preserved - even a sorted associative
    D_TX_CHECK( (!transform_preserves_sortedness<std::set<int>>::value) );
    D_TX_CHECK( (!transform_preserves_sortedness<std::map<int, int>>::value) );
    D_TX_CHECK( (!transform_preserves_sortedness<std::vector<int>>::value) );
    D_TX_CHECK( (!transform_preserves_sortedness<int>::value) );

    // uniqueness not preserved - even a unique set (a non-injective map may collide)
    D_TX_CHECK( (!transform_preserves_uniqueness<std::set<int>>::value) );
    D_TX_CHECK( (!transform_preserves_uniqueness<std::unordered_set<int>>::value) );
    D_TX_CHECK( (!transform_preserves_uniqueness<std::vector<int>>::value) );
    D_TX_CHECK( (!transform_preserves_uniqueness<int>::value) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
