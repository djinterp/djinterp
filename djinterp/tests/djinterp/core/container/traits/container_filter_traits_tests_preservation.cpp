/*******************************************************************************
* djinterp [testing]              container_filter_traits_tests_preservation.cpp
*
*   Section IV - preservation, the weight of the axis.  Because selection keeps
* the survivors' arrangement, the result is ordered / sorted / unique exactly
* when the source is; the capacity CEILING always survives (the result is no
* larger); and the capacity FLOOR never does (the constant-false predicate
* empties the container - the single asymmetry).
*
*   These traits are pure reports of the sibling axes, so they are INDEPENDENT
* of whether a type is a filter source: filter_preserves_order tracks
* is_ordered_container even for a non-source (iterable_no_value is ordered).
* The sortedness case leans on the sortedness axis's precision - a std::set is
* `monotone`, NOT `sorted`, so it does NOT preserve sortedness; only a type that
* actually guarantees comparator order (sorted_seq, via its opt-in invariant)
* does.  The keyed/associative containers do not preserve order (they are
* unordered in the positional sense) but the unique ones preserve uniqueness.
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests_preservation.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_filter_preserves_order
//   tracks is_ordered_container: positional sequences preserve order; keyed /
//   associative containers do not.
bool
tests_filter_preserves_order()
{
    bool _cf_ok = true;

    // positional sequences: order preserved
    D_CF_CHECK( (filter_preserves_order<std::vector<int>>::value) );
    D_CF_CHECK( (filter_preserves_order<std::deque<int>>::value) );
    D_CF_CHECK( (filter_preserves_order<std::list<int>>::value) );
    D_CF_CHECK( (filter_preserves_order<std::forward_list<int>>::value) );
    D_CF_CHECK( (filter_preserves_order<std::array<int, 4>>::value) );
    D_CF_CHECK( (filter_preserves_order<std::string>::value) );
    D_CF_CHECK( (filter_preserves_order<native_filterable>::value) );
    D_CF_CHECK( (filter_preserves_order<sorted_seq>::value) );

    // keyed / associative: unordered in the positional sense
    D_CF_CHECK( (!filter_preserves_order<std::set<int>>::value) );
    D_CF_CHECK( (!filter_preserves_order<std::multiset<int>>::value) );
    D_CF_CHECK( (!filter_preserves_order<std::unordered_set<int>>::value) );
    D_CF_CHECK( (!filter_preserves_order<std::map<int, int>>::value) );

    // independence from source-hood: an ordered NON-source still reports ordered,
    // while a non-iterable type does not
    D_CF_CHECK( (filter_preserves_order<iterable_no_value>::value) );
    D_CF_CHECK( (!filter_preserves_order<value_only>::value) );

    return _cf_ok;
}


// tests_filter_preserves_sortedness
//   tracks is_sorted_container: only a guaranteed-comparator-order type; a
//   std::set is `monotone`, not `sorted`.
bool
tests_filter_preserves_sortedness()
{
    bool _cf_ok = true;

    // the only positive: an opt-in sorted invariant
    D_CF_CHECK( (filter_preserves_sortedness<sorted_seq>::value) );

    // sequences are order-dependent, not sorted
    D_CF_CHECK( (!filter_preserves_sortedness<std::vector<int>>::value) );
    D_CF_CHECK( (!filter_preserves_sortedness<std::list<int>>::value) );
    D_CF_CHECK( (!filter_preserves_sortedness<native_filterable>::value) );
    // associatives enumerate monotonically but are not `sorted` on this axis
    D_CF_CHECK( (!filter_preserves_sortedness<std::set<int>>::value) );
    D_CF_CHECK( (!filter_preserves_sortedness<std::multiset<int>>::value) );
    D_CF_CHECK( (!filter_preserves_sortedness<std::map<int, int>>::value) );
    D_CF_CHECK( (!filter_preserves_sortedness<std::unordered_set<int>>::value) );

    return _cf_ok;
}


// tests_filter_preserves_uniqueness
//   tracks is_unique_container: multiplicity can only fall, so uniqueness is kept.
bool
tests_filter_preserves_uniqueness()
{
    bool _cf_ok = true;

    // unique associatives
    D_CF_CHECK( (filter_preserves_uniqueness<std::set<int>>::value) );
    D_CF_CHECK( (filter_preserves_uniqueness<std::unordered_set<int>>::value) );
    D_CF_CHECK( (filter_preserves_uniqueness<std::map<int, int>>::value) );

    // sequences and multisets are not unique
    D_CF_CHECK( (!filter_preserves_uniqueness<std::vector<int>>::value) );
    D_CF_CHECK( (!filter_preserves_uniqueness<std::multiset<int>>::value) );
    D_CF_CHECK( (!filter_preserves_uniqueness<std::array<int, 4>>::value) );
    D_CF_CHECK( (!filter_preserves_uniqueness<std::string>::value) );
    // forward_list has no size() -> uncountable -> not unique
    D_CF_CHECK( (!filter_preserves_uniqueness<std::forward_list<int>>::value) );
    D_CF_CHECK( (!filter_preserves_uniqueness<sorted_seq>::value) );

    return _cf_ok;
}


// tests_filter_preserves_capacity_bounds
//   the ceiling always survives; the floor never does - both unconditional, so
//   they read the same for filterable, input-only, and non-source types alike.
bool
tests_filter_preserves_capacity_bounds()
{
    bool _cf_ok = true;

    // ceiling: unconditionally true
    D_CF_CHECK( (filter_preserves_capacity_ceiling<std::vector<int>>::value) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling<std::array<int, 4>>::value) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling<std::set<int>>::value) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling<std::forward_list<int>>::value) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling<native_filterable>::value) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling<int>::value) );   // even a non-container

    // floor: unconditionally false
    D_CF_CHECK( (!filter_preserves_capacity_floor<std::vector<int>>::value) );
    D_CF_CHECK( (!filter_preserves_capacity_floor<std::array<int, 4>>::value) );
    D_CF_CHECK( (!filter_preserves_capacity_floor<std::set<int>>::value) );
    D_CF_CHECK( (!filter_preserves_capacity_floor<std::forward_list<int>>::value) );
    D_CF_CHECK( (!filter_preserves_capacity_floor<native_filterable>::value) );
    D_CF_CHECK( (!filter_preserves_capacity_floor<int>::value) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
