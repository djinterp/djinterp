/******************************************************************************
* djinterp [test]                                      pool_allocator_tests.hpp
*
*   Unit-test suite for the djinterp pool allocator (`pool_allocator.hpp`).
*
*   STRUCTURE:
*   pool_allocator is a lightweight, stateful handle: it stores a
* non-owning pointer to a pool_resource and forwards single-slot requests
* to it, falling back to global operator new for everything else.  The
* suite is organised around that split - the type surface it must present
* to satisfy the Allocator named requirements, the handle's own lifetime,
* the two allocation paths, object construction, equality, and finally the
* behaviour when a real container is placed on top.
*
*   THE TWO PATHS MATTER:
*   allocate(1) draws a slot from the pool; allocate(n > 1) bypasses it
* entirely.  Almost every test in the allocate section exists to pin which
* side of that fork a given call lands on, because a silent fall-through to
* operator new is invisible at the call site and only shows up as a pool
* whose size never moves.
*
*   REBINDING IS LOSSY BY DESIGN:
*   a rebound allocator cannot share a typed pool, so the converting
* constructor deliberately produces a handle with no pool at all.  That is
* a documented property with real consequences for node-based containers,
* and it is tested as such rather than worked around.
*
*
* BUILD PREREQUISITE
* ==================
*   pool_allocator.hpp includes pool.hpp, so it inherits every prerequisite
* listed in pool_tests.hpp's preamble - in particular the generation-counter
* and concept/class name-collision defects, either of which stops this suite
* compiling.  Fix those first; this delivery edits neither module.
*
*   No further change is needed to compile this suite.
*
*
* EXPECTED FAILURES AGAINST THE UNFIXED MODULE
* ============================================
*   One test asserts the correct contract and FAILS against the module as
* shipped.  The failure is the finding, and it is expected to turn green
* when the module is corrected.
*
*   A. tests_allocated_storage_is_suitably_aligned  (allocate)
*      The bulk path - `allocate(n)` for n > 1 - uses plain
*      `::operator new(_n * sizeof(_Type))`, which guarantees only
*      __STDCPP_DEFAULT_NEW_ALIGNMENT__ and is insufficient for an
*      over-aligned _Type; `deallocate` then frees it with the non-aligned
*      `::operator delete`, which is mismatched even had the allocation
*      been correct.
*      FIX: allocate with
*        ::operator new(_n * sizeof(_Type), std::align_val_t(alignof(_Type)))
*      and free with the matching
*        ::operator delete(static_cast<void*>(_p),
*                          std::align_val_t(alignof(_Type)))
*      (This test also covers the pool-backed path, which additionally
*      depends on the block-alignment fix noted in pool_tests.hpp.)
*
*      CHECK THIS ONE WITHOUT SANITIZERS.  AddressSanitizer's allocator
*      over-aligns its returns and hides the defect entirely: built with
*      -fsanitize=address this suite reports 42/42 against the unfixed
*      module.  Built at -O0 with no sanitizer it fails as described.
*
*
* TABLE OF CONTENTS
* =================
* I.    INCLUDES  (mode gated)
* II.   CHECK MACRO
* III.  FIXTURES - POLICIES AND ELEMENT TYPES
* IV.   FIXTURES - ALLOCATOR AND POOL MATRIX
* V.    FIXTURES - HELPERS
* VI.   DECLARATIONS
* VII.  SPEC PROVIDER
*
*
* path:      /tests/djinterp/core/memory/pool_allocator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#ifndef DJINTERP_TESTS_POOL_ALLOCATOR_TESTS_
#define DJINTERP_TESTS_POOL_ALLOCATOR_TESTS_ 1


// =========================================================================
// I.   INCLUDES
// =========================================================================

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>
// djinterp
#include "djinterp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/memory/pool/pool_allocator.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP

//   The framework namespace, declared so the alias below is well-formed
// in NORMAL mode too - there no djinterp/test header is included.
NS_TEST
NS_END  // test

NS_TESTING

namespace dt = ::djinterp::test;


// =========================================================================
// II.  CHECK MACRO
// =========================================================================

// pool_allocator_check
//   function: reports a failed check and returns the condition.
inline bool
pool_allocator_check(
    bool        _cond,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

// D_PA_CHECK
//   macro: evaluate once, report on failure, early-return false.
#define D_PA_CHECK(...)                                                     \
    do                                                                      \
    {                                                                       \
        if (!::djinterp::testing::pool_allocator_check(                     \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))          \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
    } while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// =========================================================================
// III. FIXTURES - POLICIES AND ELEMENT TYPES
// =========================================================================

// pa_growth_exact
//   struct: grows to exactly the requested capacity, so the pool behind
// an allocator has predictable capacity at every point.
struct pa_growth_exact
{
    static constexpr bool can_grow = true;

    static constexpr std::size_t
    compute(
        std::size_t /*_current*/,
        std::size_t _requested
    ) noexcept
    {
        return _requested;
    }
};

// pa_growth_none
//   struct: refuses all growth, so a bounded pool can be driven to
// exhaustion and the allocator's throwing path observed.
struct pa_growth_none
{
    static constexpr bool can_grow = false;

    static constexpr std::size_t
    compute(
        std::size_t _current,
        std::size_t /*_requested*/
    ) noexcept
    {
        return _current;
    }
};

// pa_element
//   struct: the ordinary element type - trivially copyable, naturally
// aligned, comfortably larger than a free-list link.
struct pa_element
{
    std::uint64_t m_lo;
    std::uint64_t m_hi;
};

// pa_other
//   struct: a second, differently sized element type, used to drive
// rebinding onto a genuinely different specialisation.
struct pa_other
{
    std::uint32_t m_data[6];
};

// pa_overaligned
//   struct: an over-aligned element, so the allocator's obligation to
// return suitably aligned storage is testable rather than accidental.
struct alignas(32) pa_overaligned
{
    std::uint64_t m_data[4];
};

// pa_counted
//   struct: counts its own constructions and destructions, so
// construct() and destroy() can be observed rather than inferred.  The
// counters are static because the allocator's construct/destroy take no
// other channel through which to report.
struct pa_counted
{
    static inline int s_constructed = 0;
    static inline int s_destroyed   = 0;

    int m_first;
    int m_second;

    pa_counted() noexcept
        : m_first(0),
          m_second(0)
    {
        ++s_constructed;
    }

    pa_counted(
        int _first,
        int _second
    ) noexcept
        : m_first(_first),
          m_second(_second)
    {
        ++s_constructed;
    }

    ~pa_counted()
    {
        ++s_destroyed;
    }

    // copying is counted too, so a stray copy cannot hide
    pa_counted(
        const pa_counted& _other
    ) noexcept
        : m_first(_other.m_first),
          m_second(_other.m_second)
    {
        ++s_constructed;
    }

    pa_counted& operator=(const pa_counted&) noexcept = default;

    // reset_counts
    //   resets the shared counters before a measurement.
    static void
    reset_counts() noexcept
    {
        s_constructed = 0;
        s_destroyed   = 0;

        return;
    }
};


// =========================================================================
// IV.  FIXTURES - ALLOCATOR AND POOL MATRIX
// =========================================================================
//   Each allocator alias is paired with the pool_resource it must name,
// so a mismatch between the two shows up as a compile error in the suite
// rather than as a silent fall-through at run time.

using pa_alloc =
    pool_allocator<pa_element,
                   free_list_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_exact>;

using pa_pool = pa_alloc::pool_type;

using pa_other_alloc =
    pool_allocator<pa_other,
                   free_list_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_exact>;

using pa_other_pool = pa_other_alloc::pool_type;

using pa_counted_alloc =
    pool_allocator<pa_counted,
                   free_list_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_exact>;

using pa_counted_pool = pa_counted_alloc::pool_type;

using pa_over_alloc =
    pool_allocator<pa_overaligned,
                   free_list_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_exact>;

using pa_over_pool = pa_over_alloc::pool_type;

using pa_fixed_alloc =
    pool_allocator<pa_element,
                   free_list_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_none>;

using pa_fixed_pool = pa_fixed_alloc::pool_type;

using pa_mono_alloc =
    pool_allocator<pa_element,
                   monotonic_release_policy,
                   chunked_block_policy<4>,
                   pa_growth_exact>;

using pa_mono_pool = pa_mono_alloc::pool_type;


// =========================================================================
// V.   FIXTURES - HELPERS
// =========================================================================

// pa_is_aligned
//   function: true when _ptr satisfies _align.
inline bool
pa_is_aligned(
    const void* _ptr,
    std::size_t _align
) noexcept
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(_ptr);

    return ( (address % static_cast<std::uintptr_t>(_align)) == 0u );
}

// pa_plain_container
//   struct: names a non-pool allocator, so the container classification
// has a negative case.
struct pa_plain_container
{
    struct plain_allocator
    {
        using value_type = pa_element;
    };

    using allocator_type = plain_allocator;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// =========================================================================
// VI.  DECLARATIONS
// =========================================================================

// I.   TYPE SURFACE            (pool_allocator_tests_types.cpp)
bool tests_allocator_exposes_standard_type_aliases();
bool tests_allocator_pool_type_alias_matches_its_policies();
bool tests_allocator_propagation_traits();
bool tests_allocator_rebind_produces_the_expected_type();
bool tests_allocator_rebind_preserves_the_policy_triple();
bool tests_allocator_traits_agree_with_the_allocator();

// II.  HANDLE LIFETIME         (pool_allocator_tests_construction.cpp)
bool tests_allocator_binds_to_its_pool();
bool tests_allocator_pool_constructor_is_explicit();
bool tests_allocator_copy_shares_the_pool();
bool tests_allocator_copy_assignment_rebinds();
bool tests_allocator_converting_constructor_yields_no_pool();
bool tests_allocator_set_resource_rebinds();
bool tests_allocator_is_not_default_constructible();
bool tests_allocator_operations_are_noexcept();
bool tests_select_on_container_copy_shares_the_pool();

// III. ALLOCATION              (pool_allocator_tests_allocate.cpp)
bool tests_single_allocation_comes_from_the_pool();
bool tests_single_deallocation_returns_to_the_pool();
bool tests_bulk_allocation_bypasses_the_pool();
bool tests_zero_length_allocation();
bool tests_deallocate_ignores_a_null_pointer();
bool tests_allocation_without_a_pool_falls_back();
bool tests_allocation_throws_when_the_pool_is_exhausted();
bool tests_allocated_storage_is_suitably_aligned();
bool tests_allocation_round_trip_recycles_slots();

// IV.  OBJECT LIFECYCLE        (pool_allocator_tests_objects.cpp)
bool tests_construct_creates_an_object_in_place();
bool tests_construct_forwards_its_arguments();
bool tests_destroy_runs_the_destructor();
bool tests_max_size_reports_the_element_bound();
bool tests_allocator_traits_construct_and_destroy();
bool tests_full_object_lifecycle_through_the_pool();

// V.   EQUALITY                (pool_allocator_tests_equality.cpp)
bool tests_allocators_on_one_pool_compare_equal();
bool tests_allocators_on_different_pools_compare_unequal();
bool tests_copies_compare_equal_to_their_source();
bool tests_cross_type_allocators_compare_by_null_resource();
bool tests_inequality_is_the_negation_of_equality();
bool tests_equality_is_noexcept_and_const();

// VI.  INTEGRATION             (pool_allocator_tests_integration.cpp)
bool tests_default_pool_allocator_alias();
bool tests_monotonic_pool_allocator_alias();
bool tests_allocator_is_classified_by_the_pool_traits();
bool tests_container_with_a_pool_allocator_is_pool_backed();
bool tests_vector_backed_by_a_pool_allocator();
bool tests_rebound_allocator_silently_bypasses_the_pool();


// =========================================================================
// VII. SPEC PROVIDER
// =========================================================================

#ifdef DTEST_SPEC_MODE
inline dt::module_spec
pool_allocator_spec()
{
    return dt::module_spec{
        "pool_allocator",
        "STL-conforming stateful allocator backed by a pool_resource - a "
        "non-owning handle that satisfies the C++ Allocator named "
        "requirements, routes single-slot requests through the pool, and "
        "falls back to global operator new for everything else.",
        {
            dt::block_spec{
                "types",
                "The type surface the Allocator named requirements demand: "
                "the standard aliases, the propagation traits, the pool "
                "type, and rebinding.",
                {
                    { "tests_allocator_exposes_standard_type_aliases",
                      "value_type, pointer, reference, size_type and "
                      "difference_type are present and correctly formed",
                      &tests_allocator_exposes_standard_type_aliases },
                    { "tests_allocator_pool_type_alias_matches_its_policies",
                      "pool_type names pool_resource with exactly the "
                      "allocator's own element type and policy triple",
                      &tests_allocator_pool_type_alias_matches_its_policies },
                    { "tests_allocator_propagation_traits",
                      "all three propagate_on_container_* traits and "
                      "is_always_equal are false_type, matching the PMR "
                      "model for a stateful handle",
                      &tests_allocator_propagation_traits },
                    { "tests_allocator_rebind_produces_the_expected_type",
                      "rebind<U>::other is the allocator for U, and "
                      "rebinding to the same type is the identity",
                      &tests_allocator_rebind_produces_the_expected_type },
                    { "tests_allocator_rebind_preserves_the_policy_triple",
                      "only the element type changes under rebinding - the "
                      "release, block and growth policies survive",
                      &tests_allocator_rebind_preserves_the_policy_triple },
                    { "tests_allocator_traits_agree_with_the_allocator",
                      "std::allocator_traits reads back the same aliases, "
                      "propagation traits and rebind the allocator declares",
                      &tests_allocator_traits_agree_with_the_allocator },
                }
            },
            dt::block_spec{
                "construction",
                "The handle's own lifetime - binding to a pool, copying, "
                "assigning, rebinding, and the deliberately pool-less "
                "converting constructor.",
                {
                    { "tests_allocator_binds_to_its_pool",
                      "an allocator constructed from a pool reports that "
                      "pool through resource()",
                      &tests_allocator_binds_to_its_pool },
                    { "tests_allocator_pool_constructor_is_explicit",
                      "a pool does not convert implicitly into an allocator",
                      &tests_allocator_pool_constructor_is_explicit },
                    { "tests_allocator_copy_shares_the_pool",
                      "a copied allocator names the same pool - the handle "
                      "is copied, never the pool",
                      &tests_allocator_copy_shares_the_pool },
                    { "tests_allocator_copy_assignment_rebinds",
                      "copy assignment repoints the handle at the source's "
                      "pool",
                      &tests_allocator_copy_assignment_rebinds },
                    { "tests_allocator_converting_constructor_yields_no_pool",
                      "converting from an allocator of another type yields "
                      "a handle with no pool, because a typed pool cannot "
                      "be shared across element types",
                      &tests_allocator_converting_constructor_yields_no_pool },
                    { "tests_allocator_set_resource_rebinds",
                      "set_resource repoints an existing handle at a "
                      "different pool",
                      &tests_allocator_set_resource_rebinds },
                    { "tests_allocator_is_not_default_constructible",
                      "there is no default constructor - an allocator "
                      "always names a pool or is converted from one that "
                      "does",
                      &tests_allocator_is_not_default_constructible },
                    { "tests_allocator_operations_are_noexcept",
                      "construction, copying, assignment and the pool "
                      "accessors are all noexcept",
                      &tests_allocator_operations_are_noexcept },
                    { "tests_select_on_container_copy_shares_the_pool",
                      "a container copy inherits the same pool rather than "
                      "a fresh one",
                      &tests_select_on_container_copy_shares_the_pool },
                }
            },
            dt::block_spec{
                "allocate",
                "The two allocation paths - one slot from the pool, "
                "anything else from global operator new - and the "
                "boundaries between them.",
                {
                    { "tests_single_allocation_comes_from_the_pool",
                      "allocate(1) draws a slot from the bound pool and "
                      "moves its live count",
                      &tests_single_allocation_comes_from_the_pool },
                    { "tests_single_deallocation_returns_to_the_pool",
                      "deallocate(p, 1) returns the slot to the pool for "
                      "reuse rather than to the global allocator",
                      &tests_single_deallocation_returns_to_the_pool },
                    { "tests_bulk_allocation_bypasses_the_pool",
                      "allocate(n) for n greater than one leaves the pool "
                      "untouched and yields usable storage anyway",
                      &tests_bulk_allocation_bypasses_the_pool },
                    { "tests_zero_length_allocation",
                      "allocate(0) takes the bypass path and its result "
                      "round-trips through deallocate(p, 0)",
                      &tests_zero_length_allocation },
                    { "tests_deallocate_ignores_a_null_pointer",
                      "deallocate(nullptr, n) returns without touching the "
                      "pool, for both path lengths",
                      &tests_deallocate_ignores_a_null_pointer },
                    { "tests_allocation_without_a_pool_falls_back",
                      "an allocator with no pool serves single-slot "
                      "requests from operator new, symmetrically",
                      &tests_allocation_without_a_pool_falls_back },
                    { "tests_allocation_throws_when_the_pool_is_exhausted",
                      "a pool that cannot grow makes allocate(1) throw "
                      "std::bad_alloc rather than return null",
                      &tests_allocation_throws_when_the_pool_is_exhausted },
                    { "tests_allocated_storage_is_suitably_aligned",
                      "storage returned for an over-aligned element "
                      "satisfies that alignment on both paths",
                      &tests_allocated_storage_is_suitably_aligned },
                    { "tests_allocation_round_trip_recycles_slots",
                      "a long allocate/deallocate cycle reuses pool slots "
                      "and never grows the pool",
                      &tests_allocation_round_trip_recycles_slots },
                }
            },
            dt::block_spec{
                "objects",
                "Object construction and destruction over pool storage, "
                "and the maximum request the allocator admits.",
                {
                    { "tests_construct_creates_an_object_in_place",
                      "construct placement-news an object into the "
                      "supplied storage",
                      &tests_construct_creates_an_object_in_place },
                    { "tests_construct_forwards_its_arguments",
                      "constructor arguments reach the object unchanged, "
                      "including through a moved-from value",
                      &tests_construct_forwards_its_arguments },
                    { "tests_destroy_runs_the_destructor",
                      "destroy invokes the destructor exactly once, which "
                      "a counting element type observes directly",
                      &tests_destroy_runs_the_destructor },
                    { "tests_max_size_reports_the_element_bound",
                      "max_size is the address space divided by the "
                      "element size, and is const and noexcept",
                      &tests_max_size_reports_the_element_bound },
                    { "tests_allocator_traits_construct_and_destroy",
                      "std::allocator_traits routes construction and "
                      "destruction through the allocator's own members",
                      &tests_allocator_traits_construct_and_destroy },
                    { "tests_full_object_lifecycle_through_the_pool",
                      "allocate, construct, use, destroy and deallocate "
                      "compose with balanced counts and no leaked slots",
                      &tests_full_object_lifecycle_through_the_pool },
                }
            },
            dt::block_spec{
                "equality",
                "Allocator equality - two handles are interchangeable "
                "exactly when they name the same pool.",
                {
                    { "tests_allocators_on_one_pool_compare_equal",
                      "two independently constructed handles on one pool "
                      "compare equal and equality is reflexive",
                      &tests_allocators_on_one_pool_compare_equal },
                    { "tests_allocators_on_different_pools_compare_unequal",
                      "handles naming different pools of the same type "
                      "compare unequal",
                      &tests_allocators_on_different_pools_compare_unequal },
                    { "tests_copies_compare_equal_to_their_source",
                      "copying and copy-assigning preserve equality, so a "
                      "copy can free what its source allocated",
                      &tests_copies_compare_equal_to_their_source },
                    { "tests_cross_type_allocators_compare_by_null_resource",
                      "allocators of different element types are equal "
                      "only when both name no pool at all",
                      &tests_cross_type_allocators_compare_by_null_resource },
                    { "tests_inequality_is_the_negation_of_equality",
                      "operator!= is exactly the complement of operator== "
                      "in every case tested",
                      &tests_inequality_is_the_negation_of_equality },
                    { "tests_equality_is_noexcept_and_const",
                      "both comparison operators are noexcept and operate "
                      "on const references",
                      &tests_equality_is_noexcept_and_const },
                }
            },
            dt::block_spec{
                "integration",
                "The convenience aliases, the classification traits from "
                "pool.hpp applied to a real allocator, and a standard "
                "container running on top of one.",
                {
                    { "tests_default_pool_allocator_alias",
                      "default_pool_allocator expands to free-list, "
                      "chunked, exponential growth",
                      &tests_default_pool_allocator_alias },
                    { "tests_monotonic_pool_allocator_alias",
                      "monotonic_pool_allocator differs from the default "
                      "on the release axis alone",
                      &tests_monotonic_pool_allocator_alias },
                    { "tests_allocator_is_classified_by_the_pool_traits",
                      "is_pool_allocator, the resource extraction, and "
                      "pool_allocator_class all recognise a real allocator",
                      &tests_allocator_is_classified_by_the_pool_traits },
                    { "tests_container_with_a_pool_allocator_is_pool_backed",
                      "a container parameterised on a pool allocator is "
                      "classified as pool-backed",
                      &tests_container_with_a_pool_allocator_is_pool_backed },
                    { "tests_vector_backed_by_a_pool_allocator",
                      "a std::vector on a pool allocator stores and "
                      "retrieves values correctly and balances its "
                      "allocations",
                      &tests_vector_backed_by_a_pool_allocator },
                    { "tests_rebound_allocator_silently_bypasses_the_pool",
                      "a rebound allocator has no pool, so a node-based "
                      "container built on one allocates from operator new "
                      "- a documented consequence worth pinning",
                      &tests_rebound_allocator_silently_bypasses_the_pool },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_POOL_ALLOCATOR_TESTS_
