/*******************************************************************************
* djinterp [testing]                           container_memory_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_memory_traits.hpp unit-test suite.  The section translation units
* (container_memory_traits_tests_*.cpp) DEFINE the tests_* functions declared
* here; the runner (in the config tree) calls them.  Every test function is
* flat in djinterp::testing and returns true on success.
*
*   MECHANISM.  The memory-discipline traits are compile-time facts, so each
* check compares a constant trait reading against its expected result.  A
* failing check prints the offending expression and drives the function's bool
* result to false, which the runner reports and turns into a non-zero exit code
* - the runtime contract the DTest runner is built around.
*
*   HELPER TYPES.  container_memory_discipline resolves a container's strategy by
* a five-step precedence (explicit memory_strategy alias; else allocator_type -
* its own declared discipline, or the plain-allocator INDIVIDUAL default; else an
* inline aggregate -> NONE; else a bare out-of-line shape -> INDIVIDUAL; else
* UNKNOWN).  No standard container reaches the pool / arena / declared-strategy
* branches, so two families of custom types are supplied:
*
*   STRATEGY types (fed to discipline_of via a container's alias/allocator):
*     - pool_strategy         kind + allocate + individual-release + pointer-stable  -> pooled
*     - arena_strategy        kind + allocate (no individual-release)                -> arena
*     - heap_strategy         kind + allocate + individual-release (not stable)      -> individual
*     - descriptive_strategy  kind, but NO allocate surface                          -> none
*     - not_a_strategy        no strategy_storage_kind at all                        -> unknown
*
*   CONTAINER types (each isolates a resolution branch or precedence edge):
*     - pooled_via_strategy            memory_strategy = pool_strategy   (step 1 -> pooled)
*     - pooled_via_allocator           allocator_type  = pool_strategy   (step 2 declared -> pooled)
*     - arena_via_strategy             memory_strategy = arena_strategy  (step 1 -> arena)
*     - descriptive_via_strategy       memory_strategy = descriptive_strategy (step 1 -> none)
*     - alias_unknown_container        memory_strategy = not_a_strategy, PLUS an
*                                      allocator_type: the alias wins (step 1 -> unknown),
*                                      proving alias precedence over the allocator
*     - reserve_only                   a reserve() but NO allocator_type  (step 4 -> individual)
*     - extent_aggregate               a static extent member             (step 3 -> none)
*     - alloc_beats_static             allocator_type AND a static extent: the
*                                      allocator wins (step 2 -> individual), and the
*                                      siting is hybrid_storage (both signals)
*     - inconsistent_individual_static memory_strategy = heap_strategy AND a static
*                                      extent: discipline individual but siting
*                                      static_storage - the ONE contradiction
*                                      container_memory_consistent must flag
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_MEMORY_TRAITS_TESTS_
#define DJINTERP_TESTING_CONTAINER_MEMORY_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
// djinterp
#include <djinterp/djinterp.hpp>
#include <djinterp/core/container/buffer/byte_buffer.hpp>
#include <djinterp/core/container/traits/container_memory_traits.hpp>
#include <djinterp/test/test_defaults.hpp>


// D_CM_CHECK
//   macro: the per-suite runtime check (unique letters: CM = Container Memory).
// Records a failure into the enclosing function's `_cm_ok` accumulator and
// prints the offending expression.  Each `_cond` is a compile-time-constant
// trait reading, so the check is deterministic.  Always pass the condition
// already wrapped in parentheses so interior template commas (map<int,int>,
// array<int,N>, is_same<A,B>) survive macro argument splitting.
#define D_CM_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _cm_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// cm_cstr_eq
//   helper: constexpr C-string equality, so the *_name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).
constexpr bool
cm_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : cm_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Strategy helper types (fed to discipline_of through a container)
// ===========================================================================
//   Each advertises the strategy contract discipline_of reads: the
// strategy_storage_kind marker, an optional allocate() surface, and the optional
// pointer_stable / supports_individual_release constants.

// pool_strategy -> pooled (allocates, per-object release, pointer-stable)
struct pool_strategy
{
    static constexpr int  strategy_storage_kind        = 0;
    static constexpr bool supports_individual_release  = true;
    static constexpr bool pointer_stable               = true;
    void* allocate(std::size_t) { return nullptr; }
};

// arena_strategy -> arena (allocates, but no individual release)
struct arena_strategy
{
    static constexpr int  strategy_storage_kind = 0;
    void* allocate(std::size_t) { return nullptr; }
    // supports_individual_release absent -> false -> arena
};

// heap_strategy -> individual (allocates, per-object release, NOT pointer-stable)
struct heap_strategy
{
    static constexpr int  strategy_storage_kind       = 0;
    static constexpr bool supports_individual_release = true;
    void* allocate(std::size_t) { return nullptr; }
    // pointer_stable absent -> false -> individual
};

// descriptive_strategy -> none (a strategy that allocates nothing)
struct descriptive_strategy
{
    static constexpr int strategy_storage_kind = 0;
    // no allocate() surface -> none
};

// not_a_strategy -> unknown (no strategy_storage_kind marker at all)
struct not_a_strategy
{
};


// ===========================================================================
// Container helper types (each isolates one resolution branch / edge)
// ===========================================================================

// step 1 (explicit alias) -> pooled
struct pooled_via_strategy
{
    using value_type      = int;
    using memory_strategy = pool_strategy;
};

// step 2 (allocator with a DECLARED discipline) -> pooled
struct pooled_via_allocator
{
    using value_type     = int;
    using allocator_type = pool_strategy;
};

// step 1 -> arena
struct arena_via_strategy
{
    using value_type      = int;
    using memory_strategy = arena_strategy;
};

// step 1 (discipline_of -> none) -> none
struct descriptive_via_strategy
{
    using value_type      = int;
    using memory_strategy = descriptive_strategy;
};

// step 1 precedence: the alias resolves to unknown and WINS over the allocator
// (which alone would give individual), proving the alias is consulted first.
struct alias_unknown_container
{
    using value_type      = int;
    using memory_strategy = not_a_strategy;
    using allocator_type  = std::allocator<int>;
};

// step 4 (bare out-of-line shape: a reserve(), no allocator_type) -> individual
struct reserve_only
{
    using value_type = int;
    void reserve(std::size_t) {}
};

// step 3 (inline aggregate via a static extent, no allocator) -> none
struct extent_aggregate
{
    using value_type = int;
    static constexpr std::size_t extent = 8;
};

// step 2 precedence: an allocator_type present alongside a static extent; the
// allocator wins (-> individual), and BOTH storage signals fire (-> hybrid siting).
struct alloc_beats_static
{
    using value_type     = int;
    using allocator_type = std::allocator<int>;
    static constexpr std::size_t extent = 4;
};

// the deliberate contradiction: an individual-heap strategy (implies DYNAMIC
// siting) declared on a type whose only storage signal is a static extent
// (-> static_storage).  container_memory_consistent must report false.
struct inconsistent_individual_static
{
    using value_type      = int;
    using memory_strategy = heap_strategy;
    static constexpr std::size_t extent = 4;
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Strategy-alias detection --
bool tests_has_memory_strategy_alias();

// -- II.  Discipline classifier --
bool tests_container_memory_discipline();
bool tests_container_memory_discipline_companions();

// -- III. Discipline predicates --
bool tests_container_has_inline_storage();
bool tests_container_uses_individual_heap();
bool tests_container_uses_pool();
bool tests_container_uses_arena();
bool tests_container_allocates();
bool tests_container_memory_resolved();

// -- IV.  Cross-axis consistency --
bool tests_container_memory_consistent();

// -- V.   Aggregate snapshot + variable-template companions --
bool tests_container_memory_class();
bool tests_memory_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_CONTAINER_MEMORY_TRAITS_TESTS_
