/*******************************************************************************
* djinterp [testing]             memory_strategy_traits_tests_classification.cpp
*
* Section: descriptive / operational classification.
*   memory_strategy_traits.hpp V - is_memory_strategy, is_non_memory_strategy,
*                                  is_allocating_strategy,
*                                  is_descriptive_only_strategy
*
*   Membership is the axis' one provider-independent question, and it is
* narrower than it looks: it asks ONLY whether a type declares its storage
* discipline (the kind constant), not whether it can allocate.  A complete
* element surface with no constant - std::allocator's exact shape - is not a
* member.  A bare constant with no surface IS a member.  These two facts,
* together, are the whole reason the trait exists apart from the typing traits,
* and most of the checks below are one or the other of them.
*   From membership the operational split falls out: a member that speaks a
* typed surface allocates; a member that does not is descriptive-only.  The two
* are exhaustive and disjoint over members, which is asserted directly.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_classification.cpp
*                                  memory_strategy_tests_classification.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <type_traits>
// djinterp
#include "memory_strategy_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_membership_classification
  is_memory_strategy / is_non_memory_strategy: the descriptive contract (core V).
  Tests the following:
  - membership is exactly "declares the kind constant" and nothing else: every
    descriptive fixture is a member, on the strength of the constant alone
  - a complete typed surface without the constant is NOT a member - both
    ms_element_surface_only and std::allocator<int> carry the whole element
    surface yet declare no discipline, and are rejected.  This is the single
    most load-bearing negative on the axis
  - the mistyped-kind-constant fixture is NOT admitted: detection is by name AND
    type, so a `strategy_storage_kind` of the wrong type is rejected at
    membership - and because it is rejected, the kind accessor is total on it
    (returns unknown) rather than hard-erroring
  - concrete providers are all members
  - is_non_memory_strategy is the exact complement, fixture for fixture
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_membership_classification()
{

    // --- a bare kind constant is sufficient ---
    D_MT_CHECK(is_memory_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_dynamic>::value);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_unknown>::value);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_releasing>::value);

    // --- members that also speak a surface ---
    D_MT_CHECK(is_memory_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_memory_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_memory_strategy<ms_dual_typed>::value);
    D_MT_CHECK(is_memory_strategy<ms_void_value_element>::value);

    // --- a complete surface without the constant is NOT a member ---
    D_MT_CHECK(is_element_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(!is_memory_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(is_element_strategy<std::allocator<int>>::value);
    D_MT_CHECK(!is_memory_strategy<std::allocator<int>>::value);

    // --- detection is by name AND type: the mistyped constant is rejected ---
    //     a `strategy_storage_kind` that is not a storage_kind is not membership,
    //     and the kind accessor is then total on it (unknown, no hard error)
    D_MT_CHECK(!has_strategy_kind_constant<ms_mistyped_kind_constant>::value);
    D_MT_CHECK(!is_memory_strategy<ms_mistyped_kind_constant>::value);
    D_MT_CHECK(memory_strategy_kind_of<ms_mistyped_kind_constant>::value
                   == storage_kind::unknown);

    // --- a stray non-kind constant is not membership ---
    D_MT_CHECK(!is_memory_strategy<ms_stable_non_strategy>::value);

    // --- concrete providers are members ---
    D_MT_CHECK(is_memory_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_memory_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(is_memory_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(is_memory_strategy<static_extent_strategy<int, 8>>::value);
    D_MT_CHECK(
        is_memory_strategy<allocator_memory_strategy<std::allocator<int>>>
            ::value);

    // --- nothing at all ---
    D_MT_CHECK(!is_memory_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_memory_strategy<int>::value);
    D_MT_CHECK(!is_memory_strategy<void>::value);

    // --- is_non_memory_strategy is the exact complement ---
    D_MT_CHECK(!is_non_memory_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_non_memory_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_non_memory_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(is_non_memory_strategy<std::allocator<int>>::value);
    D_MT_CHECK(is_non_memory_strategy<ms_plain_type>::value);
    D_MT_CHECK(is_non_memory_strategy<int>::value);
    D_MT_CHECK(is_non_memory_strategy<ms_descriptive_static>::value
                   == !is_memory_strategy<ms_descriptive_static>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_memory_strategy<const ms_descriptive_static>::value);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_static&>::value);
    D_MT_CHECK(is_memory_strategy<const ms_descriptive_static&>::value);
    D_MT_CHECK(is_memory_strategy<volatile ms_descriptive_static&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(is_memory_strategy_v<ms_descriptive_static>
                   == is_memory_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_memory_strategy_v<std::allocator<int>>
                   == is_memory_strategy<std::allocator<int>>::value);
    D_MT_CHECK(is_non_memory_strategy_v<ms_plain_type>
                   == is_non_memory_strategy<ms_plain_type>::value);

    return true;
}


/*
tests_allocating_classification
  is_allocating_strategy: membership AND a typed surface (core V).
  Tests the following:
  - it is the conjunction of membership with (element OR byte): a type must be
    BOTH a member and able to allocate, and each concrete provider satisfies
    both
  - a member with no surface is not allocating (the descriptive fixtures)
  - a surface with no membership is not allocating: std::allocator<int> and
    ms_element_surface_only both speak element yet fail the membership term.
    This is the term that keeps an allocator from being mistaken for a strategy
  - a HALF surface does not allocate, on either typing
  - ms_void_value_element is a member that allocates through no RECOGNIZED
    typing, so it is not allocating - the void guard reaches this far
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_allocating_classification()
{

    // --- member and speaks a surface ---
    D_MT_CHECK(is_allocating_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_allocating_strategy<ms_element_static_monotonic>::value);
    D_MT_CHECK(is_allocating_strategy<ms_element_generational>::value);
    D_MT_CHECK(is_allocating_strategy<ms_byte_cell_element>::value);
    D_MT_CHECK(is_allocating_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_allocating_strategy<ms_byte_releasing>::value);
    D_MT_CHECK(is_allocating_strategy<ms_dual_typed>::value);

    // --- concrete providers all allocate ---
    D_MT_CHECK(is_allocating_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_allocating_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(is_allocating_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(
        is_allocating_strategy<allocator_memory_strategy<std::allocator<int>>>
            ::value);

    // --- a member with no surface does not allocate ---
    D_MT_CHECK(!is_allocating_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_allocating_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(!is_allocating_strategy<static_extent_strategy<int, 8>>::value);

    // --- a surface with no membership does not allocate ---
    D_MT_CHECK(!is_allocating_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(!is_allocating_strategy<std::allocator<int>>::value);

    // --- a half surface does not allocate ---
    D_MT_CHECK(!is_allocating_strategy<ms_element_no_dealloc>::value);
    D_MT_CHECK(!is_allocating_strategy<ms_element_no_alloc>::value);
    D_MT_CHECK(!is_allocating_strategy<ms_byte_no_dealloc>::value);

    // --- allocates through no recognized typing: not allocating ---
    D_MT_CHECK(is_memory_strategy<ms_void_value_element>::value);
    D_MT_CHECK(!is_allocating_strategy<ms_void_value_element>::value);

    // --- nothing at all ---
    D_MT_CHECK(!is_allocating_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_allocating_strategy<int>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_allocating_strategy<const ms_element_dynamic>::value);
    D_MT_CHECK(is_allocating_strategy<ms_element_dynamic&>::value);
    D_MT_CHECK(is_allocating_strategy<const ms_byte_fixed&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(is_allocating_strategy_v<ms_element_dynamic>
                   == is_allocating_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_allocating_strategy_v<ms_descriptive_static>
                   == is_allocating_strategy<ms_descriptive_static>::value);

    return true;
}


/*
tests_descriptive_only_classification
  is_descriptive_only_strategy: a member that does not allocate (core V).
  Tests the following:
  - the descriptive fixtures are the archetype: a kind constant and no surface
  - static_extent_strategy is the concrete descriptive-only provider - the
    container embeds the storage and the strategy only describes it
  - a half surface is descriptive-only: it is a member (has the constant) and
    is not allocating (surface incomplete), which places it here
  - ms_void_value_element lands here too - member, non-allocating-by-typing
  - descriptive-only and allocating PARTITION the members: over every member
    fixture, exactly one of the two holds.  Over a non-member, NEITHER holds
  - a non-member is never descriptive-only, even with a full surface
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_descriptive_only_classification()
{

    // --- constant, no surface ---
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_dynamic>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_unknown>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(
        is_descriptive_only_strategy<static_extent_strategy<int, 8>>::value);

    // --- a half surface is descriptive-only ---
    D_MT_CHECK(is_descriptive_only_strategy<ms_element_no_dealloc>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_element_no_alloc>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_byte_no_dealloc>::value);

    // --- allocates through no recognized typing ---
    D_MT_CHECK(is_descriptive_only_strategy<ms_void_value_element>::value);

    // --- allocating providers are NOT descriptive-only ---
    D_MT_CHECK(!is_descriptive_only_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(!is_descriptive_only_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(!is_descriptive_only_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(
        !is_descriptive_only_strategy<static_buffer_strategy<int, 8>>::value);

    // --- a non-member is never descriptive-only, full surface or not ---
    D_MT_CHECK(!is_descriptive_only_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(!is_descriptive_only_strategy<std::allocator<int>>::value);
    D_MT_CHECK(!is_descriptive_only_strategy<ms_plain_type>::value);

    // --- descriptive-only and allocating partition the members ---
    //     each member is exactly one of the two
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_static>::value
                   != is_allocating_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_element_dynamic>::value
                   != is_allocating_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_void_value_element>::value
                   != is_allocating_strategy<ms_void_value_element>::value);
    D_MT_CHECK(is_descriptive_only_strategy<monotonic_byte_arena<64>>::value
                   != is_allocating_strategy<monotonic_byte_arena<64>>::value);
    //     over a non-member, neither holds
    D_MT_CHECK(!is_descriptive_only_strategy<std::allocator<int>>::value
                   && !is_allocating_strategy<std::allocator<int>>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_descriptive_only_strategy<const ms_descriptive_static>
                   ::value);
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_static&>::value);
    D_MT_CHECK(is_descriptive_only_strategy<const ms_descriptive_static&>
                   ::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(
        is_descriptive_only_strategy_v<ms_descriptive_static>
            == is_descriptive_only_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(
        is_descriptive_only_strategy_v<ms_element_dynamic>
            == is_descriptive_only_strategy<ms_element_dynamic>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
