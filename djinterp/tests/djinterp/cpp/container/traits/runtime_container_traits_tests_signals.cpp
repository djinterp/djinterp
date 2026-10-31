/*******************************************************************************
* djinterp [cpp]                      runtime_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/runtime_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "runtime_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Structural signals
// ===========================================================================
//   Three independent probes.  One asks whether the type looks like a
// container at all; the other two ask whether it actively requires runtime
// storage.  None of the three consults either of the others, which is what
// lets the two umbrellas above them come apart.


/*
tests_has_size_accessor_signal
  Verifies the "is a container" probe - any size() accessor, constexpr or not.
  Tests the following:
  - a const size() is detected, and every standard container carries one
  - the probe supplies a CONST instance, so a size() that is not const-
    qualified is missed
  - access control participates: a private size() is invisible
  - a size() returning VOID still counts - the probe names the call
    expression's type and never constrains it, so this is a structural probe
    and not a semantic one
  - a type with no size() at all is not container-shaped, whatever else it has
*/
bool tests_has_size_accessor_signal()
{
    bool ok = true;

    // present
    D_RT_CHECK(has_size_accessor_signal<DRtShaped>::value == true);
    D_RT_CHECK(has_size_accessor_signal<rt_vector>::value == true);
    D_RT_CHECK(has_size_accessor_signal<rt_array>::value  == true);
    D_RT_CHECK(has_size_accessor_signal<DRtString>::value == true);
    D_RT_CHECK(has_size_accessor_signal<rt_deque>::value  == true);
    D_RT_CHECK(has_size_accessor_signal<DRtList>::value   == true);
    D_RT_CHECK(has_size_accessor_signal<DRtSet>::value    == true);
    D_RT_CHECK(has_size_accessor_signal<DRtMap>::value    == true);

    // const-ness is required
    D_RT_CHECK(has_size_accessor_signal<DRtSizeNonconst>::value == false);

    // access control participates
    D_RT_CHECK(has_size_accessor_signal<DRtSizePrivate>::value == false);

    // a void return still counts
    D_RT_CHECK(has_size_accessor_signal<DRtSizeVoid>::value == true);

    // absent
    D_RT_CHECK(has_size_accessor_signal<DRtNoSize>::value == false);
    D_RT_CHECK(has_size_accessor_signal<DRtPair>::value    == false);
    D_RT_CHECK(has_size_accessor_signal<DRtAllocatorNoShape>::value
                   == false);
    D_RT_CHECK(has_size_accessor_signal<int>::value  == false);
    D_RT_CHECK(has_size_accessor_signal<void>::value == false);

    return ok;
}

/*
tests_has_allocator_alias
  Verifies the heap-storage alias probe.
  Tests the following:
  - an allocator_type member alias is detected
  - every allocator-aware standard container carries one; std::array does not,
    having no allocator at all
  - the probe asks for a nested TYPE, so an `allocator_type` that names a
    static data member does not match
  - access control participates: a private alias is invisible
  - the probe does NOT require a container shape, so a type carrying only the
    alias is detected
*/
bool tests_has_allocator_alias()
{
    bool ok = true;

    // present
    D_RT_CHECK(has_allocator_alias<DRtAllocator>::value == true);
    D_RT_CHECK(has_allocator_alias<rt_vector>::value == true);
    D_RT_CHECK(has_allocator_alias<DRtString>::value == true);
    D_RT_CHECK(has_allocator_alias<rt_deque>::value  == true);
    D_RT_CHECK(has_allocator_alias<DRtList>::value   == true);
    D_RT_CHECK(has_allocator_alias<DRtSet>::value    == true);
    D_RT_CHECK(has_allocator_alias<DRtMap>::value    == true);

    // a fixed array has no allocator
    D_RT_CHECK(has_allocator_alias<rt_array>::value == false);

    // it must name a type
    D_RT_CHECK(has_allocator_alias<DRtAllocatorValue>::value == false);

    // access control participates
    D_RT_CHECK(has_allocator_alias<DRtAllocatorPrivate>::value == false);

    // no container shape is required
    D_RT_CHECK(   ( has_size_accessor_signal<DRtAllocatorNoShape>::value
                        == false )
               && ( has_allocator_alias<DRtAllocatorNoShape>::value
                        == true ));

    // absent
    D_RT_CHECK(has_allocator_alias<DRtShaped>::value == false);
    D_RT_CHECK(has_allocator_alias<DRtPair>::value   == false);
    D_RT_CHECK(has_allocator_alias<int>::value       == false);
    D_RT_CHECK(has_allocator_alias<void>::value      == false);

    return ok;
}

/*
tests_has_reserve_method_signal
  Verifies the growable-capacity probe.
  Tests the following:
  - a reserve() callable with a size_t is detected
  - std::vector and std::string carry it; the node-based and associative
    containers do NOT, so this probe and the allocator probe are not redundant
    with one another
  - a reserve() taking something a size_t will not convert to is missed, the
    probe supplying a size_t
  - the probe calls on a NON-const lvalue, so a const-qualified reserve() is
    accepted too - a const member function may be called on a non-const object
  - no container shape is required here either
*/
bool tests_has_reserve_method_signal()
{
    bool ok = true;

    // present
    D_RT_CHECK(has_reserve_method_signal<DRtReserve>::value == true);
    D_RT_CHECK(has_reserve_method_signal<rt_vector>::value == true);
    D_RT_CHECK(has_reserve_method_signal<DRtString>::value == true);

    // the two storage signals are not redundant: these have an allocator but
    // no reserve
    D_RT_CHECK(   ( has_allocator_alias<rt_deque>::value        == true )
               && ( has_reserve_method_signal<rt_deque>::value  == false ));
    D_RT_CHECK(   ( has_allocator_alias<DRtList>::value         == true )
               && ( has_reserve_method_signal<DRtList>::value   == false ));
    D_RT_CHECK(   ( has_allocator_alias<DRtSet>::value          == true )
               && ( has_reserve_method_signal<DRtSet>::value    == false ));
    D_RT_CHECK(has_reserve_method_signal<DRtMap>::value   == false);
    D_RT_CHECK(has_reserve_method_signal<rt_array>::value == false);

    // the argument type is required
    D_RT_CHECK(has_reserve_method_signal<DRtReserveWrongArg>::value
                   == false);

    // a const reserve() is accepted, being callable on a non-const lvalue
    D_RT_CHECK(has_reserve_method_signal<DRtReserveConst>::value == true);

    // no container shape is required
    D_RT_CHECK(   ( has_size_accessor_signal<DRtReserveNoShape>::value
                        == false )
               && ( has_reserve_method_signal<DRtReserveNoShape>::value
                        == true ));

    // absent
    D_RT_CHECK(has_reserve_method_signal<DRtShaped>::value == false);
    D_RT_CHECK(has_reserve_method_signal<DRtPair>::value   == false);
    D_RT_CHECK(has_reserve_method_signal<int>::value       == false);
    D_RT_CHECK(has_reserve_method_signal<void>::value      == false);

    return ok;
}

/*
tests_signals_are_independent
  Verifies that the three probes answer independently - none is derivable from
  the others, so each carries information the umbrellas above can use.
  Tests the following:
  - shape without either storage signal
  - each storage signal without the shape
  - each storage signal without the other, on a shaped carrier
  - all three together
  - none of the three
*/
bool tests_signals_are_independent()
{
    bool ok = true;

    // shape alone
    D_RT_CHECK(   ( has_size_accessor_signal<DRtShaped>::value   == true )
               && ( has_allocator_alias<DRtShaped>::value        == false )
               && ( has_reserve_method_signal<DRtShaped>::value  == false ));

    // allocator alone
    D_RT_CHECK(   ( has_size_accessor_signal<DRtAllocatorNoShape>::value
                        == false )
               && ( has_allocator_alias<DRtAllocatorNoShape>::value
                        == true )
               && ( has_reserve_method_signal<DRtAllocatorNoShape>::value
                        == false ));

    // reserve alone
    D_RT_CHECK(   ( has_size_accessor_signal<DRtReserveNoShape>::value
                        == false )
               && ( has_allocator_alias<DRtReserveNoShape>::value
                        == false )
               && ( has_reserve_method_signal<DRtReserveNoShape>::value
                        == true ));

    // shape and allocator, no reserve
    D_RT_CHECK(   ( has_size_accessor_signal<DRtAllocator>::value  == true )
               && ( has_allocator_alias<DRtAllocator>::value       == true )
               && ( has_reserve_method_signal<DRtAllocator>::value == false ));

    // shape and reserve, no allocator
    D_RT_CHECK(   ( has_size_accessor_signal<DRtReserve>::value   == true )
               && ( has_allocator_alias<DRtReserve>::value        == false )
               && ( has_reserve_method_signal<DRtReserve>::value  == true ));

    // all three
    D_RT_CHECK(   ( has_size_accessor_signal<rt_vector>::value   == true )
               && ( has_allocator_alias<rt_vector>::value        == true )
               && ( has_reserve_method_signal<rt_vector>::value  == true ));

    // none of the three
    D_RT_CHECK(   ( has_size_accessor_signal<DRtPair>::value   == false )
               && ( has_allocator_alias<DRtPair>::value        == false )
               && ( has_reserve_method_signal<DRtPair>::value  == false ));

    return ok;
}

/*
tests_signals_std_types
  Verifies the signals across the standard library, where the three separate
  most clearly.
  Tests the following:
  - every standard container is shape-detected, std::array included
  - std::array carries neither storage signal, being inline-stored and fixed
  - the contiguous growable containers carry both
  - the node-based and associative containers carry the allocator only
  - a std::pair carries none, having no size()
*/
bool tests_signals_std_types()
{
    bool ok = true;

    // all shape-detected
    D_RT_CHECK(has_size_accessor_signal<rt_vector>::value == true);
    D_RT_CHECK(has_size_accessor_signal<rt_array>::value  == true);
    D_RT_CHECK(has_size_accessor_signal<rt_deque>::value  == true);
    D_RT_CHECK(has_size_accessor_signal<DRtMap>::value    == true);

    // the fixed array carries neither storage signal
    D_RT_CHECK(   ( has_allocator_alias<rt_array>::value       == false )
               && ( has_reserve_method_signal<rt_array>::value == false ));

    // contiguous growable: both
    D_RT_CHECK(   ( has_allocator_alias<rt_vector>::value       == true )
               && ( has_reserve_method_signal<rt_vector>::value == true ));
    D_RT_CHECK(   ( has_allocator_alias<DRtString>::value       == true )
               && ( has_reserve_method_signal<DRtString>::value == true ));

    // node-based and associative: allocator only
    D_RT_CHECK(   ( has_allocator_alias<rt_deque>::value       == true )
               && ( has_reserve_method_signal<rt_deque>::value == false ));
    D_RT_CHECK(   ( has_allocator_alias<DRtMap>::value         == true )
               && ( has_reserve_method_signal<DRtMap>::value   == false ));

    // a pair carries none
    D_RT_CHECK(   ( has_size_accessor_signal<DRtPair>::value   == false )
               && ( has_allocator_alias<DRtPair>::value        == false )
               && ( has_reserve_method_signal<DRtPair>::value  == false ));

    return ok;
}

/*
tests_signals_cv_ref
  Verifies that all three probes honour the clean_t contract the header's
  preamble claims for them.
  Tests the following:
  - each probe is identical across all seven cv- and reference-qualified
    spellings, true and false alike
  - the volatile spellings in particular
  - the allocator probe too, which reaches clean_t through the alias macro
    rather than by spelling it directly - so the claim covers it only if the
    macro cooperates
*/
bool tests_signals_cv_ref()
{
    bool ok = true;

    D_RT_CV_REF_AGREE(has_size_accessor_signal, rt_vector,  true);
    D_RT_CV_REF_AGREE(has_size_accessor_signal, DRtShaped,  true);
    D_RT_CV_REF_AGREE(has_size_accessor_signal, DRtNoSize, false);
    D_RT_CV_REF_AGREE(has_size_accessor_signal, DRtPair,    false);

    D_RT_CV_REF_AGREE(has_allocator_alias, rt_vector, true);
    D_RT_CV_REF_AGREE(has_allocator_alias, rt_array,  false);
    D_RT_CV_REF_AGREE(has_allocator_alias, DRtShaped, false);

    D_RT_CV_REF_AGREE(has_reserve_method_signal, rt_vector, true);
    D_RT_CV_REF_AGREE(has_reserve_method_signal, rt_deque,  false);
    D_RT_CV_REF_AGREE(has_reserve_method_signal, DRtShaped, false);

    return ok;
}

/*
tests_signals_companions
  Verifies the `_v` companions the trait_detect engine emits for each probe.
  Tests the following:
  - each companion agrees with its ::value
  - each is usable in a constant expression
  - each trait derives from integral_constant, so it carries the standard
    value_type and type members a metafunction is expected to have
  - the block is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool tests_signals_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_RT_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        has_size_accessor_signal<rt_vector> >::value);
    D_RT_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        has_allocator_alias<rt_array> >::value);
    D_RT_STATIC_CHECK(
        std::is_same<has_reserve_method_signal<rt_vector>::value_type,
                     bool>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_RT_STATIC_CHECK(has_size_accessor_signal_v<rt_vector>);
    D_RT_STATIC_CHECK(has_allocator_alias_v<rt_array> == false);

    D_RT_CHECK(has_size_accessor_signal_v<rt_vector>
                   == has_size_accessor_signal<rt_vector>::value);
    D_RT_CHECK(has_size_accessor_signal_v<DRtPair>
                   == has_size_accessor_signal<DRtPair>::value);
    D_RT_CHECK(has_allocator_alias_v<rt_vector>
                   == has_allocator_alias<rt_vector>::value);
    D_RT_CHECK(has_allocator_alias_v<rt_array>
                   == has_allocator_alias<rt_array>::value);
    D_RT_CHECK(has_reserve_method_signal_v<rt_vector>
                   == has_reserve_method_signal<rt_vector>::value);
    D_RT_CHECK(has_reserve_method_signal_v<rt_deque>
                   == has_reserve_method_signal<rt_deque>::value);
    D_RT_CHECK(has_size_accessor_signal_v<const rt_vector&>
                   == has_size_accessor_signal<rt_vector>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
