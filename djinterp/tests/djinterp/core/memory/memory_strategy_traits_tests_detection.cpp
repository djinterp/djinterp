/*******************************************************************************
* djinterp [testing]                  memory_strategy_traits_tests_detection.cpp
*
* Section: the raw structural detectors.
*   memory_strategy_traits.hpp II  - constant detection
*   memory_strategy_traits.hpp III - typed-surface detection
*
*   These traits are the axis's only contact with a strategy's actual shape;
* everything above them is arithmetic on their answers.  They are checked here
* RAW - before any composite cleans, guards, or prioritizes them - because the
* composites can only be trusted once it is known exactly what they are made
* of.  Two properties matter most and are easy to assume wrongly:
*     - detection is by NAME and SHAPE, never by meaning
*     - each surface is a CONJUNCTION; half a surface is no surface
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_detection.cpp
*                                       memory_strategy_tests_detection.cpp
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
tests_constant_detection
  The descriptive constant vocabulary (core II).
  Tests the following:
  - has_strategy_kind_constant fires on every declaring fixture and on none of
    the non-declaring ones
  - the three OPTIONAL constants are detected independently of one another and
    of the mandatory one: declaring pointer_stable alone is detected, and
    declaring the kind alone leaves the optional three undetected
  - detection of the kind constant is by NAME AND TYPE: a `strategy_storage_kind`
    of the wrong type is NOT detected, and therefore not admitted as a memory
    strategy - see ms_mistyped_kind_constant
  - every `_v` companion agrees with its trait's ::value
*/
bool
tests_constant_detection()
{

    // --- the mandatory constant: present ---
    D_MT_CHECK(has_strategy_kind_constant<ms_descriptive_static>::value);
    D_MT_CHECK(has_strategy_kind_constant<ms_descriptive_fixed>::value);
    D_MT_CHECK(has_strategy_kind_constant<ms_descriptive_dynamic>::value);
    D_MT_CHECK(has_strategy_kind_constant<ms_descriptive_unknown>::value);
    D_MT_CHECK(has_strategy_kind_constant<ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_kind_constant<ms_byte_fixed>::value);

    // --- the mandatory constant: absent ---
    D_MT_CHECK(!has_strategy_kind_constant<ms_plain_type>::value);
    D_MT_CHECK(!has_strategy_kind_constant<ms_element_surface_only>::value);
    D_MT_CHECK(!has_strategy_kind_constant<ms_stable_non_strategy>::value);
    D_MT_CHECK(!has_strategy_kind_constant<std::allocator<int>>::value);
    D_MT_CHECK(!has_strategy_kind_constant<int>::value);

    // --- pointer_stable, detected independently ---
    D_MT_CHECK(has_strategy_pointer_stable_constant<ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_pointer_stable_constant<
                   ms_stable_non_strategy>::value);
    D_MT_CHECK(!has_strategy_pointer_stable_constant<
                   ms_descriptive_static>::value);
    D_MT_CHECK(!has_strategy_pointer_stable_constant<ms_plain_type>::value);

    // --- supports_individual_release, detected independently ---
    D_MT_CHECK(has_strategy_individual_release_constant<
                   ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_individual_release_constant<
                   ms_descriptive_releasing>::value);
    D_MT_CHECK(!has_strategy_individual_release_constant<
                   ms_descriptive_static>::value);
    D_MT_CHECK(!has_strategy_individual_release_constant<
                   ms_element_no_dealloc>::value);

    // --- supports_generational_sweep, detected independently ---
    D_MT_CHECK(has_strategy_generational_constant<
                   ms_element_generational>::value);
    D_MT_CHECK(has_strategy_generational_constant<
                   ms_descriptive_releasing>::value);
    D_MT_CHECK(!has_strategy_generational_constant<ms_element_dynamic>::value);
    D_MT_CHECK(!has_strategy_generational_constant<ms_plain_type>::value);

    // --- detection is by NAME AND TYPE: the mistyped constant is rejected ---
    D_MT_CHECK(!has_strategy_kind_constant<ms_mistyped_kind_constant>::value);
    D_MT_CHECK(!is_memory_strategy<ms_mistyped_kind_constant>::value);

    // --- the `_v` companions agree ---
    D_MT_CHECK(has_strategy_kind_constant_v<ms_descriptive_static>
                   == has_strategy_kind_constant<ms_descriptive_static>::value);
    D_MT_CHECK(has_strategy_pointer_stable_constant_v<ms_element_dynamic>
                   == has_strategy_pointer_stable_constant<
                          ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_individual_release_constant_v<ms_element_dynamic>
                   == has_strategy_individual_release_constant<
                          ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_generational_constant_v<ms_element_generational>
                   == has_strategy_generational_constant<
                          ms_element_generational>::value);

    return true;
}


/*
tests_value_type_detection
  The nested value_type alias (core II) and the alias operation the composites
  read it through.
  Tests the following:
  - has_strategy_value_type fires on element fixtures and not on byte ones
  - it fires for `value_type = void` too: PRESENCE is all it claims, and the
    non-void question is asked separately by is_element_strategy
  - unlike its neighbours it is cv-ref agnostic, because D_TYPE_TRAIT_HAS_TYPE
    cleans the type before probing (the raw constant/method detectors do not -
    see tests_cv_ref_agnosticism)
  - internal::value_type_of_alias yields the alias when present
  - detected_or_t over that alias falls back when it is absent, which is the
    exact mechanism is_element_strategy and memory_strategy_traits rely on
*/
bool
tests_value_type_detection()
{

    // --- present ---
    D_MT_CHECK(has_strategy_value_type<ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_value_type<ms_byte_cell_element>::value);
    D_MT_CHECK(has_strategy_value_type<std::allocator<int>>::value);

    // --- present, and void: presence is the only claim ---
    D_MT_CHECK(has_strategy_value_type<ms_void_value_element>::value);
    D_MT_CHECK(has_strategy_value_type<monotonic_byte_arena<64>>::value);

    // --- absent ---
    D_MT_CHECK(!has_strategy_value_type<ms_byte_fixed>::value);
    D_MT_CHECK(!has_strategy_value_type<ms_plain_type>::value);
    D_MT_CHECK(!has_strategy_value_type<ms_descriptive_static>::value);

    // --- cv-ref agnostic (HAS_TYPE cleans; the raw detectors do not) ---
    D_MT_CHECK(has_strategy_value_type<const ms_element_dynamic>::value);
    D_MT_CHECK(has_strategy_value_type<ms_element_dynamic&>::value);
    D_MT_CHECK(has_strategy_value_type<const ms_element_dynamic&>::value);

    // --- the alias operation itself ---
    D_MT_CHECK(std::is_same<
                   internal::value_type_of_alias<ms_element_dynamic>,
                   int>::value);
    D_MT_CHECK(std::is_same<
                   internal::value_type_of_alias<ms_element_static_monotonic>,
                   double>::value);

    // --- detected_or_t: yield when present, fall back when absent ---
    D_MT_CHECK(std::is_same<
                   detected_or_t<void, internal::value_type_of_alias,
                                 ms_element_dynamic>,
                   int>::value);
    D_MT_CHECK(std::is_same<
                   detected_or_t<void, internal::value_type_of_alias,
                                 ms_byte_fixed>,
                   void>::value);
    D_MT_CHECK(std::is_same<
                   detected_or_t<void, internal::value_type_of_alias,
                                 ms_void_value_element>,
                   void>::value);

    return true;
}


/*
tests_element_surface_detection
  The element-typed surface: allocate(n) and deallocate(p,n) (core III).
  Tests the following:
  - has_element_allocate fires on a one-argument allocate and not on its
    absence
  - has_element_deallocate fires on deallocate(value_type*, size_t), and is
    guarded: a type with NO value_type cannot have the trait probed, and
    answers false rather than failing to compile
  - each half is detected independently: the two half-surface fixtures each
    trip exactly one of the pair
  - DEFAULT ARGUMENTS COUNT.  A byte arena's allocate(bytes, align = _MaxAlign)
    is callable with one argument, so the ELEMENT detector fires on a BYTE
    strategy.  This is not a bug in the detector - it reports callability, and
    the call is real - but it is the reason is_element_strategy cannot be built
    from these traits alone, and the whole justification for its non-void
    value_type guard.  Pinned here so the guard is never "simplified" away
  - the has_element_deallocate _v companion agrees with ::value
*/
bool
tests_element_surface_detection()
{

    // --- allocate(n) ---
    D_MT_CHECK(has_element_allocate<ms_element_dynamic>::value);
    D_MT_CHECK(has_element_allocate<ms_element_no_dealloc>::value);
    D_MT_CHECK(has_element_allocate<std::allocator<int>>::value);
    D_MT_CHECK(!has_element_allocate<ms_element_no_alloc>::value);
    D_MT_CHECK(!has_element_allocate<ms_plain_type>::value);
    D_MT_CHECK(!has_element_allocate<ms_descriptive_static>::value);

    // --- a two-argument-only allocate is not a one-argument allocate ---
    D_MT_CHECK(!has_element_allocate<ms_byte_fixed>::value);
    D_MT_CHECK(!has_element_allocate<ms_byte_no_dealloc>::value);

    // --- deallocate(value_type*, size_t) ---
    D_MT_CHECK(has_element_deallocate<ms_element_dynamic>::value);
    D_MT_CHECK(has_element_deallocate<ms_element_no_alloc>::value);
    D_MT_CHECK(has_element_deallocate<std::allocator<int>>::value);
    D_MT_CHECK(!has_element_deallocate<ms_element_no_dealloc>::value);
    D_MT_CHECK(!has_element_deallocate<ms_plain_type>::value);

    // --- guarded: no value_type means false, not a compile error ---
    D_MT_CHECK(!has_element_deallocate<ms_byte_fixed>::value);
    D_MT_CHECK(!has_element_deallocate<ms_byte_no_dealloc>::value);
    D_MT_CHECK(!has_element_deallocate<ms_descriptive_static>::value);
    D_MT_CHECK(!has_element_deallocate<int>::value);

    // --- the internal check the public trait cleans for ---
    D_MT_CHECK(internal::element_deallocate_check<ms_element_dynamic>::value);
    D_MT_CHECK(!internal::element_deallocate_check<ms_byte_fixed>::value);

    // --- default arguments count: a BYTE arena trips the ELEMENT detector ---
    D_MT_CHECK(has_element_allocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_element_deallocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_element_allocate<dynamic_byte_arena>::value);
    D_MT_CHECK(has_element_deallocate<dynamic_byte_arena>::value);

    // --- ... and yet neither is an element strategy.  The guard earns its keep
    D_MT_CHECK(!is_element_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(!is_element_strategy<dynamic_byte_arena>::value);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(has_element_allocate_v<ms_element_dynamic>
                   == has_element_allocate<ms_element_dynamic>::value);
    D_MT_CHECK(has_element_deallocate_v<ms_element_dynamic>
                   == has_element_deallocate<ms_element_dynamic>::value);

    return true;
}


/*
tests_byte_surface_detection
  The byte-typed surface: allocate(bytes,align) and deallocate(p,bytes,align)
  (core III).
  Tests the following:
  - has_byte_allocate fires on a two-argument allocate, has_byte_deallocate on
    a three-argument deallocate
  - neither asks for a value_type: the canonical byte fixture declares none,
    and is detected in full
  - each half is detected independently
  - a one-argument element allocate is not a byte allocate
  - default arguments count here too, in the mirror direction: the arenas'
    allocate(bytes, align = _MaxAlign) is callable BOTH ways, which is what
    makes them the dual-surface case the typing rule must arbitrate
  - the `_v` companions agree with their traits
*/
bool
tests_byte_surface_detection()
{

    // --- allocate(bytes, align) ---
    D_MT_CHECK(has_byte_allocate<ms_byte_fixed>::value);
    D_MT_CHECK(has_byte_allocate<ms_byte_releasing>::value);
    D_MT_CHECK(has_byte_allocate<ms_byte_no_dealloc>::value);
    D_MT_CHECK(has_byte_allocate<ms_dual_typed>::value);
    D_MT_CHECK(has_byte_allocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_byte_allocate<dynamic_byte_arena>::value);

    // --- a one-argument allocate is not a byte allocate ---
    D_MT_CHECK(!has_byte_allocate<ms_element_dynamic>::value);
    D_MT_CHECK(!has_byte_allocate<std::allocator<int>>::value);
    D_MT_CHECK(!has_byte_allocate<ms_plain_type>::value);
    D_MT_CHECK(!has_byte_allocate<ms_void_value_element>::value);

    // --- deallocate(void*, bytes, align) ---
    D_MT_CHECK(has_byte_deallocate<ms_byte_fixed>::value);
    D_MT_CHECK(has_byte_deallocate<ms_byte_releasing>::value);
    D_MT_CHECK(has_byte_deallocate<ms_dual_typed>::value);
    D_MT_CHECK(has_byte_deallocate<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(has_byte_deallocate<dynamic_byte_arena>::value);
    D_MT_CHECK(!has_byte_deallocate<ms_byte_no_dealloc>::value);
    D_MT_CHECK(!has_byte_deallocate<ms_element_dynamic>::value);
    D_MT_CHECK(!has_byte_deallocate<ms_plain_type>::value);

    // --- no value_type is asked for, and none is missed ---
    D_MT_CHECK(!has_strategy_value_type<ms_byte_fixed>::value);
    D_MT_CHECK(is_byte_strategy<ms_byte_fixed>::value);

    // --- the `_v` companions agree ---
    D_MT_CHECK(has_byte_allocate_v<ms_byte_fixed>
                   == has_byte_allocate<ms_byte_fixed>::value);
    D_MT_CHECK(has_byte_deallocate_v<ms_byte_fixed>
                   == has_byte_deallocate<ms_byte_fixed>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
