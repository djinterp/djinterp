/*******************************************************************************
* djinterp [test]                                                 pool_tests.hpp
*
*   Unit-test suite for the djinterp pool memory module (`pool.hpp`).
*
*   STRUCTURE:
*   pool.hpp is a policy-based memory resource combining three orthogonal
* axes (release / block / growth), so coverage is structured around
* exercising each axis in isolation and then across their cross-product.
* Every section of the header under test maps to exactly one block of this
* spec and exactly one section translation unit.
*
*   TEST GROWTH POLICIES:
*   To keep growth behaviour deterministic and independent of whatever
* buffer.hpp ships, this header defines four minimal growth policies that
* satisfy the structural contract pool_resource expects (`can_grow`,
* `compute`).  These let the growth tests assert exact post-growth
* capacities without coupling the suite to production growth tuning.
*
*   TEST ELEMENT TYPES:
*   Five element types drive the slot-geometry computation across the
* interesting size/alignment combinations: below-link-size, exactly
* aligned, requiring round-up, link-dominating, and over-aligned.
*
*   PROBES, NOT INTERNALS:
*   Slot and block geometry expectations are computed from local mirror
* structs (`pool_link_probe`, `pool_block_probe`) whose layout matches
* `internal::free_node` / `internal::pool_block`.  The suite therefore
* asserts the documented geometry contract without reaching into the
* module's internal namespace.
*
*   PORTABILITY:
*   Every geometry expectation is expressed in terms of sizeof/alignof of
* the probe types rather than hard-coded byte counts, so the suite is
* correct on 32-bit and 64-bit targets alike.
*
*
* BUILD PREREQUISITE
* ==================
*   pool.hpp as shipped does NOT compile against this suite.  Three defects
* must be fixed in the module first; none is edited by this delivery.
*
*   1. GENERATION COUNTER NAMES BOTH ARMS  (blocking, section VI)
*      The `std::conditional_t` selecting the generation counter's type
*      instantiates BOTH arms, and the false arm names
*      `_ReleasePolicy::generation_type`.  Only generational_release_policy
*      publishes one, so every free-list and monotonic pool - and therefore
*      default_pool_resource, monotonic_pool_resource, flat_pool_resource,
*      default_pool_allocator and monotonic_pool_allocator - fails to
*      compile.  The header already carries a `FIXME (pre-existing)` here.
*      FIX: resolve the member type lazily, e.g. a deferred helper
*        template<typename _RP, typename _Fallback,
*                 bool = _RP::supports_generational_sweep>
*        struct generation_state_of              { using type = _Fallback; };
*        template<typename _RP, typename _Fallback>
*        struct generation_state_of<_RP,_Fallback,true>
*                              { using type = typename _RP::generation_type; };
*      and use it in place of the conditional_t alias.
*      PINNED BY: tests_non_generational_policies_instantiate (selection).
*
*   2. CONCEPT / CLASS NAME COLLISION  (blocking, section XVIII)
*      pool.hpp declares `concept pool_allocator` and `concept
*      monotonic_pool_allocator`; pool_allocator.hpp declares a class
*      template and an alias template of exactly those names.  Including
*      both - which pool_allocator.hpp does itself - is a redeclaration.
*      FIX: rename the two concepts to `pool_allocator_c` and
*      `monotonic_pool_allocator_c`, matching the `pool_resource_c`
*      convention already used in the same section, and update the uses in
*      `non_pool_allocator` and `pool_allocator_with_resource`.
*      PINNED BY: tests_allocator_and_container_concepts,
*      tests_resource_extraction_concepts (concepts).
*
*   3. is_pool_backed_container HARD-ERRORS  (blocking, section XIII)
*      `typename clean_type::allocator_type` is named inside an `&&`.
*      Short-circuiting is a runtime property - both operands are still
*      instantiated - so the trait fails to compile for any type without an
*      allocator_type (int, void, every pool) instead of reporting false.
*      FIX: read the nested type only in a specialisation selected when it
*      exists, e.g.
*        template<typename _T, bool = has_allocator_type_v<clean_t<_T>>>
*        struct pool_backed_check : std::false_type {};
*        template<typename _T> struct pool_backed_check<_T,true>
*          : std::bool_constant<is_pool_allocator_v<
*                typename clean_t<_T>::allocator_type>> {};
*      and have the trait inherit from it.
*      PINNED BY: tests_pool_backed_container_classification (traits).
*
*
* EXPECTED FAILURES AGAINST THE UNFIXED MODULE
* ============================================
*   Once the three defects above are fixed the suite compiles, and two
* tests then FAIL.  Both assert the correct contract; the failure is the
* finding, and each is expected to turn green when the module is corrected.
*
*   A. tests_generation_is_carried_across_moves  (generational)
*      `move_block_state` transfers only m_blocks, so a moved-to
*      generational pool restarts at generation 0 and reissues generations
*      it has already handed out.
*      FIX: transfer m_generation as well, under
*      `if constexpr (supports_generational_sweep)`.
*
*   B. tests_acquired_slots_honour_slot_alignment  (geometry)
*      Slots are carved from storage obtained with plain `::operator new`,
*      which guarantees only __STDCPP_DEFAULT_NEW_ALIGNMENT__ (typically
*      16).  For an alignas(32) element an unpatched probe returns 64 of 64
*      misaligned slots in the contiguous layout; the chunked layout is
*      additionally skewed because the block header is not padded up to the
*      slot alignment.
*      FIX: give pool_block a `header_size(slot_align)` that rounds the
*      header up to the slot alignment, and allocate every block with
*      `::operator new(size, std::align_val_t(align))` / the matching
*      aligned `::operator delete`, in both block policies.
*
*      CHECK THIS ONE WITHOUT SANITIZERS.  AddressSanitizer's allocator
*      over-aligns its returns, which masks the defect completely: built
*      with -fsanitize=address the suite reports 102/102 against the
*      unfixed module and only this test's disappearance would hint that
*      anything is wrong.  Built at -O0 with no sanitizer it fails as
*      described.  A sanitizer-only CI lane will not see this bug.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INCLUDES  (mode gated)
      ----------------------

II.   CHECK MACRO
      -----------

III.  FIXTURES - GROWTH POLICIES
      --------------------------

IV.   FIXTURES - ELEMENT TYPES
      ------------------------

V.    FIXTURES - GEOMETRY PROBES AND HELPERS
      --------------------------------------

VI.   FIXTURES - POOL TYPE MATRIX
      ---------------------------

VII.  FIXTURES - DUCK-TYPED CLASSIFICATION SUBJECTS
      ---------------------------------------------

VIII. DECLARATIONS
      ------------

IX.   SPEC PROVIDER
      -------------
*/

#ifndef DJINTERP_CORE_MEMORY_POOL_TESTS_
#define DJINTERP_CORE_MEMORY_POOL_TESTS_ 1


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
#include "../../../../../inc/djinterp/core/memory/pool/pool.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP

//   The framework namespace, declared (harmlessly re-opened in spec mode)
// so the alias below is well-formed in NORMAL mode too - there no
// djinterp/test header is included, and a namespace alias may not name a
// namespace that has never been declared.
NS_TEST
NS_END  // test

NS_TESTING

// dt names the framework entities (djinterp::test).  Declared
// unconditionally because the spec provider needs dt::module_spec.
namespace dt = ::djinterp::test;


// =========================================================================
// II.  CHECK MACRO
// =========================================================================

// pool_check
//   function: reports a failed check and returns the condition so the
// macro can early-return.  Self-contained - depends on nothing from the
// header under test, so it lives outside the fixture guard.
inline bool
pool_check(
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

// D_POOL_CHECK
//   macro: evaluate once, report on failure, early-return false.
// Variadic so a top-level comma inside a trait expression passes
// through whole.
#define D_POOL_CHECK(...)                                                     \
    do                                                                      \
    {                                                                       \
        if (!::djinterp::testing::pool_check(                               \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))          \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
    } while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// =========================================================================
// III. FIXTURES - GROWTH POLICIES
// =========================================================================
//   pool_resource's growth axis is parameterised on a stateless policy
// struct exposing two members:
//
//     static constexpr bool can_grow;
//     static constexpr std::size_t compute(std::size_t _current,
//                                          std::size_t _requested) noexcept;
//
//   The production policy (exponential_growth_policy, from buffer.hpp) is
// fine at runtime, but its exact return values would couple this suite to
// that policy's tuning.  The policies below are pure structural fits with
// deterministic, trivially verified behaviour.

// pool_growth_exact
//   struct: grows to exactly the requested capacity.  Because acquire()
// requests `m_capacity + 1`, this policy grows the pool one slot at a
// time, which makes every post-growth capacity value predictable.
struct pool_growth_exact
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

// pool_growth_step4
//   struct: grows by exactly four slots per request, floored at the
// requested amount.  Used to verify the retry-after-grow path and the
// interaction between growth quanta and chunk sizing.
struct pool_growth_step4
{
    static constexpr bool        can_grow = true;
    static constexpr std::size_t step     = 4;

    static constexpr std::size_t
    compute(
        std::size_t _current,
        std::size_t _requested
    ) noexcept
    {
        return ( ((_current + step) > _requested)
                 ? (_current + step)
                 : _requested );
    }
};

// pool_growth_double
//   struct: doubling growth with a floor of the request.  Used to show
// that capacity is driven by the policy, not by the pool.
struct pool_growth_double
{
    static constexpr bool can_grow = true;

    static constexpr std::size_t
    compute(
        std::size_t _current,
        std::size_t _requested
    ) noexcept
    {
        const std::size_t next = (_current == 0) ? 1 : (_current * 2);

        return ( (next > _requested) ? next : _requested );
    }
};

// pool_growth_none
//   struct: refuses all growth.  Used to verify the out-of-memory branch
// of acquire() and to show that reserve() bypasses the growth axis.
struct pool_growth_none
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


// =========================================================================
// IV.  FIXTURES - ELEMENT TYPES
// =========================================================================
//   These exercise slot-geometry computation across the interesting
// size/alignment combinations: smaller than the free-list link, requiring
// round-up, exactly aligned, link-dominating, and over-aligned.

// pool_small_t
//   struct: a single byte - smaller than the free-list link, so slot
// geometry must clamp the slot size up to the link size.
struct pool_small_t
{
    char m_value;
};

// pool_odd_t
//   struct: twelve bytes at four-byte alignment.  The slot alignment is
// raised to the link alignment and the size must then round up to a
// multiple of it - the only fixture that exercises the padding branch.
struct pool_odd_t
{
    char          m_lead;
    std::uint32_t m_mid;
    char          m_trail;
};

// pool_medium_t
//   struct: a sixteen-byte aggregate, comfortably larger than the link
// and already a multiple of its alignment.
struct pool_medium_t
{
    std::uint64_t m_lo;
    std::uint64_t m_hi;
};

// pool_large_t
//   struct: a sixty-four byte aggregate that always dominates the link.
struct pool_large_t
{
    std::uint64_t m_data[8];
};

// pool_overaligned_t
//   struct: thirty-two bytes at a forced thirty-two byte alignment.
// Exercises the branch of slot geometry where the element alignment
// exceeds the link alignment, and pins the requirement that acquired
// storage actually honours that alignment.
struct alignas(32) pool_overaligned_t
{
    std::uint64_t m_data[4];
};


// =========================================================================
// V.   FIXTURES - GEOMETRY PROBES AND HELPERS
// =========================================================================

// pool_link_probe
//   struct: a local mirror of internal::free_node.  Lets the suite state
// slot-geometry expectations without depending on the internal namespace.
struct pool_link_probe
{
    pool_link_probe* next;
};

// pool_block_probe
//   struct: a local mirror of internal::pool_block's header.  Used to
// predict the per-block overhead reported by bytes_allocated().
struct pool_block_probe
{
    pool_block_probe* next;
    std::size_t       slot_count;
};

// pool_expected_align
//   function: the slot alignment the module must compute for _Type -
// the stricter of the element and free-list-link alignments.
template<typename _Type>
constexpr std::size_t
pool_expected_align() noexcept
{
    return ( (alignof(_Type) > alignof(pool_link_probe))
             ? alignof(_Type)
             : alignof(pool_link_probe) );
}

// pool_expected_slot
//   function: the slot size the module must compute for _Type - the
// larger of the element and link sizes, rounded up to the slot
// alignment.
template<typename _Type>
constexpr std::size_t
pool_expected_slot() noexcept
{
    const std::size_t align = pool_expected_align<_Type>();
    const std::size_t raw   = ( (sizeof(_Type) > sizeof(pool_link_probe))
                                ? sizeof(_Type)
                                : sizeof(pool_link_probe) );

    return ( ((raw + align - 1) / align) * align );
}

// pool_expected_block_header
//   function: the per-block header cost for slots of _slot_align
// alignment - sizeof(pool_block) rounded up so the first slot starts on
// an aligned boundary.
constexpr std::size_t
pool_expected_block_header(
    std::size_t _slot_align
) noexcept
{
    const std::size_t align = ( (_slot_align > alignof(pool_block_probe))
                                ? _slot_align
                                : alignof(pool_block_probe) );

    return ( ((sizeof(pool_block_probe) + align - 1) / align) * align );
}

// pool_is_aligned
//   function: true when _ptr satisfies _align.
inline bool
pool_is_aligned(
    const void* _ptr,
    std::size_t _align
) noexcept
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(_ptr);

    return ( (address % static_cast<std::uintptr_t>(_align)) == 0u );
}

// pool_all_distinct
//   function: true when no two of the first _count entries of _slots
// hold the same address.  Quadratic, so callers keep _count small.
inline bool
pool_all_distinct(
    void* const* _slots,
    std::size_t  _count
) noexcept
{
    std::size_t i = 0;
    std::size_t j = 0;

    for (i = 0; i < _count; ++i)
    {
        for (j = (i + 1); j < _count; ++j)
        {
            if (_slots[i] == _slots[j])
            {
                return false;
            }
        }
    }

    return true;
}

// pool_slots_disjoint
//   function: true when no two of the first _count slots overlap, given
// a slot width of _slot_size bytes.
inline bool
pool_slots_disjoint(
    void* const* _slots,
    std::size_t  _count,
    std::size_t  _slot_size
) noexcept
{
    std::size_t i = 0;
    std::size_t j = 0;

    for (i = 0; i < _count; ++i)
    {
        for (j = (i + 1); j < _count; ++j)
        {
            const char* a = static_cast<const char*>(_slots[i]);
            const char* b = static_cast<const char*>(_slots[j]);
            const char* lo = (a < b) ? a : b;
            const char* hi = (a < b) ? b : a;

            if ( static_cast<std::size_t>(hi - lo) < _slot_size )
            {
                return false;
            }
        }
    }

    return true;
}


// =========================================================================
// VI.  FIXTURES - POOL TYPE MATRIX
// =========================================================================
//   The cross-product of the three axes, named <release>_<block>[_<growth>].
// The unsuffixed forms carry the exact growth policy so post-growth
// capacities are predictable.  Chunked variants use four slots per block
// so block boundaries are crossed within a handful of acquires.

using pool_fl_chunked =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_exact>;

using pool_fl_contig =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  contiguous_block_policy,
                  pool_growth_exact>;

using pool_mono_chunked =
    pool_resource<pool_medium_t,
                  monotonic_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_exact>;

using pool_mono_contig =
    pool_resource<pool_medium_t,
                  monotonic_release_policy,
                  contiguous_block_policy,
                  pool_growth_exact>;

using pool_gen_chunked =
    pool_resource<pool_medium_t,
                  generational_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_exact>;

using pool_gen_contig =
    pool_resource<pool_medium_t,
                  generational_release_policy,
                  contiguous_block_policy,
                  pool_growth_exact>;

using pool_fl_chunked_step =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_step4>;

using pool_fl_contig_step =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  contiguous_block_policy,
                  pool_growth_step4>;

using pool_fl_chunked_double =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  chunked_block_policy<64>,
                  pool_growth_double>;

using pool_fl_chunked_fixed =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_none>;

using pool_fl_contig_fixed =
    pool_resource<pool_medium_t,
                  free_list_release_policy,
                  contiguous_block_policy,
                  pool_growth_none>;

using pool_over_chunked =
    pool_resource<pool_overaligned_t,
                  free_list_release_policy,
                  chunked_block_policy<4>,
                  pool_growth_exact>;

using pool_over_contig =
    pool_resource<pool_overaligned_t,
                  free_list_release_policy,
                  contiguous_block_policy,
                  pool_growth_exact>;


// =========================================================================
// VII. FIXTURES - DUCK-TYPED CLASSIFICATION SUBJECTS
// =========================================================================
//   The trait layer is purely structural, so the classification tests
// need subjects that satisfy (or miss) the protocol without inheriting
// from anything.

// pool_not_a_pool
//   struct: satisfies no part of the pool protocol.
struct pool_not_a_pool
{
    int m_value;
};

// pool_duck_resource
//   struct: hand-rolled minimum pool protocol - acquire(), release(void*),
// size(), value_type.  Recognised by is_pool_resource without being a
// pool_resource.
struct pool_duck_resource
{
    using value_type = int;

    void*       acquire()            { return nullptr; }
    void        release(void*)       {}
    std::size_t size() const noexcept { return 0; }
};

// pool_duck_partial
//   struct: acquire() and release() but no size() or value_type - just
// short of the protocol.
struct pool_duck_partial
{
    void* acquire()      { return nullptr; }
    void  release(void*) {}
};

// pool_duck_stable
//   struct: carries only the pointer_stable constant.
struct pool_duck_stable
{
    static constexpr bool pointer_stable = true;
};

// pool_duck_allocator
//   struct: value_type plus resource() - the shape is_pool_allocator
// looks for.  resource() returns a real pool so the resource-type
// extraction has something to name.
struct pool_duck_allocator
{
    using value_type = pool_medium_t;

    pool_fl_chunked* resource() const noexcept { return nullptr; }
};

// pool_duck_container
//   struct: exposes allocator_type naming a pool allocator.
struct pool_duck_container
{
    using allocator_type = pool_duck_allocator;
};

// pool_duck_plain_container
//   struct: exposes allocator_type naming a non-pool allocator.
struct pool_duck_plain_container
{
    struct plain_allocator
    {
        using value_type = int;
    };

    using allocator_type = plain_allocator;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// =========================================================================
// VIII. DECLARATIONS
// =========================================================================

// I.   ENUMS                          (pool_tests_enums.cpp)
bool tests_pool_release_enumerators_are_distinct();
bool tests_pool_block_layout_enumerators_are_distinct();
bool tests_pool_enums_are_scoped_and_distinct_types();

// II.  POLICIES                       (pool_tests_policies.cpp)
bool tests_monotonic_release_policy_constants();
bool tests_free_list_release_policy_constants();
bool tests_generational_release_policy_constants();
bool tests_release_policies_are_empty_and_distinct();
bool tests_contiguous_block_policy_constants();
bool tests_chunked_block_policy_defaults();
bool tests_chunked_block_policy_parameterisation();
bool tests_block_policies_are_empty_and_distinct();

// III. GEOMETRY                       (pool_tests_geometry.cpp)
bool tests_slot_geometry_clamps_small_types();
bool tests_slot_geometry_rounds_up_to_alignment();
bool tests_slot_geometry_large_and_overaligned_types();
bool tests_slot_geometry_queries_are_static_constexpr();
bool tests_slot_geometry_is_policy_independent();
bool tests_acquired_slots_honour_slot_alignment();
bool tests_acquired_slots_do_not_overlap();

// IV.  CONSTRUCTION                   (pool_tests_construction.cpp)
bool tests_default_construction_is_empty();
bool tests_capacity_construction_preallocates();
bool tests_zero_capacity_construction_allocates_nothing();
bool tests_construction_across_the_policy_matrix();
bool tests_pool_is_non_copyable();
bool tests_pool_move_operations_are_noexcept();
bool tests_move_construction_transfers_ownership();
bool tests_move_assignment_transfers_ownership();
bool tests_self_move_assignment_is_a_noop();
bool tests_moved_from_pool_is_valid_and_reusable();
bool tests_repeated_construction_destruction_is_clean();

// V.   ACQUIRE / RELEASE              (pool_tests_acquire_release.cpp)
bool tests_acquire_returns_distinct_slots();
bool tests_acquired_slots_are_independently_writable();
bool tests_acquire_grows_on_exhaustion();
bool tests_release_null_pointer_is_ignored();
bool tests_free_list_recycles_in_lifo_order();
bool tests_free_list_is_preferred_over_the_bump_region();
bool tests_monotonic_release_does_not_recycle_storage();
bool tests_generational_release_recycles_like_free_list();
bool tests_acquire_release_cycles_do_not_grow_the_pool();
bool tests_acquire_fails_when_growth_is_refused();

// VI.  CAPACITY                       (pool_tests_capacity.cpp)
bool tests_size_capacity_and_empty_initial_state();
bool tests_size_tracks_live_slots();
bool tests_reserve_raises_capacity();
bool tests_reserve_is_a_noop_when_capacity_suffices();
bool tests_reserve_bypasses_the_growth_policy();
bool tests_reset_rewinds_without_releasing_storage();
bool tests_reset_makes_all_capacity_reusable();
bool tests_clear_releases_all_storage();
bool tests_clear_leaves_a_reusable_pool();
bool tests_reset_and_clear_are_idempotent();

// VII. GENERATIONAL                   (pool_tests_generational.cpp)
bool tests_generation_starts_at_zero();
bool tests_advance_generation_returns_the_new_value();
bool tests_generation_counter_spans_a_wide_range();
bool tests_generation_survives_reset();
bool tests_clear_rewinds_the_generation_counter();
bool tests_generation_is_carried_across_moves();
bool tests_generational_api_is_sfinae_gated();
bool tests_generational_api_signatures();

// VIII. ACCOUNTING                    (pool_tests_accounting.cpp)
bool tests_accounting_on_an_empty_pool();
bool tests_contiguous_accounting_has_no_block_overhead();
bool tests_chunked_accounting_includes_block_headers();
bool tests_bytes_in_use_tracks_live_slots();
bool tests_utilization_reports_size_over_capacity();
bool tests_utilization_stays_within_bounds();
bool tests_accounting_after_reset_and_clear();
bool tests_accounting_queries_are_const_and_noexcept();

// IX.  GROWTH                         (pool_tests_growth.cpp)
bool tests_chunked_growth_keeps_pointers_stable();
bool tests_contiguous_growth_relocates_storage();
bool tests_contiguous_growth_rebases_the_free_list();
bool tests_chunked_blocks_respect_slots_per_block();
bool tests_chunked_growth_caps_a_block_at_the_request();
bool tests_growth_policy_drives_new_capacity();
bool tests_refused_growth_bounds_the_pool();
bool tests_growth_from_zero_capacity();
bool tests_sustained_growth_yields_unique_slots();

// X.   SELECTION AND ALIASES          (pool_tests_selection.cpp)
bool tests_select_release_policy_mapping();
bool tests_select_block_policy_mapping();
bool tests_selected_policies_compose_into_a_pool();
bool tests_default_pool_resource_alias();
bool tests_monotonic_pool_resource_alias();
bool tests_flat_pool_resource_alias();
bool tests_default_aliases_are_distinct_and_operational();
bool tests_non_generational_policies_instantiate();

// XI.  TRAITS                         (pool_tests_traits.cpp)
bool tests_method_detection_traits();
bool tests_is_pool_resource_accepts_every_pool();
bool tests_is_pool_resource_rejects_non_pools();
bool tests_is_pool_resource_is_purely_structural();
bool tests_traits_ignore_cv_and_reference_qualifiers();
bool tests_pointer_stability_classification();
bool tests_individual_release_classification();
bool tests_monotonic_pool_classification();
bool tests_generational_sweep_classification();
bool tests_memory_accounting_classification();
bool tests_pool_allocator_classification();
bool tests_pool_backed_container_classification();
bool tests_pool_resource_type_extraction();
bool tests_pool_class_aggregate();
bool tests_pool_allocator_class_aggregate();

// XII. CONCEPTS                       (pool_tests_concepts.cpp)
#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
bool tests_pool_resource_concepts();
bool tests_stability_and_release_concepts();
bool tests_capability_concepts();
bool tests_allocator_and_container_concepts();
bool tests_resource_extraction_concepts();
#endif  // __cpp_concepts


// =========================================================================
// IX.  SPEC PROVIDER
// =========================================================================

#ifdef DTEST_SPEC_MODE
inline dt::module_spec
pool_spec()
{
    return dt::module_spec{
        "pool",
        "Block-structured, fixed-slot memory resource with compile-time "
        "policy selection across three orthogonal axes (release, block "
        "layout, growth), its structural classification traits, and its "
        "C++20 concept faces.",
        {
            dt::block_spec{
                "enums",
                "The two classification enums - pool_release and "
                "pool_block_layout - and their scoped-enum hygiene.",
                {
                    { "tests_pool_release_enumerators_are_distinct",
                      "the three release enumerators are pairwise distinct "
                      "and reachable in a constant expression",
                      &tests_pool_release_enumerators_are_distinct },
                    { "tests_pool_block_layout_enumerators_are_distinct",
                      "the two block-layout enumerators are distinct and "
                      "constexpr-reachable",
                      &tests_pool_block_layout_enumerators_are_distinct },
                    { "tests_pool_enums_are_scoped_and_distinct_types",
                      "both enums are scoped, non-convertible to int, and "
                      "distinct types from one another",
                      &tests_pool_enums_are_scoped_and_distinct_types },
                }
            },
            dt::block_spec{
                "policies",
                "The three release policies and the two block policies - "
                "their compile-time constants, emptiness, triviality, and "
                "pairwise distinctness.",
                {
                    { "tests_monotonic_release_policy_constants",
                      "monotonic reports its strategy and refuses both "
                      "individual release and generational sweep",
                      &tests_monotonic_release_policy_constants },
                    { "tests_free_list_release_policy_constants",
                      "free_list reports its strategy, supports individual "
                      "release, and refuses generational sweep",
                      &tests_free_list_release_policy_constants },
                    { "tests_generational_release_policy_constants",
                      "generational supports both release modes and "
                      "publishes an unsigned generation_type of at least "
                      "32 bits",
                      &tests_generational_release_policy_constants },
                    { "tests_release_policies_are_empty_and_distinct",
                      "all three release policies are empty, trivially "
                      "default-constructible, and pairwise distinct types",
                      &tests_release_policies_are_empty_and_distinct },
                    { "tests_contiguous_block_policy_constants",
                      "contiguous reports the contiguous layout and no "
                      "pointer-stability guarantee",
                      &tests_contiguous_block_policy_constants },
                    { "tests_chunked_block_policy_defaults",
                      "chunked_block_policy<> reports the chunked layout, "
                      "pointer stability, and a 256-slot default block",
                      &tests_chunked_block_policy_defaults },
                    { "tests_chunked_block_policy_parameterisation",
                      "the slot count propagates through the template and "
                      "distinct counts yield distinct types",
                      &tests_chunked_block_policy_parameterisation },
                    { "tests_block_policies_are_empty_and_distinct",
                      "both block policies are empty, trivially "
                      "default-constructible, and distinct from each other",
                      &tests_block_policies_are_empty_and_distinct },
                }
            },
            dt::block_spec{
                "geometry",
                "Slot sizing and alignment - the clamp to the free-list "
                "link, round-up to alignment, the constexpr query surface, "
                "and the alignment of storage actually handed out.",
                {
                    { "tests_slot_geometry_clamps_small_types",
                      "an element smaller than the free-list link still "
                      "yields a slot large enough to hold the link",
                      &tests_slot_geometry_clamps_small_types },
                    { "tests_slot_geometry_rounds_up_to_alignment",
                      "a size that is not a multiple of the slot alignment "
                      "is padded up to one",
                      &tests_slot_geometry_rounds_up_to_alignment },
                    { "tests_slot_geometry_large_and_overaligned_types",
                      "link-dominating and over-aligned elements drive the "
                      "slot size and alignment respectively",
                      &tests_slot_geometry_large_and_overaligned_types },
                    { "tests_slot_geometry_queries_are_static_constexpr",
                      "bytes_per_slot() and alignment() are static, "
                      "noexcept, and usable in a constant expression",
                      &tests_slot_geometry_queries_are_static_constexpr },
                    { "tests_slot_geometry_is_policy_independent",
                      "slot size and alignment depend only on the element "
                      "type, never on the release or block policy",
                      &tests_slot_geometry_is_policy_independent },
                    { "tests_acquired_slots_honour_slot_alignment",
                      "every pointer acquire() returns satisfies the "
                      "pool's declared alignment, including over-aligned "
                      "elements in both layouts",
                      &tests_acquired_slots_honour_slot_alignment },
                    { "tests_acquired_slots_do_not_overlap",
                      "concurrently held slots are pairwise at least "
                      "slot_size bytes apart",
                      &tests_acquired_slots_do_not_overlap },
                }
            },
            dt::block_spec{
                "construction",
                "Default and capacity constructors, destruction, the "
                "deleted copy operations, and move construction and "
                "assignment including self-move.",
                {
                    { "tests_default_construction_is_empty",
                      "a default-constructed pool reports zero size, zero "
                      "capacity, empty, and no allocated bytes",
                      &tests_default_construction_is_empty },
                    { "tests_capacity_construction_preallocates",
                      "the capacity constructor reserves at least the "
                      "requested slots without acquiring any",
                      &tests_capacity_construction_preallocates },
                    { "tests_zero_capacity_construction_allocates_nothing",
                      "constructing with a zero capacity request allocates "
                      "no backing storage",
                      &tests_zero_capacity_construction_allocates_nothing },
                    { "tests_construction_across_the_policy_matrix",
                      "every release/block combination constructs, "
                      "acquires, releases, and destroys cleanly",
                      &tests_construction_across_the_policy_matrix },
                    { "tests_pool_is_non_copyable",
                      "the copy constructor and copy assignment operator "
                      "are deleted",
                      &tests_pool_is_non_copyable },
                    { "tests_pool_move_operations_are_noexcept",
                      "move construction and move assignment are available "
                      "and declared noexcept",
                      &tests_pool_move_operations_are_noexcept },
                    { "tests_move_construction_transfers_ownership",
                      "the destination adopts size, capacity and storage "
                      "while the source is left empty",
                      &tests_move_construction_transfers_ownership },
                    { "tests_move_assignment_transfers_ownership",
                      "move assignment releases the destination's storage "
                      "before adopting the source's",
                      &tests_move_assignment_transfers_ownership },
                    { "tests_self_move_assignment_is_a_noop",
                      "assigning a pool to itself leaves its state intact",
                      &tests_self_move_assignment_is_a_noop },
                    { "tests_moved_from_pool_is_valid_and_reusable",
                      "a moved-from pool is empty and can be grown and "
                      "used again",
                      &tests_moved_from_pool_is_valid_and_reusable },
                    { "tests_repeated_construction_destruction_is_clean",
                      "many construct/grow/destroy cycles complete without "
                      "leaking or corrupting the allocator",
                      &tests_repeated_construction_destruction_is_clean },
                }
            },
            dt::block_spec{
                "acquire_release",
                "Slot acquisition and return across all three release "
                "policies - free-list recycling order, the monotonic "
                "no-op, the null guard, and the exhaustion path.",
                {
                    { "tests_acquire_returns_distinct_slots",
                      "successive acquires return non-null, pairwise "
                      "distinct addresses",
                      &tests_acquire_returns_distinct_slots },
                    { "tests_acquired_slots_are_independently_writable",
                      "writes through one slot never disturb another",
                      &tests_acquired_slots_are_independently_writable },
                    { "tests_acquire_grows_on_exhaustion",
                      "acquiring past capacity grows the pool and still "
                      "returns a usable slot",
                      &tests_acquire_grows_on_exhaustion },
                    { "tests_release_null_pointer_is_ignored",
                      "release(nullptr) returns without touching the live "
                      "count or the free list",
                      &tests_release_null_pointer_is_ignored },
                    { "tests_free_list_recycles_in_lifo_order",
                      "released slots come back in last-in first-out "
                      "order",
                      &tests_free_list_recycles_in_lifo_order },
                    { "tests_free_list_is_preferred_over_the_bump_region",
                      "acquire drains the free list before consuming fresh "
                      "capacity",
                      &tests_free_list_is_preferred_over_the_bump_region },
                    { "tests_monotonic_release_does_not_recycle_storage",
                      "monotonic release decrements the live count but "
                      "does not return the slot for reuse",
                      &tests_monotonic_release_does_not_recycle_storage },
                    { "tests_generational_release_recycles_like_free_list",
                      "the generational policy recycles per slot exactly "
                      "as the free-list policy does",
                      &tests_generational_release_recycles_like_free_list },
                    { "tests_acquire_release_cycles_do_not_grow_the_pool",
                      "a balanced acquire/release loop reuses one slot and "
                      "never raises capacity",
                      &tests_acquire_release_cycles_do_not_grow_the_pool },
                    { "tests_acquire_fails_when_growth_is_refused",
                      "with a can_grow==false policy, acquire returns "
                      "nullptr once capacity is exhausted",
                      &tests_acquire_fails_when_growth_is_refused },
                }
            },
            dt::block_spec{
                "capacity",
                "The size / capacity / empty queries, reserve, and the "
                "reset and clear bulk operations.",
                {
                    { "tests_size_capacity_and_empty_initial_state",
                      "a fresh pool reports size 0, capacity 0, and empty",
                      &tests_size_capacity_and_empty_initial_state },
                    { "tests_size_tracks_live_slots",
                      "size rises with each acquire and falls with each "
                      "release while capacity holds steady",
                      &tests_size_tracks_live_slots },
                    { "tests_reserve_raises_capacity",
                      "reserve grows capacity to at least the request "
                      "without acquiring anything",
                      &tests_reserve_raises_capacity },
                    { "tests_reserve_is_a_noop_when_capacity_suffices",
                      "reserving at or below the current capacity succeeds "
                      "and changes nothing",
                      &tests_reserve_is_a_noop_when_capacity_suffices },
                    { "tests_reserve_bypasses_the_growth_policy",
                      "reserve allocates even when the growth policy "
                      "refuses growth - it consults the block layout, not "
                      "the growth axis",
                      &tests_reserve_bypasses_the_growth_policy },
                    { "tests_reset_rewinds_without_releasing_storage",
                      "reset zeroes the live count and empties the free "
                      "list while preserving capacity",
                      &tests_reset_rewinds_without_releasing_storage },
                    { "tests_reset_makes_all_capacity_reusable",
                      "after reset the full capacity can be acquired again "
                      "across every block in the chain",
                      &tests_reset_makes_all_capacity_reusable },
                    { "tests_clear_releases_all_storage",
                      "clear returns the pool to its default-constructed "
                      "state with zero capacity",
                      &tests_clear_releases_all_storage },
                    { "tests_clear_leaves_a_reusable_pool",
                      "a cleared pool grows and serves slots again",
                      &tests_clear_leaves_a_reusable_pool },
                    { "tests_reset_and_clear_are_idempotent",
                      "repeated reset and clear calls, including on an "
                      "untouched pool, are safe no-ops",
                      &tests_reset_and_clear_are_idempotent },
                }
            },
            dt::block_spec{
                "generational",
                "The generation counter exposed only when the release "
                "policy supports generational sweep, and the SFINAE gating "
                "that hides it otherwise.",
                {
                    { "tests_generation_starts_at_zero",
                      "a fresh generational pool reports generation 0",
                      &tests_generation_starts_at_zero },
                    { "tests_advance_generation_returns_the_new_value",
                      "advance_generation pre-increments and the counter "
                      "is observable through current_generation",
                      &tests_advance_generation_returns_the_new_value },
                    { "tests_generation_counter_spans_a_wide_range",
                      "the counter holds values beyond 16 bits without "
                      "truncation",
                      &tests_generation_counter_spans_a_wide_range },
                    { "tests_generation_survives_reset",
                      "reset rewinds slot state but leaves the generation "
                      "counter untouched",
                      &tests_generation_survives_reset },
                    { "tests_clear_rewinds_the_generation_counter",
                      "clear reinitialises block state and so returns the "
                      "generation counter to 0",
                      &tests_clear_rewinds_the_generation_counter },
                    { "tests_generation_is_carried_across_moves",
                      "move construction and move assignment carry the "
                      "generation counter to the destination",
                      &tests_generation_is_carried_across_moves },
                    { "tests_generational_api_is_sfinae_gated",
                      "monotonic and free-list pools do not expose "
                      "current_generation or advance_generation at all",
                      &tests_generational_api_is_sfinae_gated },
                    { "tests_generational_api_signatures",
                      "both generational methods return generation_type "
                      "and are noexcept",
                      &tests_generational_api_signatures },
                }
            },
            dt::block_spec{
                "accounting",
                "Byte-level memory accounting - bytes_allocated, "
                "bytes_in_use, and utilization across both block layouts.",
                {
                    { "tests_accounting_on_an_empty_pool",
                      "an empty pool reports zero allocated, zero in use, "
                      "and 0.0 utilization",
                      &tests_accounting_on_an_empty_pool },
                    { "tests_contiguous_accounting_has_no_block_overhead",
                      "contiguous bytes_allocated is exactly capacity "
                      "times the slot size",
                      &tests_contiguous_accounting_has_no_block_overhead },
                    { "tests_chunked_accounting_includes_block_headers",
                      "chunked bytes_allocated adds one block header per "
                      "block on top of the slot bytes",
                      &tests_chunked_accounting_includes_block_headers },
                    { "tests_bytes_in_use_tracks_live_slots",
                      "bytes_in_use is always the live count times the "
                      "slot size, for every release policy",
                      &tests_bytes_in_use_tracks_live_slots },
                    { "tests_utilization_reports_size_over_capacity",
                      "utilization equals size divided by capacity, "
                      "reaching 1.0 at a full pool",
                      &tests_utilization_reports_size_over_capacity },
                    { "tests_utilization_stays_within_bounds",
                      "utilization never leaves [0.0, 1.0] and rises "
                      "monotonically through a fill cycle",
                      &tests_utilization_stays_within_bounds },
                    { "tests_accounting_after_reset_and_clear",
                      "reset preserves allocated bytes and zeroes bytes in "
                      "use; clear zeroes both",
                      &tests_accounting_after_reset_and_clear },
                    { "tests_accounting_queries_are_const_and_noexcept",
                      "all three accounting queries are const, noexcept, "
                      "and return the documented types",
                      &tests_accounting_queries_are_const_and_noexcept },
                }
            },
            dt::block_spec{
                "growth",
                "Growth on exhaustion - chunked pointer stability, "
                "contiguous relocation and free-list rebasing, block "
                "sizing, and the refused-growth boundary.",
                {
                    { "tests_chunked_growth_keeps_pointers_stable",
                      "values written through slots held across a growth "
                      "event survive it intact",
                      &tests_chunked_growth_keeps_pointers_stable },
                    { "tests_contiguous_growth_relocates_storage",
                      "growing a contiguous pool moves the backing "
                      "allocation, as its policy advertises",
                      &tests_contiguous_growth_relocates_storage },
                    { "tests_contiguous_growth_rebases_the_free_list",
                      "free-list entries are rewritten into the new "
                      "allocation at their original slot indices and are "
                      "handed back in order",
                      &tests_contiguous_growth_rebases_the_free_list },
                    { "tests_chunked_blocks_respect_slots_per_block",
                      "a bulk reserve is split into blocks of at most "
                      "slots_per_block slots",
                      &tests_chunked_blocks_respect_slots_per_block },
                    { "tests_chunked_growth_caps_a_block_at_the_request",
                      "a growth request smaller than slots_per_block "
                      "yields a correspondingly small block",
                      &tests_chunked_growth_caps_a_block_at_the_request },
                    { "tests_growth_policy_drives_new_capacity",
                      "post-growth capacity follows the growth policy's "
                      "compute(), not a rule internal to the pool",
                      &tests_growth_policy_drives_new_capacity },
                    { "tests_refused_growth_bounds_the_pool",
                      "with growth refused, capacity never exceeds what "
                      "was explicitly reserved",
                      &tests_refused_growth_bounds_the_pool },
                    { "tests_growth_from_zero_capacity",
                      "the first acquire on a zero-capacity pool "
                      "provisions storage and succeeds",
                      &tests_growth_from_zero_capacity },
                    { "tests_sustained_growth_yields_unique_slots",
                      "several thousand acquires across many growth events "
                      "yield unique, aligned, non-overlapping slots",
                      &tests_sustained_growth_yields_unique_slots },
                }
            },
            dt::block_spec{
                "selection",
                "The enum-to-policy selection metafunctions and the three "
                "convenience pool aliases.",
                {
                    { "tests_select_release_policy_mapping",
                      "each pool_release enumerator maps to its release "
                      "policy type and the three results are distinct",
                      &tests_select_release_policy_mapping },
                    { "tests_select_block_policy_mapping",
                      "each pool_block_layout enumerator maps to its block "
                      "policy, with chunked defaulting to 256 slots",
                      &tests_select_block_policy_mapping },
                    { "tests_selected_policies_compose_into_a_pool",
                      "selector-resolved policies build a working pool "
                      "whose policy aliases round-trip",
                      &tests_selected_policies_compose_into_a_pool },
                    { "tests_default_pool_resource_alias",
                      "default_pool_resource expands to free-list, "
                      "chunked, exponential growth",
                      &tests_default_pool_resource_alias },
                    { "tests_monotonic_pool_resource_alias",
                      "monotonic_pool_resource expands to monotonic, "
                      "chunked, exponential growth",
                      &tests_monotonic_pool_resource_alias },
                    { "tests_flat_pool_resource_alias",
                      "flat_pool_resource expands to free-list, "
                      "contiguous, exponential growth",
                      &tests_flat_pool_resource_alias },
                    { "tests_default_aliases_are_distinct_and_operational",
                      "the three aliases are mutually distinct types and "
                      "each instantiates into a working pool",
                      &tests_default_aliases_are_distinct_and_operational },
                    { "tests_non_generational_policies_instantiate",
                      "regression pin: pools whose release policy has no "
                      "generation_type still instantiate, so the "
                      "generation slot must be resolved lazily",
                      &tests_non_generational_policies_instantiate },
                }
            },
            dt::block_spec{
                "traits",
                "The structural classification layer - method detection, "
                "the pool and allocator predicates, resource-type "
                "extraction, and the aggregate classification structs.",
                {
                    { "tests_method_detection_traits",
                      "each has_* detector fires exactly on the types that "
                      "expose the corresponding member",
                      &tests_method_detection_traits },
                    { "tests_is_pool_resource_accepts_every_pool",
                      "every policy combination of pool_resource is "
                      "recognised as a pool resource",
                      &tests_is_pool_resource_accepts_every_pool },
                    { "tests_is_pool_resource_rejects_non_pools",
                      "plain types, partial protocols, and library types "
                      "are rejected",
                      &tests_is_pool_resource_rejects_non_pools },
                    { "tests_is_pool_resource_is_purely_structural",
                      "a hand-rolled type matching the protocol is "
                      "accepted without inheriting or tagging anything",
                      &tests_is_pool_resource_is_purely_structural },
                    { "tests_traits_ignore_cv_and_reference_qualifiers",
                      "classification is performed on the clean_t form, so "
                      "cv- and reference-qualified pools classify alike",
                      &tests_traits_ignore_cv_and_reference_qualifiers },
                    { "tests_pointer_stability_classification",
                      "chunked pools are pointer-stable and contiguous "
                      "pools are not",
                      &tests_pointer_stability_classification },
                    { "tests_individual_release_classification",
                      "free-list and generational pools support per-slot "
                      "release; monotonic pools do not",
                      &tests_individual_release_classification },
                    { "tests_monotonic_pool_classification",
                      "is_monotonic_pool holds only for pools that are "
                      "pools and lack individual release",
                      &tests_monotonic_pool_classification },
                    { "tests_generational_sweep_classification",
                      "only generational pools report generational sweep "
                      "support",
                      &tests_generational_sweep_classification },
                    { "tests_memory_accounting_classification",
                      "every pool exposes the full accounting triple and "
                      "non-pools do not",
                      &tests_memory_accounting_classification },
                    { "tests_pool_allocator_classification",
                      "is_pool_allocator fires on value_type plus "
                      "resource() and not on plain allocators",
                      &tests_pool_allocator_classification },
                    { "tests_pool_backed_container_classification",
                      "a container is pool-backed exactly when its "
                      "allocator_type is a pool allocator",
                      &tests_pool_backed_container_classification },
                    { "tests_pool_resource_type_extraction",
                      "pool_resource_type_t names the pointee of "
                      "resource() and is void when there is none",
                      &tests_pool_resource_type_extraction },
                    { "tests_pool_class_aggregate",
                      "pool_class rolls every individual pool predicate up "
                      "consistently",
                      &tests_pool_class_aggregate },
                    { "tests_pool_allocator_class_aggregate",
                      "pool_allocator_class classifies an allocator "
                      "through its extracted resource type",
                      &tests_pool_allocator_class_aggregate },
                }
            },
#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
            dt::block_spec{
                "concepts",
                "The C++20 concept face over the trait layer - each "
                "concept must be satisfied by exactly the types its "
                "underlying trait accepts.",
                {
                    { "tests_pool_resource_concepts",
                      "pool_resource_c and its companions agree with "
                      "is_pool_resource_v",
                      &tests_pool_resource_concepts },
                    { "tests_stability_and_release_concepts",
                      "the stability and release concepts partition the "
                      "pool matrix as their traits do",
                      &tests_stability_and_release_concepts },
                    { "tests_capability_concepts",
                      "the capability concepts track reset, reserve, slot "
                      "geometry, generation, and accounting support",
                      &tests_capability_concepts },
                    { "tests_allocator_and_container_concepts",
                      "the allocator and container concepts agree with "
                      "their traits on duck-typed subjects",
                      &tests_allocator_and_container_concepts },
                    { "tests_resource_extraction_concepts",
                      "the extraction concepts classify an allocator by "
                      "the properties of its resource type",
                      &tests_resource_extraction_concepts },
                }
            },
#endif  // __cpp_concepts
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CORE_MEMORY_POOL_TESTS_
