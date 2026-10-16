/*******************************************************************************
* djinterp [testing]                 memory_strategy_concepts_tests_concepts.cpp
*
* Section: the C++20 concept layer.
*   memory_strategy_concepts.hpp - membership, typing, storage-discipline, and
*                                  stability/release concepts
*
*   Every concept forwards to a trait or `_v` variable, so most checks confirm
* the forward.  Two things earn the section its length.  First, several concepts
* GATE on a prior concept before reading their own trait - pointer_stable_strategy
* asks for membership first, the release/generational concepts ask for
* allocation first - and that gate makes the concept DIVERGE from the bare trait
* on exactly the fixtures built to expose it.  Those divergences are asserted
* head to head (trait says yes, concept says no), because they are the whole
* reason the concept layer is not just a rename of the traits.
*   Second, a concept's real job is to CONSTRAIN, so the last test dispatches
* through the header's probe overloads and confirms that allocating_element_strategy
* SUBSUMES element_strategy - the more refined constraint wins without ambiguity,
* and the plain element case still resolves to the plain overload.  That is the
* one place the axis leans on constraint partial ordering rather than a value.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_concepts_tests_concepts.cpp
*                                         memory_strategy_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <type_traits>
// djinterp
#include "memory_strategy_concepts_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_concepts_membership
  memory_strategy / non_memory_strategy / allocating_strategy /
  descriptive_only_strategy (concepts I).
  Tests the following:
  - each membership concept satisfies exactly where its backing trait does
  - non_memory_strategy is the strict complement of memory_strategy
  - allocating and descriptive_only partition the members and hold for neither
    non-member
  - a full element surface without a kind constant satisfies none of the
    membership concepts (std::allocator<int>) - it is not a strategy
*/
bool
tests_concepts_membership()
{

    // --- memory_strategy where the trait holds ---
    D_MC_CHECK(memory_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(memory_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(memory_strategy<allocator_memory_strategy<std::allocator<int>>>);
    D_MC_CHECK(memory_strategy<ms_descriptive_static>);
    D_MC_CHECK(!memory_strategy<std::allocator<int>>);
    D_MC_CHECK(!memory_strategy<ms_element_surface_only>);
    D_MC_CHECK(!memory_strategy<ms_plain_type>);

    // --- concept agrees with trait ---
    D_MC_CHECK(memory_strategy<monotonic_byte_arena<256>>
                   == is_memory_strategy_v<monotonic_byte_arena<256>>);
    D_MC_CHECK(memory_strategy<std::allocator<int>>
                   == is_memory_strategy_v<std::allocator<int>>);

    // --- non_memory_strategy is the strict complement ---
    D_MC_CHECK(non_memory_strategy<std::allocator<int>>);
    D_MC_CHECK(non_memory_strategy<ms_plain_type>);
    D_MC_CHECK(!non_memory_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(non_memory_strategy<ms_plain_type>
                   == !memory_strategy<ms_plain_type>);

    // --- allocating / descriptive_only ---
    D_MC_CHECK(allocating_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(allocating_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(!allocating_strategy<ms_descriptive_static>);
    D_MC_CHECK(!allocating_strategy<std::allocator<int>>);

    D_MC_CHECK(descriptive_only_strategy<ms_descriptive_static>);
    D_MC_CHECK(descriptive_only_strategy<static_extent_strategy<int, 8>>);
    D_MC_CHECK(!descriptive_only_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(!descriptive_only_strategy<std::allocator<int>>);

    // --- allocating and descriptive_only partition the members ---
    D_MC_CHECK(allocating_strategy<monotonic_byte_arena<256>>
                   != descriptive_only_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(allocating_strategy<ms_descriptive_static>
                   != descriptive_only_strategy<ms_descriptive_static>);
    //     neither holds for a non-member
    D_MC_CHECK(!allocating_strategy<std::allocator<int>>
                   && !descriptive_only_strategy<std::allocator<int>>);

    return true;
}


/*
tests_concepts_typing
  element_strategy / byte_strategy / allocating_element_strategy /
  allocating_byte_strategy (concepts II).
  Tests the following:
  - element_strategy and byte_strategy match their traits, and are disjoint on
    every fixture (nothing is both)
  - the composed operational typings require BOTH allocation and the typing:
    std::allocator<int> is element-typed but not allocating (not a member), so
    allocating_element_strategy rejects it while element_strategy accepts it -
    the precise gap the composite is for
  - the arenas are allocating_byte_strategy; the buffer and allocator wrapper
    are allocating_element_strategy
*/
bool
tests_concepts_typing()
{

    // --- element / byte match their traits ---
    D_MC_CHECK(element_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(element_strategy<std::allocator<int>>);
    D_MC_CHECK(!element_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(byte_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(byte_strategy<dynamic_byte_arena>);
    D_MC_CHECK(!byte_strategy<static_buffer_strategy<int, 8>>);

    D_MC_CHECK(element_strategy<static_buffer_strategy<int, 8>>
                   == is_element_strategy_v<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(byte_strategy<monotonic_byte_arena<256>>
                   == is_byte_strategy_v<monotonic_byte_arena<256>>);

    // --- a single-surface fixture satisfies exactly one typing concept ---
    D_MC_CHECK(element_strategy<static_buffer_strategy<int, 8>>
                   && !byte_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(byte_strategy<monotonic_byte_arena<256>>
                   && !element_strategy<monotonic_byte_arena<256>>);
    // --- but a DUAL-surface type satisfies BOTH typing concepts at once ---
    //     the concepts read the two surfaces independently; only the
    //     strategy_typing_of ENUM is single-valued, and it resolves to element
    D_MC_CHECK(element_strategy<ms_dual_typed>);
    D_MC_CHECK(byte_strategy<ms_dual_typed>);
    D_MC_CHECK(strategy_typing_of<ms_dual_typed>::value
                   == strategy_typing::element_typed);

    // --- composed typings require allocation AND the typing ---
    //     std::allocator is element-typed but not allocating (not a member)
    D_MC_CHECK(element_strategy<std::allocator<int>>);
    D_MC_CHECK(!allocating_element_strategy<std::allocator<int>>);
    //     the buffer and allocator wrapper are allocating element strategies
    D_MC_CHECK(allocating_element_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(
        allocating_element_strategy<
            allocator_memory_strategy<std::allocator<int>>>);
    //     the arenas are allocating byte strategies
    D_MC_CHECK(allocating_byte_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(allocating_byte_strategy<dynamic_byte_arena>);
    D_MC_CHECK(!allocating_byte_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(!allocating_element_strategy<monotonic_byte_arena<256>>);

    return true;
}


/*
tests_concepts_storage_discipline
  static_memory_strategy / fixed_memory_strategy / dynamic_memory_strategy
  (concepts III).
  Tests the following:
  - each discipline concept holds for exactly the matching kind, and the three
    are mutually exclusive
  - each concept gates on membership first: a non-member satisfies none, and the
    explicitly-unknown member also satisfies none
  - the arenas split static vs fixed, the allocator wrapper is dynamic
*/
bool
tests_concepts_storage_discipline()
{

    // --- static ---
    D_MC_CHECK(static_memory_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(static_memory_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(static_memory_strategy<ms_descriptive_static>);
    D_MC_CHECK(!fixed_memory_strategy<ms_descriptive_static>);
    D_MC_CHECK(!dynamic_memory_strategy<ms_descriptive_static>);

    // --- fixed ---
    D_MC_CHECK(fixed_memory_strategy<dynamic_byte_arena>);
    D_MC_CHECK(fixed_memory_strategy<ms_descriptive_fixed>);
    D_MC_CHECK(!static_memory_strategy<dynamic_byte_arena>);

    // --- dynamic ---
    D_MC_CHECK(
        dynamic_memory_strategy<allocator_memory_strategy<std::allocator<int>>>);
    D_MC_CHECK(dynamic_memory_strategy<ms_descriptive_dynamic>);
    D_MC_CHECK(!static_memory_strategy<ms_descriptive_dynamic>);

    // --- mutual exclusivity over a known kind ---
    D_MC_CHECK(( int(static_memory_strategy<monotonic_byte_arena<256>>)
                 + int(fixed_memory_strategy<monotonic_byte_arena<256>>)
                 + int(dynamic_memory_strategy<monotonic_byte_arena<256>>) )
                   == 1);

    // --- gates on membership: non-member and unknown-member satisfy none ---
    D_MC_CHECK(( int(static_memory_strategy<std::allocator<int>>)
                 + int(fixed_memory_strategy<std::allocator<int>>)
                 + int(dynamic_memory_strategy<std::allocator<int>>) )
                   == 0);
    D_MC_CHECK(( int(static_memory_strategy<ms_descriptive_unknown>)
                 + int(fixed_memory_strategy<ms_descriptive_unknown>)
                 + int(dynamic_memory_strategy<ms_descriptive_unknown>) )
                   == 0);
    D_MC_CHECK(is_memory_strategy_v<ms_descriptive_unknown>);   // yet a member

    return true;
}


/*
tests_concepts_stability_release
  pointer_stable_strategy / individually_releasing_strategy / monotonic_strategy
  / generational_strategy (concepts IV).
  Tests the following:
  - each concept holds where expected for the concrete providers
  - THE DIVERGENCES, head to head with the bare traits:
      * ms_stable_non_strategy: the pointer-stable TRAIT is true (it declares the
        constant), but pointer_stable_strategy is FALSE - the concept gates on
        membership, which this type fails
      * ms_descriptive_releasing: the release and generational TRAITS are true,
        but individually_releasing_strategy and generational_strategy are both
        FALSE - those concepts gate on allocation, which this type fails
    These are the point of the concept layer, so each is asserted as trait !=
    concept, not merely as a value
  - static_extent_strategy is pointer_stable_strategy (member + stable) even
    though descriptive-only: the membership gate PASSES here, so no divergence
  - monotonic_strategy agrees with its trait on the allocating providers
*/
bool
tests_concepts_stability_release()
{

    // --- concrete providers ---
    D_MC_CHECK(pointer_stable_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(pointer_stable_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(
        !pointer_stable_strategy<allocator_memory_strategy<std::allocator<int>>>);
    D_MC_CHECK(
        individually_releasing_strategy<
            allocator_memory_strategy<std::allocator<int>>>);
    D_MC_CHECK(!individually_releasing_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(monotonic_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK(monotonic_strategy<static_buffer_strategy<int, 8>>);
    D_MC_CHECK(
        !monotonic_strategy<allocator_memory_strategy<std::allocator<int>>>);
    D_MC_CHECK(generational_strategy<ms_element_generational>);

    // --- DIVERGENCE 1: pointer-stable trait true, concept false (no member) ---
    D_MC_CHECK(is_pointer_stable_strategy_v<ms_stable_non_strategy>);
    D_MC_CHECK(!pointer_stable_strategy<ms_stable_non_strategy>);
    D_MC_CHECK(is_pointer_stable_strategy_v<ms_stable_non_strategy>
                   != pointer_stable_strategy<ms_stable_non_strategy>);

    // --- DIVERGENCE 2: release / generational traits true, concepts false ---
    //     (descriptive-only fails the allocation gate)
    D_MC_CHECK(supports_individual_release_strategy_v<ms_descriptive_releasing>);
    D_MC_CHECK(!individually_releasing_strategy<ms_descriptive_releasing>);
    D_MC_CHECK(
        supports_individual_release_strategy_v<ms_descriptive_releasing>
            != individually_releasing_strategy<ms_descriptive_releasing>);

    D_MC_CHECK(is_generational_strategy_v<ms_descriptive_releasing>);
    D_MC_CHECK(!generational_strategy<ms_descriptive_releasing>);
    D_MC_CHECK(is_generational_strategy_v<ms_descriptive_releasing>
                   != generational_strategy<ms_descriptive_releasing>);

    // --- NO divergence where the gate passes: static_extent is a member ---
    D_MC_CHECK(descriptive_only_strategy<static_extent_strategy<int, 8>>);
    D_MC_CHECK(is_pointer_stable_strategy_v<static_extent_strategy<int, 8>>);
    D_MC_CHECK(pointer_stable_strategy<static_extent_strategy<int, 8>>);
    D_MC_CHECK(is_pointer_stable_strategy_v<static_extent_strategy<int, 8>>
                   == pointer_stable_strategy<static_extent_strategy<int, 8>>);

    // --- monotonic_strategy agrees with its trait on allocating providers ---
    D_MC_CHECK(monotonic_strategy<monotonic_byte_arena<256>>
                   == is_monotonic_strategy_v<monotonic_byte_arena<256>>);
    D_MC_CHECK(
        monotonic_strategy<allocator_memory_strategy<std::allocator<int>>>
            == is_monotonic_strategy_v<
                   allocator_memory_strategy<std::allocator<int>>>);

    return true;
}


/*
tests_concepts_constraint_dispatch
  concepts as constraints: overload resolution and subsumption (header probes).
  Tests the following:
  - the single-constraint probes route to the constrained overload exactly when
    the concept is satisfied, and to the fallback otherwise (memory, element,
    byte)
  - SUBSUMPTION: ms_probe_element_refinement returns 2 for an allocating element
    strategy, 1 for a plain element surface, 0 otherwise.  A buffer (allocating
    element) resolves to 2; std::allocator<int> (element-typed, NOT allocating)
    resolves to 1; the two answers together prove allocating_element_strategy
    subsumes element_strategy AND that the allocating term is what separates
    them.  Non-elements resolve to 0
  - the dispatch answers line up with the concept values directly
*/
bool
tests_concepts_constraint_dispatch()
{

    // --- single-constraint dispatch ---
    D_MC_CHECK(ms_probe_memory_strategy<monotonic_byte_arena<256>>() == 1);
    D_MC_CHECK(ms_probe_memory_strategy<std::allocator<int>>() == 0);
    D_MC_CHECK(ms_probe_memory_strategy<ms_plain_type>() == 0);

    D_MC_CHECK(ms_probe_element_strategy<static_buffer_strategy<int, 8>>() == 1);
    D_MC_CHECK(ms_probe_element_strategy<monotonic_byte_arena<256>>() == 0);
    D_MC_CHECK(ms_probe_element_strategy<std::allocator<int>>() == 1);

    D_MC_CHECK(ms_probe_byte_strategy<monotonic_byte_arena<256>>() == 1);
    D_MC_CHECK(ms_probe_byte_strategy<static_buffer_strategy<int, 8>>() == 0);

    // --- subsumption: refined constraint wins, plain constraint still resolves ---
    D_MC_CHECK(
        ms_probe_element_refinement<static_buffer_strategy<int, 8>>() == 2);
    D_MC_CHECK(
        ms_probe_element_refinement<
            allocator_memory_strategy<std::allocator<int>>>() == 2);
    D_MC_CHECK(ms_probe_element_refinement<std::allocator<int>>() == 1);
    D_MC_CHECK(ms_probe_element_refinement<ms_plain_type>() == 0);
    D_MC_CHECK(ms_probe_element_refinement<ms_descriptive_static>() == 0);
    D_MC_CHECK(ms_probe_element_refinement<monotonic_byte_arena<256>>() == 0);

    // --- dispatch answers line up with the concept values ---
    D_MC_CHECK((ms_probe_memory_strategy<monotonic_byte_arena<256>>() == 1)
                   == memory_strategy<monotonic_byte_arena<256>>);
    D_MC_CHECK((ms_probe_element_refinement<static_buffer_strategy<int, 8>>()
                    == 2)
                   == allocating_element_strategy<
                          static_buffer_strategy<int, 8>>);
    D_MC_CHECK((ms_probe_element_refinement<std::allocator<int>>() == 1)
                   == ( element_strategy<std::allocator<int>>
                        && !allocating_element_strategy<std::allocator<int>> ));

    return true;
}


NS_END  // testing
NS_END  // djinterp
