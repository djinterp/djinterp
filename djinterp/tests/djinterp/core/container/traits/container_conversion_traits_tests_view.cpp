/*******************************************************************************
* djinterp [core]                     container_conversion_traits_tests_view.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_view.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// view_of
//   alias: the VIEW predicate, named once.  It lives in internal:: because it is a
// helper, and it is tested directly anyway - it is an eight-way conjunction, the
// most falsifiable thing in the module, and one of its eight clauses is
// unreachable through the public trait.  A helper that cannot be reached is a
// helper that cannot be tested, and this is how it gets tested.
template<typename _From, typename _To>
using view_of = ::djinterp::internal::is_reinterpretation_helper<_From, _To>;


/*
tests_view_baseline
  THE BASELINE.  view_from -> view_to is a VIEW.

  Two DISTINCT types, identical on every one of the eight things the predicate
  reads: the same elements, the same content rung, the same topology, the same
  backing layout, the same lifetime, the same storage, the same iteration
  capability, the same discipline.  Nothing moves, so nothing needs to.

  Everything else in this TU is this pair with exactly ONE of the eight changed.
  If the baseline is not a view, no lever below proves anything - so it is checked
  first, and its eight clauses are checked one by one, so that a failure names the
  clause that broke rather than the conjunction that contains it.

  Tests the following:
  - the pair is a view, through the helper and through the public tier
  - each of the eight clauses holds, individually
  - a view is a view even though a CONSTRUCTIVE PATH also exists.  view_to has a
    range constructor; the ladder tests the view at rung 4 and the construction at
    rung 5, so the lesser concern wins.  Without this check the whole rung-ordering
    could be inverted and every test in the suite would still pass
*/
bool
tests_view_baseline()
{
    bool ok = true;

    // *** the baseline ***
    D_CVT_CHECK((view_of<view_from, view_to>::value));
    D_CVT_TIER(conversion_tier::view, view_from, view_to);

    // 1. the same elements
    D_CVT_CHECK((elements_same_type<view_from, view_to>::value));

    // 2. the same content rung
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<view_to>::value));
    D_CVT_LEVEL(content_level::seq, native_content_level_of<view_from>::value);

    // 3. no topology change
    D_CVT_CHECK((!conversion_changes_topology<view_from, view_to>::value));

    // 4. the same backing layout
    D_CVT_CHECK((sequential_layout_of<view_from>::value ==
                 sequential_layout_of<view_to>::value));
    D_CVT_CHECK(sequential_layout_of<view_from>::value ==
                sequential_layout::array_like);

    // 5, 6, 7. every realization axis EQUIVALENT
    D_CVT_CHECK((lifetime_relation<view_from, view_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<view_from, view_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<view_from, view_to>::value ==
                 order_relation::equivalent));

    // 8. the discipline no weaker
    D_CVT_CHECK((discipline_relation<view_from, view_to>::value ==
                 order_relation::equivalent));

    // *** THE RUNG ORDERING.  A construction path EXISTS, and the pair is still a
    // view - because a reinterpretation is the LESSER concern, and rung 4 beats
    // rung 5.  Invert the two and every other check in this suite still passes
    D_CVT_CHECK((has_constructive_path<view_from, view_to>::value));
    D_CVT_TIER(conversion_tier::view, view_from, view_to);
    D_CVT_CHECK((is_view_conversion<view_from, view_to>::value));
    D_CVT_CHECK((!is_lossy_conversion<view_from, view_to>::value));

    // and a type is a view of itself
    D_CVT_TIER(conversion_tier::view, view_from, view_from);
    D_CVT_TIER(conversion_tier::view, std_vector, std_vector);
    D_CVT_TIER(conversion_tier::view, std_set, std_set);

    return ok;
}

/*
tests_view_conjunct_elements
  CONJUNCT 1: elements_same_type.

  A CONVERTING element copy is not a reinterpretation - the bits have to change.
  So the predicate demands the elements be the SAME, and not merely compatible,
  which is exactly the distinction elements_compatible was careful to keep.

  Tests the following:
  - the pair is still elements-COMPATIBLE, so the ladder does not reject it at rung
    2 - it falls through to rung 4, the view fails on conjunct 1 alone, and rung 5
    catches it as `constructive`
  - the other seven clauses all hold, so the failure is attributable
  - and the same, on the real thing: vector<int> -> vector<long>
*/
bool
tests_view_conjunct_elements()
{
    bool ok = true;

    // the conjunct fails
    D_CVT_CHECK((!elements_same_type<view_from, lever_elements>::value));
    D_CVT_CHECK((!view_of<view_from, lever_elements>::value));

    // ...and the other seven hold
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<lever_elements>::value));
    D_CVT_CHECK((!conversion_changes_topology<view_from,
                                              lever_elements>::value));
    D_CVT_CHECK((sequential_layout_of<view_from>::value ==
                 sequential_layout_of<lever_elements>::value));
    D_CVT_CHECK((lifetime_relation<view_from, lever_elements>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<view_from, lever_elements>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<view_from, lever_elements>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<view_from, lever_elements>::value ==
                 order_relation::equivalent));

    // so the tier drops exactly one rung: view -> constructive
    D_CVT_CHECK((elements_compatible<view_from, lever_elements>::value));
    D_CVT_CHECK((has_constructive_path<view_from, lever_elements>::value));
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_elements);

    // the same, on the real thing
    D_CVT_CHECK((!elements_same_type<std_vector, std_vector_long>::value));
    D_CVT_CHECK((elements_convertible<std_vector, std_vector_long>::value));
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_vector_long);

    return ok;
}

/*
tests_view_conjunct_content
  CONJUNCT 2: the native content rungs are equal.

  Isolating this one needs its own baseline, and the reason is instructive.  The
  clause fails when the two rungs DIFFER - but a To that is strictly COARSER is
  caught at rung 3 as `lossy`, before the view is ever consulted.  So the difference
  has to be in the other direction: To must be strictly FINER.

  The baseline sequence is at rung `seq` (3), and the only finer FLAT rung is `str`
  (4), which is hierarchical and would trip rung 1 as `structural`.  So the pair
  starts one level down: content_from is a SET (rank 1) and content_to a BAG (rank
  2).  To is finer, nothing is forgotten, and From's discipline is the tighter, so
  conjunct 8 passes too.

  Tests the following:
  - the rungs differ, and nothing is forgotten
  - the other seven clauses hold
  - so the tier is `constructive`, and NOT `lossy`
  - the reverse pair - bag -> set - IS lossy, which is what makes the direction of
    the check visible
*/
bool
tests_view_conjunct_content()
{
    bool ok = true;

    // the rungs differ...
    D_CVT_LEVEL(content_level::set,
                native_content_level_of<content_from>::value);
    D_CVT_LEVEL(content_level::bag,
                native_content_level_of<content_to>::value);
    D_CVT_CHECK((native_content_level_of<content_from>::value !=
                 native_content_level_of<content_to>::value));

    // ...and NOTHING is forgotten, because To is the FINER
    D_CVT_CHECK((!conversion_forgets_content<content_from,
                                             content_to>::value));
    D_CVT_CHECK((!conversion_clamps_capacity<content_from,
                                             content_to>::value));

    // the conjunct fails
    D_CVT_CHECK((!view_of<content_from, content_to>::value));

    // ...and the other seven hold
    D_CVT_CHECK((elements_same_type<content_from, content_to>::value));
    D_CVT_CHECK((!conversion_changes_topology<content_from,
                                              content_to>::value));
    D_CVT_CHECK((sequential_layout_of<content_from>::value ==
                 sequential_layout_of<content_to>::value));
    D_CVT_CHECK((lifetime_relation<content_from, content_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<content_from, content_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<content_from, content_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<content_from, content_to>::value ==
                 order_relation::greater));   // From is TIGHTER: clause 8 admits it

    // so the tier is constructive, not lossy
    D_CVT_TIER(conversion_tier::constructive, content_from, content_to);

    // ...and the REVERSE is lossy, which is what makes the direction visible
    D_CVT_CHECK((conversion_forgets_content<content_to,
                                            content_from>::value));
    D_CVT_TIER(conversion_tier::lossy, content_to, content_from);

    // the same, on the real thing: set -> multiset is a faithful build, and
    // multiset -> set is deduplication
    D_CVT_TIER(conversion_tier::constructive, std_set, std_multiset);
    D_CVT_TIER(conversion_tier::lossy, std_multiset, std_set);

    return ok;
}

/*
tests_view_conjunct_topology
  CONJUNCT 3: !conversion_changes_topology.

  DEAD CODE - within conversion_tier_of, this clause can never be the deciding one.
  The ladder tests the topology at rung 1 and returns `structural`; by the time rung
  4 evaluates the view predicate, !conversion_changes_topology is guaranteed true.

  It is not a bug - the clause is CORRECT, and the helper is correct standing alone.
  It is redundancy, and it is worth knowing about for two reasons: a reader tracing
  a `structural` verdict will look for it in the wrong place, and anyone who
  reorders the ladder will silently make it live again.

  It also means the clause can only be tested by naming the helper directly, which
  is what this test does - and which is why view_of exists at the top of this file.

  Tests the following:
  - the helper, named directly, reports false for a topology-crossing pair
  - the clause is the ONLY one that fails for it, so the helper really is deciding
    on the topology
  - and the public tier never reaches it: conversion_tier_of returns `structural`,
    at rung 1
  - the clause is therefore vacuous at rung 4, restated as an implication: for every
    pair the tier reaches the view check on, the topology is unchanged
*/
bool
tests_view_conjunct_topology()
{
    bool ok = true;

    // the clause fails, in the helper
    D_CVT_CHECK((conversion_changes_topology<view_from,
                                             lever_topology>::value));
    D_CVT_CHECK((!view_of<view_from, lever_topology>::value));

    // ...but the PUBLIC tier never gets there
    D_CVT_TIER(conversion_tier::structural, view_from, lever_topology);
    D_CVT_TIER(conversion_tier::structural, lever_topology, view_from);
    D_CVT_TIER(conversion_tier::structural, std_vector, std_vec_vec);
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_vector);

    // the clause is VACUOUS at rung 4: whenever the tier is not `structural`, the
    // topology is unchanged, so !changes_topology holds by construction
    #define D_CVT_TOPO_VACUOUS(FROM, TO)                                      \
        D_CVT_CHECK(((conversion_tier_of<FROM, TO>::value ==                  \
                      conversion_tier::structural)) ||                        \
                    (!conversion_changes_topology<FROM, TO>::value))

    D_CVT_TOPO_VACUOUS(view_from,  view_to);
    D_CVT_TOPO_VACUOUS(view_from,  lever_topology);
    D_CVT_TOPO_VACUOUS(view_from,  lever_elements);
    D_CVT_TOPO_VACUOUS(std_vector, std_list);
    D_CVT_TOPO_VACUOUS(std_vector, std_vec_vec);
    D_CVT_TOPO_VACUOUS(std_vec_vec, std_set);
    D_CVT_TOPO_VACUOUS(int,        std_vector);

    #undef D_CVT_TOPO_VACUOUS

    // and the structural rung beats EVERYTHING below it, including `none`: a
    // topology-crossing pair with INCOMPATIBLE elements is still `structural`, not
    // `none`.  The header says why - a topology change relates the LEAVES, not the
    // top-level elements, so it stands apart from element compatibility - and this
    // is the check
    D_CVT_CHECK((!elements_compatible<std_vec_vec, std_vector>::value));
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_vector);

    return ok;
}

/*
tests_view_conjunct_layout
  CONJUNCT 4: the backing layouts are equal.

  THE MOST VALUABLE CLAUSE IN THE PREDICATE, and the one the header's own RELIABILITY
  note exists for.  A std::vector and a std::list share a comparison profile
  ENTIRELY - same elements, same content rung, same discipline, same lifetime, same
  storage, same iterability.  Every other clause of the view predicate passes for
  them.  Without conjunct 4, vector -> list would be classified a ZERO-COST VIEW.

  That is the single most expensive wrong answer this module could give, and the
  layout check is the only thing standing between the model and it.

  Tests the following:
  - vector and list agree on every OTHER clause - checked one by one, because the
    claim is the point
  - and disagree on the layout, so the view fails and the tier is `constructive`
  - the fixture pair does the same, with the layout as the sole lever
  - deque is a third layout again, so vector -> deque is also constructive
*/
bool
tests_view_conjunct_layout()
{
    bool ok = true;

    // *** vector and list agree on EVERYTHING except the layout ***
    D_CVT_CHECK((elements_same_type<std_vector, std_list>::value));
    D_CVT_CHECK((native_content_level_of<std_vector>::value ==
                 native_content_level_of<std_list>::value));
    D_CVT_CHECK((!conversion_changes_topology<std_vector, std_list>::value));
    D_CVT_CHECK((lifetime_relation<std_vector, std_list>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<std_vector, std_list>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<std_vector, std_list>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<std_vector, std_list>::value ==
                 order_relation::equivalent));

    // ...they are REALIZATION-EQUAL, in the comparison module's own words
    D_CVT_CHECK((realization_equal<std_vector, std_list>::value));

    // ...and ONLY the layout separates them
    D_CVT_CHECK(sequential_layout_of<std_vector>::value ==
                sequential_layout::array_like);
    D_CVT_CHECK(sequential_layout_of<std_list>::value ==
                sequential_layout::list_like);
    D_CVT_CHECK((sequential_layout_of<std_vector>::value !=
                 sequential_layout_of<std_list>::value));

    // so the view fails, and the conversion is a COPY
    D_CVT_CHECK((!view_of<std_vector, std_list>::value));
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_list);
    D_CVT_TIER(conversion_tier::constructive, std_list, std_vector);

    // the fixture pair, with the layout as the sole lever
    D_CVT_CHECK((sequential_layout_of<view_from>::value !=
                 sequential_layout_of<lever_layout>::value));
    D_CVT_CHECK((!view_of<view_from, lever_layout>::value));
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_layout);

    // ...and the other seven clauses hold for it
    D_CVT_CHECK((elements_same_type<view_from, lever_layout>::value));
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<lever_layout>::value));
    D_CVT_CHECK((lifetime_relation<view_from, lever_layout>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<view_from, lever_layout>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<view_from, lever_layout>::value ==
                 order_relation::equivalent));

    // a deque is a third layout, so it too is a copy
    D_CVT_CHECK(sequential_layout_of<std_deque>::value ==
                sequential_layout::deque_like);
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_deque);

    // a string is a fourth, and vector<char> -> string would be a copy for the same
    // reason - the layout proxy is what carries the whole distinction
    D_CVT_CHECK(sequential_layout_of<std_str>::value ==
                sequential_layout::string_like);

    return ok;
}

/*
tests_view_conjunct_lifetime
  CONJUNCT 5: lifetime_relation == equivalent.

  The clause that CANNOT be isolated for an iterable pair.

  The iterability axis reads its compile-stage bit from is_compile_stage_helper.  So
  does the entire lifetime axis.  It is the same bit - which
  container_comparison_traits.hpp's own coupling makes unavoidable - so for two
  ITERABLE types, a differing lifetime necessarily makes the iterability relation
  non-equivalent as well, and conjunct 5 can never be the sole failure.

  It becomes separately reachable only when BOTH types are NON-iterable: two
  non-iterables sit together at the bottom of the iterability lattice, so conjunct 7
  holds by default, and a lifetime tag on one of them breaks conjunct 5 alone.

  Tests the following:
  - the implication, for an iterable pair: lifetime non-equivalent => iterability
    non-equivalent.  So conjunct 5 is REDUNDANT there, and the suite says so
  - the non-iterable pair, where it is not: nc_runtime -> nc_compile fails conjunct
    5 and passes conjunct 7, and is therefore not a view
  - the control: nc_runtime -> nc_runtime2, identical, IS a view - which is the
    finding, and gets its own test
*/
bool
tests_view_conjunct_lifetime()
{
    bool ok = true;

    // *** the coupling, for ITERABLE types: lifetime != equivalent implies
    // iterability != equivalent, so conjunct 5 is never the sole failure there
    #define D_CVT_LIFETIME_IMPLIES_ITER(FROM, TO)                             \
        D_CVT_CHECK(((lifetime_relation<FROM, TO>::value) ==                  \
                     order_relation::equivalent) ||                           \
                    ((iterability_relation<FROM, TO>::value) !=               \
                     order_relation::equivalent))

    D_CVT_LIFETIME_IMPLIES_ITER(std_array,  std_vector);
    D_CVT_LIFETIME_IMPLIES_ITER(std_vector, std_array);
    D_CVT_LIFETIME_IMPLIES_ITER(view_from,  lossy_clamp);
    D_CVT_LIFETIME_IMPLIES_ITER(view_from,  view_to);
    D_CVT_LIFETIME_IMPLIES_ITER(std_vector, std_list);

    #undef D_CVT_LIFETIME_IMPLIES_ITER

    // std::array is the real instance: it is compile-staged where a vector is not,
    // and BOTH clauses therefore fail at once
    D_CVT_CHECK((lifetime_relation<std_array, std_vector>::value ==
                 order_relation::greater));
    D_CVT_CHECK((iterability_relation<std_array, std_vector>::value ==
                 order_relation::greater));
    D_CVT_CHECK((!view_of<std_array, std_vector>::value));

    // ...even though the two share a LAYOUT.  A std::array is contiguous exactly
    // like a std::vector, and array -> vector is still a COPY - which is precisely
    // the case the header's comment on the helper calls out
    D_CVT_CHECK((sequential_layout_of<std_array>::value ==
                 sequential_layout_of<std_vector>::value));
    D_CVT_CHECK((elements_same_type<std_array, std_vector>::value));
    D_CVT_TIER(conversion_tier::constructive, std_array, std_vector);

    // *** the NON-iterable pair, where conjunct 5 stands alone ***
    D_CVT_CHECK(!is_iterable_container<nc_runtime>::value);
    D_CVT_CHECK(!is_iterable_container<nc_compile>::value);

    // conjunct 7 holds - both sit at the bottom of the iterability lattice
    D_CVT_CHECK((iterability_relation<nc_runtime, nc_compile>::value ==
                 order_relation::equivalent));

    // ...and conjunct 5 does NOT
    D_CVT_CHECK((lifetime_relation<nc_runtime, nc_compile>::value !=
                 order_relation::equivalent));

    // so the view fails on clause 5 alone
    D_CVT_CHECK((!view_of<nc_runtime, nc_compile>::value));

    // ...and there is no construction path either, so the tier is `none`
    D_CVT_CHECK((!has_constructive_path<nc_runtime, nc_compile>::value));
    D_CVT_TIER(conversion_tier::none, nc_runtime, nc_compile);

    return ok;
}

/*
tests_view_conjunct_storage
  CONJUNCT 6: storage_relation == equivalent.

  A differing siting betrays a materialization - the data has to be moved from one
  place to another, whatever else is equal.  So the clause demands the two agree,
  and `unknown` against a declared siting is `incomparable`, not agreement.

  Tests the following:
  - the lever declares a static siting where the baseline declares none, so the
    storage relation is incomparable and the clause fails
  - the other seven clauses hold
  - so the tier drops one rung, to `constructive`
  - and the axis is INERT for the standard library - nothing in it declares a siting
    - which means this clause never fires for any real type, and the suite has to
    reach for a fixture to exercise it at all
*/
bool
tests_view_conjunct_storage()
{
    bool ok = true;

    // the clause fails
    D_CVT_CHECK(storage_of<view_from>::value == storage_duration::unknown);
    D_CVT_CHECK(storage_of<lever_storage>::value ==
                storage_duration::static_storage);
    D_CVT_CHECK((storage_relation<view_from, lever_storage>::value ==
                 order_relation::incomparable));
    D_CVT_CHECK((!view_of<view_from, lever_storage>::value));

    // ...and the other seven hold
    D_CVT_CHECK((elements_same_type<view_from, lever_storage>::value));
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<lever_storage>::value));
    D_CVT_CHECK((!conversion_changes_topology<view_from,
                                              lever_storage>::value));
    D_CVT_CHECK((sequential_layout_of<view_from>::value ==
                 sequential_layout_of<lever_storage>::value));
    D_CVT_CHECK((lifetime_relation<view_from, lever_storage>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<view_from, lever_storage>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<view_from, lever_storage>::value ==
                 order_relation::equivalent));

    // so the tier drops one rung
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_storage);

    // *** and the axis is INERT for the standard library.  Nothing in it declares a
    // siting, so every stdlib pair reports `equivalent` on this clause and it never
    // fires - which is why the suite needs a fixture to exercise it at all
    D_CVT_CHECK(storage_of<std_vector>::value == storage_duration::unknown);
    D_CVT_CHECK(storage_of<std_array>::value == storage_duration::unknown);
    D_CVT_CHECK((storage_relation<std_vector, std_array>::value ==
                 order_relation::equivalent));

    return ok;
}

/*
tests_view_conjunct_iterability
  CONJUNCT 7: iterability_relation == equivalent.

  Note which way it points: EQUIVALENT, not "equivalent or greater".  A container
  that can be written through, read as one that cannot, is NOT a view - even though
  the reader is asking for strictly less than the writer offers.

  That is a deliberate asymmetry with conjunct 8, which DOES admit `greater`.  The
  difference is what a "restriction" means on each axis.  Dropping a discipline is
  free: a sorted container read as an unsorted one is still just a read, and nothing
  has to change.  Dropping an ITERATION CAPABILITY is not: a mutable container and a
  const-only one have different interfaces, and reading the first as the second is a
  reinterpretation only if the two interfaces coincide.

  Tests the following:
  - the lever is const-only where the baseline is not, so From is `greater` on the
    axis - and the clause STILL fails, because it demands equivalence
  - the other seven clauses hold
  - so the tier drops one rung, to `constructive`
  - the contrast with conjunct 8 is stated outright, since the asymmetry is the
    point
*/
bool
tests_view_conjunct_iterability()
{
    bool ok = true;

    // the modes differ, and From is the MORE capable
    D_CVT_CHECK(iteration_mode_of<view_from>::value ==
                iteration_mode::non_const);
    D_CVT_CHECK(iteration_mode_of<lever_iterability>::value ==
                iteration_mode::const_only);
    D_CVT_CHECK((iterability_relation<view_from, lever_iterability>::value ==
                 order_relation::greater));

    // ...and the clause fails ANYWAY - it demands equivalence, not "no weaker"
    D_CVT_CHECK((!view_of<view_from, lever_iterability>::value));

    // the other seven hold
    D_CVT_CHECK((elements_same_type<view_from, lever_iterability>::value));
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<lever_iterability>::value));
    D_CVT_CHECK((!conversion_changes_topology<view_from,
                                              lever_iterability>::value));
    D_CVT_CHECK((sequential_layout_of<view_from>::value ==
                 sequential_layout_of<lever_iterability>::value));
    D_CVT_CHECK((lifetime_relation<view_from, lever_iterability>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<view_from, lever_iterability>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<view_from, lever_iterability>::value ==
                 order_relation::equivalent));

    // so the tier drops one rung
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_iterability);

    // *** THE ASYMMETRY, stated.  Clause 7 demands `equivalent`; clause 8 admits
    // `greater`.  Here is a pair where From is `greater` on BOTH axes - and only
    // one of the two clauses forgives it
    D_CVT_CHECK((iterability_relation<view_from, lever_iterability>::value ==
                 order_relation::greater));
    D_CVT_CHECK((!view_of<view_from, lever_iterability>::value));   // NOT forgiven

    D_CVT_CHECK((discipline_relation<lever_discipline, view_from>::value ==
                 order_relation::greater));
    D_CVT_CHECK((view_of<lever_discipline, view_from>::value));      // FORGIVEN

    return ok;
}

/*
tests_view_conjunct_discipline
  CONJUNCT 8: discipline_relation is `equivalent` OR `greater`.

  The one clause that is not an equality, and the header says why: dropping a
  restriction is free.  A From that is STRONGER than To - it bears every invariant
  To asks for, and more - can be read through To's weaker interface without moving a
  byte.  A From that is WEAKER cannot: To asks for an invariant From does not
  guarantee, and establishing it is work.

  Tests the following:
  - `less` fails the clause: lever_discipline bears a sorted invariant view_from does
    not, so From is the weaker, and the view fails
  - the other seven clauses hold
  - so the tier drops one rung, to `constructive`
  - and `greater` PASSES it, which is the whole point - see
    tests_view_discipline_asymmetry
*/
bool
tests_view_conjunct_discipline()
{
    bool ok = true;

    // From is the WEAKER: To asks for a sorted invariant From does not bear
    D_CVT_CHECK(overlay_of<lever_discipline>::sorted);
    D_CVT_CHECK(!overlay_of<view_from>::sorted);
    D_CVT_CHECK((discipline_relation<view_from, lever_discipline>::value ==
                 order_relation::less));

    // ...so the clause fails
    D_CVT_CHECK((!view_of<view_from, lever_discipline>::value));

    // the other seven hold
    D_CVT_CHECK((elements_same_type<view_from, lever_discipline>::value));
    D_CVT_CHECK((native_content_level_of<view_from>::value ==
                 native_content_level_of<lever_discipline>::value));
    D_CVT_CHECK((!conversion_changes_topology<view_from,
                                              lever_discipline>::value));
    D_CVT_CHECK((sequential_layout_of<view_from>::value ==
                 sequential_layout_of<lever_discipline>::value));
    D_CVT_CHECK((lifetime_relation<view_from, lever_discipline>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<view_from, lever_discipline>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<view_from, lever_discipline>::value ==
                 order_relation::equivalent));

    // ...and nothing is forgotten and nothing is clamped, so rung 3 does not catch
    // it either - it really is the discipline clause that decides
    D_CVT_CHECK((!conversion_forgets_content<view_from,
                                             lever_discipline>::value));
    D_CVT_CHECK((!conversion_clamps_capacity<view_from,
                                             lever_discipline>::value));

    // so the tier drops one rung
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_discipline);

    return ok;
}

/*
tests_view_discipline_asymmetry
  THE OTHER DIRECTION.  Conjunct 8 admits `greater`, and this is what that buys.

  lever_discipline -> view_from is the SAME PAIR, read backwards.  From now bears a
  sorted invariant To does not ask for; To asks for nothing From does not already
  guarantee.  Dropping a restriction is free, and the conversion is a VIEW.

  So the same two types convert one way as a copy and the other way as a
  reinterpretation - and that asymmetry is the single clearest statement of what the
  discipline order MEANS.  A suite that only ever ran the pair in one direction
  would have established nothing about clause 8 at all: it would have shown that
  `less` fails, and left `greater` untested.

  Tests the following:
  - the reverse pair IS a view
  - and the forward pair is not
  - so the tier is asymmetric, in the one direction where asymmetry is correct
  - `equivalent` passes the clause too, which is the third of its three cases
*/
bool
tests_view_discipline_asymmetry()
{
    bool ok = true;

    // *** the same pair, both ways ***
    D_CVT_CHECK((discipline_relation<lever_discipline, view_from>::value ==
                 order_relation::greater));
    D_CVT_CHECK((view_of<lever_discipline, view_from>::value));
    D_CVT_TIER(conversion_tier::view, lever_discipline, view_from);

    D_CVT_CHECK((discipline_relation<view_from, lever_discipline>::value ==
                 order_relation::less));
    D_CVT_CHECK((!view_of<view_from, lever_discipline>::value));
    D_CVT_TIER(conversion_tier::constructive, view_from, lever_discipline);

    // ...so the tiers really are asymmetric
    D_CVT_CHECK((conversion_tier_of<lever_discipline, view_from>::value !=
                 conversion_tier_of<view_from, lever_discipline>::value));

    // the third case: `equivalent` passes as well
    D_CVT_CHECK((discipline_relation<view_from, view_to>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((view_of<view_from, view_to>::value));

    // and the fourth possible relation - `incomparable` - fails, as it must: two
    // overlays neither of which contains the other cannot be reinterpreted into
    // each other.  content_from is tighter on multiplicity; lever_discipline bears
    // a flag it does not
    D_CVT_CHECK((discipline_relation<content_from,
                                     lever_discipline>::value ==
                 order_relation::incomparable));
    D_CVT_CHECK((!view_of<content_from, lever_discipline>::value));

    return ok;
}

/*
tests_view_non_container
  THE FINDING.  Two unrelated non-containers are a zero-cost VIEW of each other.

      conversion_tier_of<int, plain_struct>::value   ==   view
      is_view_conversion<int, plain_struct>          ==   true
      is_lossless_conversion<int, plain_struct>      ==   true
      has_constructive_path<int, plain_struct>       ==   FALSE

  The module licenses a free reinterpretation between two types that share nothing
  at all - and simultaneously reports that there is no way to build one from the
  other.

  THE MECHANISM.  Every one of the eight clauses passes VACUOUSLY, because every
  axis reports its bottom value for a non-container and two bottoms are equal:

      1. elements    element_type_of_helper has nowhere to go for a type with no
                     value_type, and goes to `void`.  So any two non-containers have
                     the SAME element type
      2. content     both `none`
      3. topology    both flat
      4. layout      both `none`
      5. lifetime    both runtime            -> equivalent
      6. storage     both unknown            -> equivalent
      7. iterability both at the bottom      -> equivalent
      8. discipline  both mult = unknown     -> equivalent

  Nothing in the predicate asks whether either operand is a CONTAINER.  A single
  guard - `is_iterable_container<From> && is_iterable_container<To>`, or a check that
  the shared content rung is not `none` - closes it, and the path traits already
  carry exactly that gate one section up.

  Tests the following:
  - the verdict, on the fixtures and on int, void and a plain struct
  - the eight clauses, one by one, so the finding is attributable
  - the contradiction: a VIEW with no construction path
  - what does NOT save it: the shared content rung is `none`, and conversion_class
    reports it - so a caller reading the whole class knows, and a caller reading
    is_view_conversion does not
*/
bool
tests_view_non_container()
{
    bool ok = true;

    // *** THE VERDICT ***
    D_CVT_TIER(conversion_tier::view, nc_runtime, nc_runtime2);
    D_CVT_TIER(conversion_tier::view, int, nc_runtime);
    D_CVT_TIER(conversion_tier::view, void, int);
    D_CVT_TIER(conversion_tier::view, fixtures::empty, int);

    D_CVT_CHECK((is_view_conversion<int, nc_runtime>::value));
    D_CVT_CHECK((is_lossless_conversion<int, nc_runtime>::value));
    D_CVT_CHECK((is_convertible_between<int, nc_runtime>::value));

    // *** THE CONTRADICTION: a zero-cost view, and no way to build it ***
    D_CVT_CHECK((!has_constructive_path<int, nc_runtime>::value));
    D_CVT_CHECK((!has_constructive_path<nc_runtime, nc_runtime2>::value));

    // THE MECHANISM, clause by clause
    D_CVT_CHECK((elements_same_type<int, nc_runtime>::value));             // 1
    D_CVT_CHECK((native_content_level_of<int>::value ==
                 native_content_level_of<nc_runtime>::value));            // 2
    D_CVT_CHECK((!conversion_changes_topology<int, nc_runtime>::value));  // 3
    D_CVT_CHECK((sequential_layout_of<int>::value ==
                 sequential_layout_of<nc_runtime>::value));               // 4
    D_CVT_CHECK((lifetime_relation<int, nc_runtime>::value ==
                 order_relation::equivalent));                            // 5
    D_CVT_CHECK((storage_relation<int, nc_runtime>::value ==
                 order_relation::equivalent));                            // 6
    D_CVT_CHECK((iterability_relation<int, nc_runtime>::value ==
                 order_relation::equivalent));                            // 7
    D_CVT_CHECK((discipline_relation<int, nc_runtime>::value ==
                 order_relation::equivalent));                            // 8

    // ...and each is VACUOUS: every axis reports its bottom for a non-container
    D_CVT_CHECK((std::is_same<internal::element_type_of_helper<int>,
                              void>::value));
    D_CVT_LEVEL(content_level::none, native_content_level_of<int>::value);
    D_CVT_CHECK(sequential_layout_of<int>::value == sequential_layout::none);
    D_CVT_CHECK(!is_iterable_container<int>::value);

    // the gate that would close it already exists one section up, in the path traits
    D_CVT_CHECK(!is_iterable_container<int>::value);
    D_CVT_CHECK(!is_iterable_container<nc_runtime>::value);

    // WHAT DOES NOT SAVE IT: the shared content rung is `none`, and conversion_class
    // reports it.  A caller reading the whole class knows the two share no rung; a
    // caller reading is_view_conversion is told the conversion is free
    D_CVT_LEVEL(content_level::none,
                (conversion_class<int, nc_runtime>::shared_content_level));
    D_CVT_CHECK((conversion_class<int, nc_runtime>::tier ==
                 conversion_tier::view));
    D_CVT_CHECK((!conversion_class<int, nc_runtime>::has_path));

    // ...and a real container against a non-container IS rejected, at rung 2 - so
    // the ladder is not broken, only unguarded at its top
    D_CVT_TIER(conversion_tier::none, int, std_vector);
    D_CVT_TIER(conversion_tier::none, std_vector, int);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
