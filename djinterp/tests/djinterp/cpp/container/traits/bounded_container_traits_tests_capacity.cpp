/*******************************************************************************
* djinterp [cpp]                     bounded_container_traits_tests_capacity.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/bounded_container_traits_tests_capacity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <list>
#include <tuple>
#include <utility>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_bounded_extent_disjunct
  is_bounded_container, first disjunct: a static `extent`.
  Tests the following:
  - an extent alone promotes, with no other signal present
  - an extent of ZERO promotes (the empty container is bounded)
  - the promotion survives every shape the extent probe accepts, which
    includes shapes it should not accept (see tests_signal_fixed_extent_shapes):
    a plain data member named `extent` is enough to make a type "bounded"
*/
bool tests_bounded_extent_disjunct()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<extent_static_member>::value);
    D_BCT_CHECK(is_bounded_container<extent_zero>::value);

    // the disjunct inherits the probe's false positives wholesale
    D_BCT_CHECK(is_bounded_container<extent_data_member>::value);
    D_BCT_CHECK(is_bounded_container<extent_static_function>::value);
    D_BCT_CHECK(is_bounded_container<extent_enumerator>::value);

    // and its correct declinings
    D_BCT_CHECK(!is_bounded_container<extent_member_function>::value);
    D_BCT_CHECK(!is_bounded_container<extent_nested_type>::value);
    D_BCT_CHECK(!is_bounded_container<extent_private>::value);

    return ok;
}

/*
tests_bounded_tuple_size_disjunct
  is_bounded_container, second disjunct: a std::tuple_size specialization.
  Tests the following:
  - promotes std::array, std::tuple, std::pair and any conforming user type
  - promotes the ZERO-length array and the EMPTY tuple
  - does not promote the growable containers, whose tuple_size is the
    undefined primary template
*/
bool tests_bounded_tuple_size_disjunct()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(is_bounded_container<std::array<int, 0>>::value);
    D_BCT_CHECK(is_bounded_container<std::tuple<int, char>>::value);
    D_BCT_CHECK(is_bounded_container<std::tuple<>>::value);
    D_BCT_CHECK(is_bounded_container<std::pair<int, char>>::value);
    D_BCT_CHECK(is_bounded_container<DTupleSizeConformer>::value);

    D_BCT_CHECK(!is_bounded_container<std::vector<int>>::value);
    D_BCT_CHECK(!is_bounded_container<std::list<int>>::value);

    return ok;
}

/*
tests_bounded_static_bounds_disjunct
  is_bounded_container, third disjunct: static interval endpoints.
  Tests the following:
  - both endpoints promote; a lone endpoint does not
  - promotion holds even when the type is also demonstrably growable - the
    third disjunct fires and the fourth is never consulted
*/
bool tests_bounded_static_bounds_disjunct()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<DBoundsBoth>::value);
    D_BCT_CHECK(is_bounded_container<DIntervalContainer>::value);
    D_BCT_CHECK(is_bounded_container<DBoundsAndGrowable>::value);

    D_BCT_CHECK(!is_bounded_container<DBoundsLowerOnly>::value);
    D_BCT_CHECK(!is_bounded_container<DBoundsUpperOnly>::value);
    D_BCT_CHECK(!is_bounded_container<DBoundsPrivate>::value);

    return ok;
}

/*
tests_bounded_capacity_disjunct
  is_bounded_container, fourth disjunct: `capacity() && !reserve()`.
  Both halves of the conjunction, in both polarities.
  Tests the following:
  - capacity without reserve promotes
  - capacity WITH reserve does not (this is what keeps std::vector unbounded)
  - reserve without capacity does not promote either - the disjunct needs a
    capacity() to be about
  - a capacity that is not const-callable is not seen at all, so it cannot
    promote
  - EDGE: a growable type whose reserve takes no argument is promoted, because
    the anti-signal probes reserve(size_t) and misses reserve().  A false
    positive, pinned here.  Tightening it means probing for reserve callability
    with any argument, or with the type's own size_type
*/
bool tests_bounded_capacity_disjunct()
{
    bool ok = true;

    // capacity && !reserve  ->  bounded
    D_BCT_CHECK(is_bounded_container<DCapacityConst>::value);
    D_BCT_CHECK(is_bounded_container<DFixedCapacity>::value);

    // capacity && reserve  ->  NOT bounded
    D_BCT_CHECK(!is_bounded_container<DGrowable>::value);
    D_BCT_CHECK(!is_bounded_container<std::vector<int>>::value);

    // !capacity && reserve  ->  NOT bounded
    D_BCT_CHECK(!is_bounded_container<DReserveSized>::value);
    D_BCT_CHECK(!is_bounded_container<DReserveWithoutCapacity>::value);

    // !capacity && !reserve  ->  NOT bounded (nothing to promote)
    D_BCT_CHECK(!is_bounded_container<DSizeConst>::value);
    D_BCT_CHECK(!is_bounded_container<no_signal>::value);

    // a non-const capacity() is invisible, so it cannot promote
    D_BCT_CHECK(!is_bounded_container<DCapacityNonconst>::value);

    // EDGE: nullary reserve slips past the anti-signal -> false positive
    D_BCT_CHECK(is_bounded_container<DGrowableNullaryReserve>::value);

    return ok;
}

/*
tests_bounded_negative
  The all-zeros row and its neighbours: types with no promoting evidence at all.
  Tests the following:
  - an empty type is not bounded
  - a type with only size(), only max_size(), or only reserve() is not bounded
  - max_size() in particular never promotes, on either kind of container
*/
bool tests_bounded_negative()
{
    bool ok = true;

    D_BCT_CHECK(!is_bounded_container<no_signal>::value);
    D_BCT_CHECK(!is_bounded_container<DSizeConst>::value);
    D_BCT_CHECK(!is_bounded_container<DSizeNonconst>::value);
    D_BCT_CHECK(!is_bounded_container<DMaxSizeOnly>::value);
    D_BCT_CHECK(!is_bounded_container<DSizedAndMaxed>::value);
    D_BCT_CHECK(!is_bounded_container<DReserveSized>::value);
    D_BCT_CHECK(!is_bounded_container<DReserveNullary>::value);

    return ok;
}

/*
tests_bounded_ignores_size
  is_bounded_container is deliberately NOT gated on size().
  Tests the following:
  - a bound with no size() is still a bound: an interval, a bare extent and a
    std::tuple are all bounded, and none of them presents a size()
  - the consequence: `is_bounded_container<T>` alone does not assert that T is
    a container.  A caller who needs "bounded AND a container" must conjoin
    has_size_signal - which is exactly what capacity_bound_of does
*/
bool tests_bounded_ignores_size()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<DBoundsBoth>::value);
    D_BCT_CHECK(!has_size_signal<DBoundsBoth>::value);

    D_BCT_CHECK(is_bounded_container<extent_static_member>::value);
    D_BCT_CHECK(!has_size_signal<extent_static_member>::value);

    D_BCT_CHECK(is_bounded_container<std::tuple<int, char>>::value);
    D_BCT_CHECK(!has_size_signal<std::tuple<int, char>>::value);

    D_BCT_CHECK(is_bounded_container<DTupleSizeConformer>::value);
    D_BCT_CHECK(!has_size_signal<DTupleSizeConformer>::value);

    return ok;
}

/*
tests_bounded_first_signal_wins
  The disjunction is not a vote.
  Tests the following:
  - a type with a static extent AND a growable capacity is bounded: the first
    promoting signal decides, and the growability anti-signal only ever guards
    the FOURTH disjunct, never the first three
  - the same for static bounds plus growability
  - the all-signals fixture, which fires every probe at once, is bounded
*/
bool tests_bounded_first_signal_wins()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<extent_and_growable>::value);
    D_BCT_CHECK(has_reserve_signal<extent_and_growable>::value);

    D_BCT_CHECK(is_bounded_container<DBoundsAndGrowable>::value);
    D_BCT_CHECK(has_reserve_signal<DBoundsAndGrowable>::value);

    D_BCT_CHECK(is_bounded_container<DEverySignal>::value);
    D_BCT_CHECK(has_reserve_signal<DEverySignal>::value);

    return ok;
}

/*
tests_unbounded_truth_table
  is_unbounded_container = has_size_signal && !is_bounded_container.
  All four rows.
  Tests the following:
  - size && !bounded   -> unbounded   (std::vector, std::list, DSizeConst)
  - size && bounded    -> NOT         (std::array, DIntervalContainer)
  - !size && !bounded  -> NOT         (an empty type is not unbounded, it is
                                       unclassified)
  - !size && bounded   -> NOT         (a bare extent, a std::tuple)
  - a non-const size() does not open the guard, so DSizeNonconst is neither
*/
bool tests_unbounded_truth_table()
{
    bool ok = true;

    // size && !bounded
    D_BCT_CHECK(is_unbounded_container<DSizeConst>::value);
    D_BCT_CHECK(is_unbounded_container<DSizedAndMaxed>::value);
    D_BCT_CHECK(is_unbounded_container<DGrowable>::value);
    D_BCT_CHECK(is_unbounded_container<DReserveWithoutCapacity>::value);
    D_BCT_CHECK(is_unbounded_container<std::vector<int>>::value);

    // size && bounded
    D_BCT_CHECK(!is_unbounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(!is_unbounded_container<DIntervalContainer>::value);
    D_BCT_CHECK(!is_unbounded_container<DFixedCapacity>::value);

    // !size && !bounded
    D_BCT_CHECK(!is_unbounded_container<no_signal>::value);
    D_BCT_CHECK(!is_unbounded_container<DMaxSizeOnly>::value);
    D_BCT_CHECK(!is_unbounded_container<DSizeNonconst>::value);
    D_BCT_CHECK(!is_unbounded_container<DReserveSized>::value);

    // !size && bounded
    D_BCT_CHECK(!is_unbounded_container<extent_static_member>::value);
    D_BCT_CHECK(!is_unbounded_container<DBoundsBoth>::value);
    D_BCT_CHECK(!is_unbounded_container<std::tuple<int, char>>::value);

    return ok;
}

/*
tests_bounded_unbounded_exclusive
  The two verdicts are mutually exclusive but NOT exhaustive.
  Tests the following:
  - no type is both bounded and unbounded - by construction, since unbounded
    conjoins !bounded, but it is the invariant every caller assumes, so it is
    checked across the whole fixture set rather than argued
  - types exist that are NEITHER: anything without a size() and without a bound.
    A caller writing `if (bounded) ... else ...` is therefore wrong; the third
    state is what capacity_bound::unknown is for
*/
bool tests_bounded_unbounded_exclusive()
{
    bool ok = true;

    // exclusive: (bounded && unbounded) holds for nothing
    D_BCT_CHECK(!(is_bounded_container<std::array<int, 3>>::value &&
                  is_unbounded_container<std::array<int, 3>>::value));
    D_BCT_CHECK(!(is_bounded_container<std::vector<int>>::value &&
                  is_unbounded_container<std::vector<int>>::value));
    D_BCT_CHECK(!(is_bounded_container<DIntervalContainer>::value &&
                  is_unbounded_container<DIntervalContainer>::value));
    D_BCT_CHECK(!(is_bounded_container<no_signal>::value &&
                  is_unbounded_container<no_signal>::value));
    D_BCT_CHECK(!(is_bounded_container<DEverySignal>::value &&
                  is_unbounded_container<DEverySignal>::value));

    // and the same, stated over the whole hostile zoo at once: neither verdict
    // fires for any of them, so a fortiori they never fire together
    D_BCT_CHECK((holds_for_none<is_bounded_container,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<is_unbounded_container,
                                D_TEST_HOSTILE_TYPES>::value));

    // NOT exhaustive: the neither-nor types
    D_BCT_CHECK(!is_bounded_container<no_signal>::value);
    D_BCT_CHECK(!is_unbounded_container<no_signal>::value);
    D_BCT_CHECK(!is_bounded_container<DMaxSizeOnly>::value);
    D_BCT_CHECK(!is_unbounded_container<DMaxSizeOnly>::value);

    return ok;
}

/*
tests_capacity_value_companions
  The `_v` companions of the two capacity verdicts.
  Tests the following:
  - is_bounded_container_v and is_unbounded_container_v track their traits on
    positive and negative subjects alike
*/
bool tests_capacity_value_companions()
{
    bool ok = true;

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_bounded_container,
                                      std::array<int, 3>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_bounded_container, std::vector<int>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_bounded_container, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_unbounded_container,
                                      std::vector<int>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_unbounded_container,
                                      std::array<int, 3>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_unbounded_container, no_signal));

    D_BCT_CHECK(is_bounded_container_v<std::array<int, 3>>);
    D_BCT_CHECK(!is_bounded_container_v<std::vector<int>>);
    D_BCT_CHECK(is_unbounded_container_v<std::vector<int>>);
    D_BCT_CHECK(!is_unbounded_container_v<std::array<int, 3>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
