/*******************************************************************************
* djinterp [testing]                     memory_strategy_traits_tests_typing.cpp
*
* Section: typing classification.
*   memory_strategy_traits.hpp IV - is_element_strategy, is_byte_strategy,
*                                   strategy_typing_of
*
*   Where the raw detectors report callability, this layer reports MEANING, and
* the two differ in exactly one place that matters: an arena's defaulted align
* argument makes it callable as an element strategy without making it one.  The
* composites earn their existence by getting that right, and these tests exist
* to hold them to it.
*   Typing is also orthogonal to membership: std::allocator<int> is a perfectly
* good element-typed surface and not a memory strategy at all.  Nothing here
* asks about storage discipline; that is section V's question.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_typing.cpp
*                                          memory_strategy_tests_typing.cpp
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
tests_element_typing_classification
  is_element_strategy: the complete element-typed surface (core IV).
  Tests the following:
  - it is a CONJUNCTION of four terms: value_type present, value_type non-void,
    allocate(n), deallocate(p,n).  Each of the four fixtures that omits exactly
    one term is rejected, so no term is redundant
  - the non-void term is the one that rejects the arenas, which satisfy the
    other three by virtue of a default argument
  - it is orthogonal to membership: std::allocator<int> passes without
    declaring any storage discipline, and ms_element_surface_only likewise
  - it is cv-ref agnostic, because it cleans before probing - even though the
    raw allocate detector it is built from is NOT (see tests_cv_ref_agnosticism)
  - the `_v` companion agrees
*/
bool
tests_element_typing_classification()
{

    // --- complete surfaces ---
    D_MT_CHECK(is_element_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_element_strategy<ms_element_static_monotonic>::value);
    D_MT_CHECK(is_element_strategy<ms_element_generational>::value);
    D_MT_CHECK(is_element_strategy<ms_byte_cell_element>::value);
    D_MT_CHECK(is_element_strategy<ms_element_surface_only>::value);
    D_MT_CHECK(is_element_strategy<std::allocator<int>>::value);
    D_MT_CHECK(is_element_strategy<static_buffer_strategy<int, 8>>::value);

    // --- each missing term rejects, so no term is redundant ---
    //     missing allocate(n)
    D_MT_CHECK(!is_element_strategy<ms_element_no_alloc>::value);
    //     missing deallocate(p,n)
    D_MT_CHECK(!is_element_strategy<ms_element_no_dealloc>::value);
    //     missing value_type entirely
    D_MT_CHECK(!is_element_strategy<ms_byte_fixed>::value);
    //     value_type present but void
    D_MT_CHECK(!is_element_strategy<ms_void_value_element>::value);

    // --- the non-void term is what rejects the arenas ---
    D_MT_CHECK(has_strategy_value_type<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_element_allocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_element_deallocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(!is_element_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(!is_element_strategy<dynamic_byte_arena>::value);

    // --- not a strategy, still element-typed: the axes are independent ---
    D_MT_CHECK(!is_memory_strategy<std::allocator<int>>::value);
    D_MT_CHECK(is_element_strategy<std::allocator<int>>::value);

    // --- nothing at all ---
    D_MT_CHECK(!is_element_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_element_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_element_strategy<int>::value);
    D_MT_CHECK(!is_element_strategy<void>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_element_strategy<const ms_element_dynamic>::value);
    D_MT_CHECK(is_element_strategy<ms_element_dynamic&>::value);
    D_MT_CHECK(is_element_strategy<const ms_element_dynamic&>::value);
    D_MT_CHECK(is_element_strategy<ms_element_dynamic&&>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(is_element_strategy_v<ms_element_dynamic>
                   == is_element_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(is_element_strategy_v<ms_byte_fixed>
                   == is_element_strategy<ms_byte_fixed>::value);

    return true;
}


/*
tests_byte_typing_classification
  is_byte_strategy: the complete byte-typed surface (core IV).
  Tests the following:
  - it is a CONJUNCTION of exactly two terms, and asks for no value_type: the
    canonical byte fixture declares none and passes
  - each half-surface fixture is rejected
  - a one-argument element allocate is not a byte allocate, so element-only
    fixtures and std::allocator are rejected
  - both concrete arenas pass, in both storage disciplines
  - it is cv-ref agnostic
  - the `_v` companion agrees
*/
bool
tests_byte_typing_classification()
{

    // --- complete surfaces, with and without a value_type ---
    D_MT_CHECK(is_byte_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_byte_strategy<ms_byte_releasing>::value);
    D_MT_CHECK(is_byte_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_byte_strategy<dynamic_byte_arena>::value);

    // --- no value_type required ---
    D_MT_CHECK(!has_strategy_value_type<ms_byte_fixed>::value);
    D_MT_CHECK(is_byte_strategy<ms_byte_fixed>::value);

    // --- half surfaces rejected ---
    D_MT_CHECK(!is_byte_strategy<ms_byte_no_dealloc>::value);

    // --- element surfaces are not byte surfaces ---
    D_MT_CHECK(!is_byte_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(!is_byte_strategy<std::allocator<int>>::value);
    D_MT_CHECK(!is_byte_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(!is_byte_strategy<ms_void_value_element>::value);

    // --- nothing at all ---
    D_MT_CHECK(!is_byte_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_byte_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_byte_strategy<int>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_byte_strategy<const ms_byte_fixed>::value);
    D_MT_CHECK(is_byte_strategy<ms_byte_fixed&>::value);
    D_MT_CHECK(is_byte_strategy<const ms_byte_fixed&>::value);
    D_MT_CHECK(is_byte_strategy<volatile ms_byte_fixed>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(is_byte_strategy_v<ms_byte_fixed>
                   == is_byte_strategy<ms_byte_fixed>::value);
    D_MT_CHECK(is_byte_strategy_v<ms_element_dynamic>
                   == is_byte_strategy<ms_element_dynamic>::value);

    return true;
}


/*
tests_typing_precedence
  strategy_typing_of: which surface a strategy is taken to speak (core IV).
  Tests the following:
  - the three answers, one fixture each
  - ELEMENT WINS when both surfaces are present.  ms_dual_typed declares both
    in full, by overload, and must report element_typed - a typed view is the
    more specific contract
  - the arenas report byte_typed, which is the interesting half of the same
    rule: they trip both raw allocate detectors, and are saved from being
    misreported as element_typed only by is_element_strategy's non-void guard.
    Were that guard dropped, the arenas would silently reclassify HERE, not at
    the detector - which is what makes this the load-bearing check
  - `none` is reported for a descriptive-only strategy AND for a non-strategy:
    typing does not gate on membership, so the answer is about surfaces only
  - ms_void_value_element reports `none` despite supplying storage: a strategy
    can allocate through no RECOGNIZED typing
  - the enum answer and the two boolean traits never disagree
  - the `_v` companion agrees
*/
bool
tests_typing_precedence()
{

    // --- the three answers ---
    D_MT_CHECK(strategy_typing_of<ms_element_dynamic>::value
                   == strategy_typing::element_typed);
    D_MT_CHECK(strategy_typing_of<ms_byte_fixed>::value
                   == strategy_typing::byte_typed);
    D_MT_CHECK(strategy_typing_of<ms_descriptive_static>::value
                   == strategy_typing::none);

    // --- element wins when both surfaces are present ---
    D_MT_CHECK(is_element_strategy<ms_dual_typed>::value);
    D_MT_CHECK(is_byte_strategy<ms_dual_typed>::value);
    D_MT_CHECK(strategy_typing_of<ms_dual_typed>::value
                   == strategy_typing::element_typed);

    // --- the arenas: byte-typed, and only the non-void guard keeps them so ---
    D_MT_CHECK(strategy_typing_of<monotonic_byte_arena<64>>::value
                   == strategy_typing::byte_typed);
    D_MT_CHECK(strategy_typing_of<dynamic_byte_arena>::value
                   == strategy_typing::byte_typed);

    // --- typing does not gate on membership ---
    D_MT_CHECK(!is_memory_strategy<std::allocator<int>>::value);
    D_MT_CHECK(strategy_typing_of<std::allocator<int>>::value
                   == strategy_typing::element_typed);
    D_MT_CHECK(strategy_typing_of<ms_element_surface_only>::value
                   == strategy_typing::element_typed);

    // --- `none` for non-strategies and for descriptive-only strategies ---
    D_MT_CHECK(strategy_typing_of<ms_plain_type>::value
                   == strategy_typing::none);
    D_MT_CHECK(strategy_typing_of<int>::value == strategy_typing::none);
    D_MT_CHECK(strategy_typing_of<static_extent_strategy<int, 4>>::value
                   == strategy_typing::none);

    // --- supplies storage through no recognized typing ---
    D_MT_CHECK(strategy_typing_of<ms_void_value_element>::value
                   == strategy_typing::none);

    // --- half surfaces fall through to `none` ---
    D_MT_CHECK(strategy_typing_of<ms_element_no_dealloc>::value
                   == strategy_typing::none);
    D_MT_CHECK(strategy_typing_of<ms_byte_no_dealloc>::value
                   == strategy_typing::none);

    // --- the enum answer and the boolean traits never disagree ---
    D_MT_CHECK((strategy_typing_of<ms_element_dynamic>::value
                    == strategy_typing::element_typed)
                   == is_element_strategy<ms_element_dynamic>::value);
    D_MT_CHECK((strategy_typing_of<ms_byte_fixed>::value
                    == strategy_typing::byte_typed)
                   == is_byte_strategy<ms_byte_fixed>::value);

    // --- cv-ref agnostic ---
    D_MT_CHECK(strategy_typing_of<const ms_dual_typed&>::value
                   == strategy_typing::element_typed);
    D_MT_CHECK(strategy_typing_of<const monotonic_byte_arena<64>&>::value
                   == strategy_typing::byte_typed);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(strategy_typing_of_v<ms_dual_typed>
                   == strategy_typing_of<ms_dual_typed>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
