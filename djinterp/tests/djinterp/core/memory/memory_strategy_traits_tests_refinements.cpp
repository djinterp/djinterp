/*******************************************************************************
* djinterp [testing]                memory_strategy_traits_tests_refinements.cpp
*
* Section: stability and release refinements.
*   memory_strategy_traits.hpp VII - is_pointer_stable_strategy,
*                                    supports_individual_release_strategy,
*                                    is_monotonic_strategy,
*                                    is_generational_strategy
*
*   Three of these four are pure constant reads with a conservative default:
* absent means no.  Crucially they do NOT gate on membership, so a type that
* declares `pointer_stable` while declaring no kind constant still reads as
* pointer-stable at the TRAIT level - the CONCEPT is where membership re-enters
* (see tests_concepts_stability_release).  These tests hold the traits to the
* looser rule on purpose, because that divergence is a designed seam, not a
* slip.
*   is_monotonic_strategy is the one composite: allocating AND not individually
* releasing.  Its two edges are the point of the section.  A descriptive-only
* strategy that declares individual release is NOT monotonic (it does not
* allocate at all), even though the release trait reads true on it.  And a
* pointer-stable descriptive strategy (static_extent_strategy) is not monotonic
* for the same reason.  Monotonicity is a property of allocation, so it is
* false wherever there is no allocation, whatever the other constants say.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_refinements.cpp
*                                      memory_strategy_tests_refinements.cpp
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
tests_pointer_stability
  is_pointer_stable_strategy: reads the `pointer_stable` constant (core VII).
  Tests the following:
  - it reports the declared value, true or false, across fixtures
  - absent means false (the conservative default): the descriptive fixtures
    that omit the constant read false
  - it does NOT gate on membership: ms_stable_non_strategy declares the constant
    and no kind, and still reads stable.  This is the trait/concept seam, held
    here at the trait level
  - static_extent_strategy is stable (declares it), which matters because it is
    the container's static-storage marker
  - the arenas and static buffer are stable; the allocator wrapper is not
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_pointer_stability()
{

    // --- declared true ---
    D_MT_CHECK(is_pointer_stable_strategy<ms_element_static_monotonic>::value);
    D_MT_CHECK(is_pointer_stable_strategy<ms_element_generational>::value);
    D_MT_CHECK(is_pointer_stable_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_pointer_stable_strategy<ms_byte_cell_element>::value);

    // --- declared false ---
    D_MT_CHECK(!is_pointer_stable_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(!is_pointer_stable_strategy<ms_byte_releasing>::value);

    // --- absent means false ---
    D_MT_CHECK(!is_pointer_stable_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_pointer_stable_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(!is_pointer_stable_strategy<ms_plain_type>::value);

    // --- reads the constant without gating on membership ---
    D_MT_CHECK(!is_memory_strategy<ms_stable_non_strategy>::value);
    D_MT_CHECK(is_pointer_stable_strategy<ms_stable_non_strategy>::value);

    // --- concrete providers ---
    D_MT_CHECK(is_pointer_stable_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_pointer_stable_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(
        is_pointer_stable_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(
        is_pointer_stable_strategy<static_extent_strategy<int, 8>>::value);
    D_MT_CHECK(
        !is_pointer_stable_strategy<
            allocator_memory_strategy<std::allocator<int>>>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(
        is_pointer_stable_strategy<const ms_element_static_monotonic>::value);
    D_MT_CHECK(is_pointer_stable_strategy<ms_byte_fixed&>::value);
    D_MT_CHECK(is_pointer_stable_strategy<const ms_byte_fixed&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(
        is_pointer_stable_strategy_v<ms_byte_fixed>
            == is_pointer_stable_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(
        is_pointer_stable_strategy_v<ms_element_dynamic>
            == is_pointer_stable_strategy<ms_element_dynamic>::value);

    return true;
}


/*
tests_individual_release
  supports_individual_release_strategy: reads the release constant (core VII).
  Tests the following:
  - it reports the declared value, true or false
  - absent means false
  - it does NOT gate on membership or on allocation: ms_descriptive_releasing is
    descriptive-only yet declares release, and reads true here - the accessor is
    a pure constant read.  is_monotonic_strategy is where allocation re-enters
  - the allocator wrapper releases; the arenas and static buffer do not
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_individual_release()
{

    // --- declared true ---
    D_MT_CHECK(
        supports_individual_release_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(
        supports_individual_release_strategy<ms_byte_releasing>::value);
    D_MT_CHECK(
        supports_individual_release_strategy<ms_element_generational>::value);

    // --- declared false ---
    D_MT_CHECK(
        !supports_individual_release_strategy<ms_element_static_monotonic>
            ::value);
    D_MT_CHECK(!supports_individual_release_strategy<ms_byte_fixed>::value);

    // --- absent means false ---
    D_MT_CHECK(
        !supports_individual_release_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!supports_individual_release_strategy<ms_plain_type>::value);

    // --- pure constant read: descriptive-only can still report release ---
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(
        supports_individual_release_strategy<ms_descriptive_releasing>::value);

    // --- concrete providers ---
    D_MT_CHECK(
        supports_individual_release_strategy<
            allocator_memory_strategy<std::allocator<int>>>::value);
    D_MT_CHECK(
        !supports_individual_release_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(
        !supports_individual_release_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(
        !supports_individual_release_strategy<static_buffer_strategy<int, 8>>
            ::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(
        supports_individual_release_strategy<const ms_element_dynamic>::value);
    D_MT_CHECK(
        supports_individual_release_strategy<ms_element_dynamic&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(
        supports_individual_release_strategy_v<ms_element_dynamic>
            == supports_individual_release_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(
        supports_individual_release_strategy_v<ms_byte_fixed>
            == supports_individual_release_strategy<ms_byte_fixed>::value);

    return true;
}


/*
tests_monotonic_refinement
  is_monotonic_strategy: allocating AND not individually releasing (core VII).
  Tests the following:
  - an allocating strategy that does not release is monotonic, on both typings
    (static buffer, arenas) and both storage disciplines (static / fixed)
  - an allocating strategy that DOES release is not monotonic (allocator wrapper,
    the releasing byte fixture)
  - the two load-bearing edges: a descriptive-only strategy is NEVER monotonic,
    even one that declares release (ms_descriptive_releasing) and even one that
    is pointer-stable (static_extent_strategy).  Monotonicity is about
    allocation, and there is none here
  - a non-member is not monotonic
  - the identity holds directly: monotonic == allocating && !release, checked
    against the two reads it is composed from
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_monotonic_refinement()
{

    // --- allocating, non-releasing: monotonic ---
    D_MT_CHECK(is_monotonic_strategy<ms_element_static_monotonic>::value);
    D_MT_CHECK(is_monotonic_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_monotonic_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_monotonic_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(is_monotonic_strategy<static_buffer_strategy<int, 8>>::value);

    // --- allocating, releasing: not monotonic ---
    D_MT_CHECK(!is_monotonic_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(!is_monotonic_strategy<ms_byte_releasing>::value);
    D_MT_CHECK(!is_monotonic_strategy<ms_element_generational>::value);
    D_MT_CHECK(
        !is_monotonic_strategy<allocator_memory_strategy<std::allocator<int>>>
            ::value);

    // --- descriptive-only is never monotonic, whatever else it declares ---
    //     declares release, but does not allocate
    D_MT_CHECK(
        supports_individual_release_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(!is_monotonic_strategy<ms_descriptive_releasing>::value);
    //     pointer-stable, but does not allocate
    D_MT_CHECK(
        is_pointer_stable_strategy<static_extent_strategy<int, 8>>::value);
    D_MT_CHECK(!is_monotonic_strategy<static_extent_strategy<int, 8>>::value);
    //     plain descriptive
    D_MT_CHECK(!is_monotonic_strategy<ms_descriptive_static>::value);

    // --- non-member ---
    D_MT_CHECK(!is_monotonic_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_monotonic_strategy<std::allocator<int>>::value);

    // --- the composition identity, checked against its two reads ---
    D_MT_CHECK(
        is_monotonic_strategy<ms_byte_fixed>::value
            == ( is_allocating_strategy<ms_byte_fixed>::value
                 && !supports_individual_release_strategy<ms_byte_fixed>
                        ::value ));
    D_MT_CHECK(
        is_monotonic_strategy<ms_element_dynamic>::value
            == ( is_allocating_strategy<ms_element_dynamic>::value
                 && !supports_individual_release_strategy<ms_element_dynamic>
                        ::value ));
    D_MT_CHECK(
        is_monotonic_strategy<ms_descriptive_releasing>::value
            == ( is_allocating_strategy<ms_descriptive_releasing>::value
                 && !supports_individual_release_strategy<
                        ms_descriptive_releasing>::value ));

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_monotonic_strategy<const monotonic_byte_arena<64>&>::value);
    D_MT_CHECK(!is_monotonic_strategy<const ms_element_dynamic&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(is_monotonic_strategy_v<monotonic_byte_arena<64>>
                   == is_monotonic_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_monotonic_strategy_v<ms_element_dynamic>
                   == is_monotonic_strategy<ms_element_dynamic>::value);

    return true;
}


/*
tests_generational_refinement
  is_generational_strategy: reads the generational-sweep constant (core VII).
  Tests the following:
  - true only where declared: exactly one element fixture opts in
  - absent means false, across every fixture that omits the constant
  - like the other reads, it does NOT gate on allocation: ms_descriptive_releasing
    declares the sweep and reads true here despite being descriptive-only.  The
    generational CONCEPT re-imposes allocation (see the concepts section)
  - no concrete provider is generational
  - cv-ref agnostic, and the `_v` companion agrees
*/
bool
tests_generational_refinement()
{

    // --- declared true ---
    D_MT_CHECK(is_generational_strategy<ms_element_generational>::value);

    // --- pure constant read: descriptive-only can report the sweep ---
    D_MT_CHECK(is_descriptive_only_strategy<ms_descriptive_releasing>::value);
    D_MT_CHECK(is_generational_strategy<ms_descriptive_releasing>::value);

    // --- absent means false ---
    D_MT_CHECK(!is_generational_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(!is_generational_strategy<ms_element_static_monotonic>::value);
    D_MT_CHECK(!is_generational_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(!is_generational_strategy<ms_byte_releasing>::value);
    D_MT_CHECK(!is_generational_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_generational_strategy<ms_plain_type>::value);

    // --- no concrete provider is generational ---
    D_MT_CHECK(!is_generational_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(!is_generational_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(!is_generational_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(
        !is_generational_strategy<allocator_memory_strategy<std::allocator<int>>>
            ::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_generational_strategy<const ms_element_generational>::value);
    D_MT_CHECK(is_generational_strategy<ms_element_generational&>::value);
    D_MT_CHECK(!is_generational_strategy<const ms_element_dynamic&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(
        is_generational_strategy_v<ms_element_generational>
            == is_generational_strategy<ms_element_generational>::value);
    D_MT_CHECK(is_generational_strategy_v<ms_element_dynamic>
                   == is_generational_strategy<ms_element_dynamic>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
