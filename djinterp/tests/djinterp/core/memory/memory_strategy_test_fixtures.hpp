/*******************************************************************************
* djinterp [testing]                           memory_strategy_test_fixtures.hpp
*
* Shared fixtures for the memory-strategy axis suites.
*   The `ms_*` types here are DECLARATION-ONLY: their member functions are
* declared and never defined.  Every structural trait on the axis probes them
* in an unevaluated context (decltype / declval), so declarations suffice - and
* the missing definitions are a guard rail: any test that accidentally odr-uses
* a fixture fails at link rather than reading garbage.  Runtime behaviour is
* exercised against the REAL strategies (arena, buffer, allocator, pool), never
* against a fixture.
*
*   This header is included by each per-module suite header behind its
* `#ifndef DTEST_SPEC_MODE` guard (fixtures name types under test, which the
* spec face does not include).  It carries the whole zoo - the abstract strategy
* fixtures, the container-shape fixtures, and the concept-constraint probes - so
* the individual suites share one definition instead of duplicating it.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_test_fixtures.hpp
*                                     memory_strategy_test_fixtures.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_MEMORY_STRATEGY_TEST_FIXTURES_
#define DJINTERP_TESTING_MEMORY_STRATEGY_TEST_FIXTURES_ 1

// std
#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp - the agnostic core, the concept layer, and the three concrete
// strategies the axis tests validate against.  Pulling them in here makes this
// the shared prelude for every strategy suite: each suite header includes only
// this file (plus, for container and pool, their one extra header).
#include "djinterp/djinterp.hpp"
#include <djinterp/core/memory/memory_strategy_traits.hpp>     // storage_kind / strategy_typing vocabulary
#include <djinterp/core/memory/memory_strategy_concepts.hpp>   // the concepts the probes constrain on
#include <djinterp/core/memory/allocator_memory_strategy.hpp>  // allocator_memory_strategy
#include <djinterp/core/memory/static_buffer_strategy.hpp>     // static_buffer_strategy + static_extent_strategy
#include <djinterp/core/memory/monotonic_byte_arena.hpp>       // monotonic_byte_arena + dynamic_byte_arena


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Fixtures - non-strategies
// ===========================================================================

// ms_plain_type
//   struct: not a strategy in any sense - no declared constants, no typed
// surface.  The negative control for every predicate on the axis.
struct ms_plain_type
{};

// ms_element_surface_only
//   struct: a complete element-typed SURFACE that declares no storage
// discipline.  Separates "speaks a surface" from "is a memory strategy" -
// std::allocator<T> has exactly this shape, and is exactly this answer.
struct ms_element_surface_only
{
    using value_type = int;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};

// ms_stable_non_strategy
//   struct: declares `pointer_stable` but not `strategy_storage_kind`.
// Isolates a deliberate trait/concept divergence: the refinement TRAITS read
// the constant unconditionally, while the CONCEPTS gate on membership first.
struct ms_stable_non_strategy
{
    static constexpr bool pointer_stable = true;
};

// ms_mistyped_kind_constant
//   struct: declares a `strategy_storage_kind` that is NOT a storage_kind.
// Detection is by NAME AND TYPE, so this type is REJECTED - it is not a memory
// strategy, and because it is rejected the kind accessors are total on it.
//
//   memory_strategy_kind_of returns unknown for it (the primary, constant-absent
// branch); is_memory_strategy is false; the typed predicates follow.  No accessor
// hard-errors on it any more.  It is here to pin that the type gate rejects a
// wrong-typed constant cleanly.
struct ms_mistyped_kind_constant
{
    static constexpr int strategy_storage_kind = 0;
};


// ===========================================================================
// III. Fixtures - descriptive-only strategies
// ===========================================================================

// ms_descriptive_static
//   struct: declares static storage and nothing else - the minimal memory
// strategy.  Membership asks for the kind constant and nothing more.
struct ms_descriptive_static
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
};

// ms_descriptive_fixed
//   struct: the minimal fixed-storage strategy.
struct ms_descriptive_fixed
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;
};

// ms_descriptive_dynamic
//   struct: the minimal dynamic-storage strategy.
struct ms_descriptive_dynamic
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
};

// ms_descriptive_unknown
//   struct: declares the `unknown` kind explicitly.  Edge case: it IS a memory
// strategy - the constant is present - yet answers false to all three
// discipline predicates.  Membership and discipline are separate questions.
struct ms_descriptive_unknown
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::unknown;
};

// ms_descriptive_releasing
//   struct: descriptive-only, yet declares individual release AND generational
// sweep.  Edge case: the refinement traits report both (they read constants,
// not behaviour), it is nonetheless NOT monotonic (monotonicity requires
// allocating), and the matching concepts reject it for that same reason.
struct ms_descriptive_releasing
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;
    static constexpr bool         supports_individual_release = true;
    static constexpr bool         supports_generational_sweep = true;
};


// ===========================================================================
// IV.  Fixtures - element-typed strategies
// ===========================================================================

// ms_element_dynamic
//   struct: the canonical operational element strategy - dynamic, unstable,
// individually releasing.  std::allocator's semantics, declared.
struct ms_element_dynamic
{
    using value_type = int;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = false;
    static constexpr bool         supports_individual_release = true;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};

// ms_element_static_monotonic
//   struct: element-typed, static, pointer-stable, monotonic - the shape of
// static_buffer_strategy reduced to declarations.
struct ms_element_static_monotonic
{
    using value_type = double;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};

// ms_element_generational
//   struct: the only allocating fixture that opts into generational sweep.
struct ms_element_generational
{
    using value_type = long;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = true;
    static constexpr bool         supports_generational_sweep = true;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};

// ms_element_no_dealloc
//   struct: allocate(n) with no deallocate(p,n) - a HALF surface.  Proves the
// element surface is detected as a conjunction, not by the supply verb alone.
struct ms_element_no_dealloc
{
    using value_type = int;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    value_type*  allocate(std::size_t _n);
};

// ms_element_no_alloc
//   struct: deallocate(p,n) with no allocate(n) - the mirror half surface.
struct ms_element_no_alloc
{
    using value_type = int;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    void  deallocate(value_type* _p, std::size_t _n);
};

// ms_void_value_element
//   struct: `value_type = void` with a one-argument allocate.  Edge case: both
// raw element-surface traits fire (deallocate(void*, size_t) IS
// deallocate(value_type*, size_t) here), but is_element_strategy's non-void
// guard rejects it, and a one-argument allocate is no byte surface either - so
// it is a memory strategy that supplies storage through NO recognized typing,
// and classifies as descriptive-only.  This is the guard's whole purpose.
struct ms_void_value_element
{
    using value_type = void;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;

    void*  allocate(std::size_t _bytes);
    void   deallocate(void* _p, std::size_t _bytes);
};

// ms_byte_cell_element
//   struct: a BYTE-GRANULAR element strategy (sizeof(value_type) == 1) - the
// only shape byte_strategy_view's enable_if admits.
struct ms_byte_cell_element
{
    using value_type = unsigned char;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = true;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};


// ===========================================================================
// V.   Fixtures - byte-typed strategies
// ===========================================================================

// ms_byte_fixed
//   struct: the canonical byte-typed strategy - pmr-shaped, fixed storage, and
// carrying NO value_type, which is the point: byte typing never asks for one.
struct ms_byte_fixed
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::fixed_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;

    void*  allocate(std::size_t _bytes, std::size_t _align);
    void   deallocate(void* _p, std::size_t _bytes, std::size_t _align);
};

// ms_byte_releasing
//   struct: a byte strategy that DOES release individually - so it is
// allocating but not monotonic.  Separates the byte typing from the monotonic
// discipline, which the arenas conflate by happening to be both.
struct ms_byte_releasing
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = false;
    static constexpr bool         supports_individual_release = true;

    void*  allocate(std::size_t _bytes, std::size_t _align);
    void   deallocate(void* _p, std::size_t _bytes, std::size_t _align);
};

// ms_byte_no_dealloc
//   struct: allocate(bytes,align) with no matching three-argument deallocate -
// the byte half surface.
struct ms_byte_no_dealloc
{
    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;

    void*  allocate(std::size_t _bytes, std::size_t _align);
};

// ms_dual_typed
//   struct: declares BOTH complete surfaces, by overload.  The edge case
// strategy_typing_of's precedence rule exists for: element wins.
struct ms_dual_typed
{
    using value_type = int;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = false;
    static constexpr bool         supports_individual_release = true;

    value_type*  allocate(std::size_t _n);
    void*        allocate(std::size_t _bytes, std::size_t _align);
    void         deallocate(value_type* _p, std::size_t _n);
    void         deallocate(void* _p, std::size_t _bytes, std::size_t _align);
};


// ===========================================================================
// VI.  Fixtures - container shapes
// ===========================================================================

// ms_container_explicit
//   struct: names its strategy outright - precedence step 1.  The member alias
// deliberately shares the spelling of the `memory_strategy` CONCEPT; class
// scope shadows the namespace name, and the resolver must still find it.
struct ms_container_explicit
{
    using value_type      = int;
    using memory_strategy = ms_element_dynamic;
};

// ms_container_alloc_and_alias
//   struct: declares BOTH an explicit strategy alias and an allocator_type.
// The precedence edge case: step 1 must beat step 2, and the two resolver
// specializations must not be ambiguous.
struct ms_container_alloc_and_alias
{
    using value_type      = int;
    using allocator_type  = std::allocator<int>;
    using memory_strategy = ms_element_static_monotonic;
};

// ms_container_allocator
//   struct: an allocator_type and no alias - precedence step 2.
struct ms_container_allocator
{
    using value_type     = char;
    using allocator_type = std::allocator<char>;
};

// ms_container_extent
//   struct: a compile-time extent plus a value_type - precedence step 3.
struct ms_container_extent
{
    using value_type = double;

    static constexpr std::size_t extent = 8;
};

// ms_container_extent_no_vt
//   struct: an extent with NO value_type.  Edge case: the resolver's
// detected_or_t fallback substitutes `unsigned char`.
struct ms_container_extent_no_vt
{
    static constexpr std::size_t extent = 4;
};

// ms_container_growable
//   struct: capacity() + reserve(), nothing else.  Edge case: no strategy
// resolves (type is void, resolved is false) yet shape inference still yields
// dynamic_storage - resolution and inference are independent answers.
struct ms_container_growable
{
    using value_type = int;

    std::size_t  capacity() const;
    void         reserve(std::size_t _n);
};

// ms_container_fixed_cap
//   struct: capacity() WITHOUT reserve() - non-growable, so shape inference
// yields fixed_storage.  The one bit that separates fixed from dynamic.
struct ms_container_fixed_cap
{
    using value_type = int;

    std::size_t  capacity() const;
};

// ms_container_opaque
//   struct: no shape at all - inference yields unknown.
struct ms_container_opaque
{};


// ===========================================================================
// VII. Concept-constraint probes
// ===========================================================================
// A concept's value is only half of it; the other half is whether it works
// where concepts are for - CONSTRAINING an overload.  Each probe pair below is
// two templates with identical signatures, separated only by their constraint,
// so the answer is decided by constraint partial ordering and nothing else.

// ms_probe_memory_strategy
//   function: 1 when _Type satisfies memory_strategy, else 0.
template<typename _Type>
    requires ::djinterp::memory_strategy<_Type>
int
ms_probe_memory_strategy()
{
    return 1;
}

template<typename _Type>
int
ms_probe_memory_strategy()
{
    return 0;
}

// ms_probe_element_strategy
//   function: 1 when _Type satisfies element_strategy, else 0.
template<typename _Type>
    requires ::djinterp::element_strategy<_Type>
int
ms_probe_element_strategy()
{
    return 1;
}

template<typename _Type>
int
ms_probe_element_strategy()
{
    return 0;
}

// ms_probe_byte_strategy
//   function: 1 when _Type satisfies byte_strategy, else 0.
template<typename _Type>
    requires ::djinterp::byte_strategy<_Type>
int
ms_probe_byte_strategy()
{
    return 1;
}

template<typename _Type>
int
ms_probe_byte_strategy()
{
    return 0;
}

// ms_probe_element_refinement
//   function: 2 when _Type satisfies allocating_element_strategy, 1 when it is
// merely an element_strategy, 0 otherwise.  allocating_element_strategy is a
// CONJUNCTION of two atomic constraints, one of which is element_strategy, so
// it subsumes element_strategy and must win without ambiguity.  This is the
// only test on the axis that checks the concept ALGEBRA rather than a value.
template<typename _Type>
int
ms_probe_element_refinement()
{
    return 0;
}

template<typename _Type>
    requires ::djinterp::element_strategy<_Type>
int
ms_probe_element_refinement()
{
    return 1;
}

template<typename _Type>
    requires ::djinterp::allocating_element_strategy<_Type>
int
ms_probe_element_refinement()
{
    return 2;
}


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_MEMORY_STRATEGY_TEST_FIXTURES_
