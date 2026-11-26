/*******************************************************************************
* djinterp [cpp][meta]                                     memory_discipline.hpp
*
*   The framework's foundational MEMORY-DISCIPLINE vocabulary - the answer to
* "what kind of memory does a strategy use?": a general-purpose heap, a pool, an
* arena, or none at all.  Where meta/storage.hpp classifies WHERE a container's
* cells are sited (static vs dynamic), this header classifies HOW an out-of-line
* region is governed - the acquisition-and-release DISCIPLINE.  The two are
* distinct: storage asks inline-or-out-of-line; discipline asks, of the out-of-
* line case, by what pattern blocks are handed out and reclaimed.
*
*   THE MODEL (the formal Memory-Strategies taxonomy, plus the pool refinement):
*   A memory strategy is the discipline governing acquisition and release.  The
* two formal poles are
*       individual (normal)  each object is allocated and freed on its OWN, in
*                            any interleaving - the general-purpose heap;
*       arena      (region)  many objects are carved from ONE region and
*                            reclaimed TOGETHER by a single deallocation
*                            (monotone bump, or scoped/LIFO release).
*   A POOL is the pointer-stable refinement of the individual discipline: it
* still releases per object, but vends fixed slots from a pre-reserved region
* so that addresses never move.  That pointer-stability is exactly what
* distinguishes a pool from a plain heap.  Finally a DESCRIPTIVE-ONLY strategy
* allocates nothing - the container embeds its cells and fills them directly.
* Encoding the verdict as one value:
*       memory_discipline::none         allocates nothing  (inline / descriptive);
*       memory_discipline::individual   per-object alloc & free, NOT stable (heap);
*       memory_discipline::pooled       per-object alloc & free, POINTER-STABLE (pool);
*       memory_discipline::arena        cohort reclaimed together (bump / region);
*       memory_discipline::unknown      not a recognisable strategy.
*
*   READ FROM THE DISCIPLINE'S OWN CONTRACT:
*   A strategy advertises three facts this axis needs - whether it ALLOCATES (an
* allocate() surface), whether it supports INDIVIDUAL RELEASE, and whether it is
* POINTER-STABLE.  From those three the discipline follows uniquely:
*       allocates?  individual-release?  pointer-stable?   ->  discipline
*          no              -                  -                  none
*          yes             no                 -                  arena
*          yes             yes                no                 individual
*          yes             yes                yes                pooled
*   No knowledge of any concrete provider (pool internals, allocator internals)
* is needed; the discipline is defined by its release PATTERN, not its type.
*
*   THE BRIDGE TO STORAGE:
*   The discipline partly determines the siting of meta/storage.hpp.  A
* descriptive-only strategy is inline, so it implies STATIC storage; an
* individual or pooled discipline draws from an acquired region, so each implies
* DYNAMIC storage (a heap or pool takes an allocator, and static storage takes
* none).  An ARENA is the one discipline that does NOT fix the siting - a bump
* allocator may run over an inline buffer (static) or a heap buffer (dynamic) -
* so it implies no siting.  discipline_implies_siting / discipline_siting_consistent
* state this at the value level.
*
*   STRATEGY-AGNOSTIC:
*   This header owns the discipline vocabulary, its algebra, a carrier, the
* contract-reading detection, the strategy-level classifier discipline_of, and
* the storage bridge.  It performs NO container resolution: deciding which
* strategy a CONTAINER draws upon (an explicit alias, an allocator_type, an
* inline extent) is container-domain and lives with the container memory traits,
* which feed their resolved strategy into discipline_of.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language exactly as the
* rest of the meta layer's do.
*
*
* path:      /inc/djinterp/cpp/meta/memory_discipline.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  The memory-discipline taxonomy
    ------------------------------
2.  Discipline algebra
    ------------------
3.  Type-level carrier
    ------------------
4.  Cross-axis bridge to Storage
    ----------------------------
5.  Strategy-contract detection
    ---------------------------
6.  Strategy-level classifier
    -------------------------
*/

#ifndef DJINTERP_META_MEMORY_DISCIPLINE_HPP
#define DJINTERP_META_MEMORY_DISCIPLINE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"      // framework root
#include "./type_utility.hpp"      // clean_t
#include "./trait_detect.hpp"   // D_VOID_T, D_TYPE_TRAIT_* detection macros
#include "./storage.hpp"        // storage_duration, components (storage bridge)


NS_DJINTERP


//==============================================================================
// 1.  The memory-discipline taxonomy
//==============================================================================

// DMemoryDiscipline
//   enum: the acquisition-and-release discipline a strategy governs storage by.
// The values are MUTUALLY EXCLUSIVE - a strategy is classified once - unlike
// the storage and lifetime lattices, since a discipline is a single verdict on
// the release pattern, not a set of independent facts.
struct DMemoryDiscipline
{
    enum value
    {
        unknown,     // not a recognisable strategy
        none,        // allocates nothing: the container embeds its cells (inline / descriptive)
        individual,  // per-object alloc & free, any order, NOT pointer-stable (general heap)
        pooled,      // per-object alloc & free, POINTER-STABLE fixed slots (pool: individual refinement)
        arena        // cohort reclaimed together; no per-object free (region / bump)
    };
};

// old name, until 0.9.0-rc.1
using memory_discipline = DMemoryDiscipline;


//==============================================================================
// 2.  Discipline algebra
//==============================================================================
//   Plain `constexpr` predicates and a name, usable in constant expressions
// (including the static initializers of the traits below) regardless of the
// constexpr-instrumentation toggle.

// is_no_allocation_discipline
//   function: true iff the strategy allocates nothing (descriptive / inline).
constexpr
bool is_no_allocation_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::none );
}

// is_individual_discipline
//   function: true iff the discipline is the plain (NOT pointer-stable) heap.
constexpr
bool is_individual_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::individual );
}

// is_pooled_discipline
//   function: true iff the discipline is a pointer-stable pool.
constexpr
bool is_pooled_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::pooled );
}

// is_arena_discipline
//   function: true iff the discipline is an arena / region (cohort release).
constexpr
bool is_arena_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::arena );
}

// is_unknown_discipline
//   function: true iff the discipline could not be determined.
constexpr
bool is_unknown_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::unknown );
}

// is_individual_family_discipline
//   function: true iff the discipline releases PER OBJECT - individual OR
// pooled.  Use this when "frees one object at a time" matters and pointer
// stability does not.
constexpr
bool is_individual_family_discipline(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( ( _d == DMemoryDiscipline::individual ) ||
             ( _d == DMemoryDiscipline::pooled ) );
}

// discipline_allocates
//   function: true iff the discipline actually acquires storage at runtime -
// individual, pooled, OR arena (i.e. everything but none / unknown).
constexpr
bool discipline_allocates(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( ( _d == DMemoryDiscipline::individual ) ||
             ( _d == DMemoryDiscipline::pooled )     ||
             ( _d == DMemoryDiscipline::arena ) );
}

// memory_discipline_name
//   function: a stable human-readable spelling, for diagnostics and agent-
// facing summaries.
constexpr
const char* memory_discipline_name(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::none       ? "none"
           : _d == DMemoryDiscipline::individual ? "individual"
           : _d == DMemoryDiscipline::pooled     ? "pooled"
           : _d == DMemoryDiscipline::arena      ? "arena"
           :                                       "unknown" );
}


//==============================================================================
// 3.  Type-level carrier
//==============================================================================

// memory_discipline_constant
//   type: an integral_constant specialized to a memory_discipline value.
template<DMemoryDiscipline::value Discipline>
using memory_discipline_constant =
    std::integral_constant<DMemoryDiscipline::value, Discipline>;

// unknown_discipline / none_discipline / ...
//   type: named carriers for the five values, for tag dispatch.
using unknown_discipline    = memory_discipline_constant<DMemoryDiscipline::unknown>;
using none_discipline       = memory_discipline_constant<DMemoryDiscipline::none>;
using individual_discipline = memory_discipline_constant<DMemoryDiscipline::individual>;
using pooled_discipline     = memory_discipline_constant<DMemoryDiscipline::pooled>;
using arena_discipline      = memory_discipline_constant<DMemoryDiscipline::arena>;


//==============================================================================
// 4.  Cross-axis bridge to Storage
//==============================================================================
//   The discipline partly fixes the siting (meta/storage.hpp).  These value-
// level helpers state which; the per-type check lives with the container memory
// traits (it needs a container's resolved siting).

// discipline_implies_siting
//   function: the storage_duration a discipline forces, or unknown when it
// forces none.  none -> static (inline, no allocator); individual / pooled ->
// dynamic (an acquired region; static storage takes no allocator); arena ->
// unknown (a bump may run over inline OR heap storage).
constexpr
DStorageDuration::value discipline_implies_siting(
    DMemoryDiscipline::value _d
) noexcept
{
    return ( _d == DMemoryDiscipline::none       ? DStorageDuration::static_storage
           : _d == DMemoryDiscipline::individual ? DStorageDuration::dynamic_storage
           : _d == DMemoryDiscipline::pooled     ? DStorageDuration::dynamic_storage
           :                                       DStorageDuration::unknown );
}

// discipline_siting_consistent
//   function: tests a (discipline, siting) pair against the bridge - holds iff
// the discipline forces no siting, OR the siting is undetermined, OR the actual
// siting carries the component the discipline forces.  An individual / pooled
// discipline paired with purely static storage (an allocator over inline cells)
// is the contradiction this catches.
constexpr
bool discipline_siting_consistent(
    DMemoryDiscipline::value _d,
    DStorageDuration::value  _s
) noexcept
{
    return ( ( discipline_implies_siting(_d) == DStorageDuration::unknown ) ||
             ( is_unknown_storage(_s) )                                     ||
             ( discipline_implies_siting(_d) == DStorageDuration::static_storage
                   ? has_static_storage_component(_s)
                   : has_dynamic_storage_component(_s) ) );
}


//==============================================================================
// 5.  Strategy-contract detection
//==============================================================================
//   The three facts the discipline is read from, each detected (and, for the
// booleans, read) from the strategy's own declared surface.  cv-ref is stripped
// via clean_t so the answer agrees for S, const S, S&.

// has_strategy_kind_signal
//   trait: detects the `strategy_storage_kind` constant - the marker that a
// type IS a memory strategy (it advertises a storage discipline).  Named with a
// _signal suffix to stay distinct from the memory core's has_strategy_kind_constant,
// so the two may co-exist in one translation unit during reconciliation.
D_TYPE_TRAIT_HAS_STATIC_MEMBER(has_strategy_kind_signal, strategy_storage_kind)

// has_element_allocate_signal
//   trait: detects an element-typed allocate(size_t) surface.
D_TYPE_TRAIT_TRUE(has_element_allocate_signal,
    decltype(std::declval<clean_t<Type>&>().allocate(
        std::declval<std::size_t>())))

// has_byte_allocate_signal
//   trait: detects a byte-typed allocate(size_t, size_t) surface (pmr-shaped).
D_TYPE_TRAIT_TRUE(has_byte_allocate_signal,
    decltype(std::declval<clean_t<Type>&>().allocate(
        std::declval<std::size_t>(),
        std::declval<std::size_t>())))

// has_pointer_stable_constant_signal / has_individual_release_constant_signal
//   trait: detect the optional `pointer_stable` / `supports_individual_release`
// constants: true exactly where the readers below read a value. A static
// member function or a non-static data member of either name declares
// nothing. (Until 2026.11.11 a function was detected here and read below as
// true, so a strategy with two such functions classified as pooled, and a
// data member was a hard error in the readers.)
D_TYPE_TRAIT_HAS_BOOL_CONSTANT(has_pointer_stable_constant_signal,
                               pointer_stable)
D_TYPE_TRAIT_HAS_BOOL_CONSTANT(has_individual_release_constant_signal,
                               supports_individual_release)

NS_INTERNAL

    // read_pointer_stable
    //   trait: yields clean_t<Type>::pointer_stable, or the conservative
    // default false where the type declares no such constant
    // (D_TYPE_TRAIT_OPT_IN).
    D_TYPE_TRAIT_OPT_IN(read_pointer_stable, pointer_stable)

    // read_individual_release
    //   trait: yields clean_t<Type>::supports_individual_release, or the
    // conservative default false (absent -> monotonic / arena).
    D_TYPE_TRAIT_OPT_IN(read_individual_release, supports_individual_release)

NS_END  // internal


//==============================================================================
// 6.  Strategy-level classifier
//==============================================================================

// discipline_of
//   trait: the memory_discipline of a STRATEGY type, read from its contract.
// A type that does not declare strategy_storage_kind is not a recognisable
// strategy (unknown); one that declares it but exposes no allocate() surface is
// descriptive-only (none); otherwise the release pattern (individual-release,
// pointer-stable) selects arena / pooled / individual.  Exposes the value plus
// a memory_discipline_constant carrier as `::type`.
template<typename Type>
struct discipline_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr DMemoryDiscipline::value value =
        ( !has_strategy_kind_signal<clean_type>::value )
              ? DMemoryDiscipline::unknown
      : ( !( has_element_allocate_signal<clean_type>::value ||
             has_byte_allocate_signal<clean_type>::value ) )
              ? DMemoryDiscipline::none
      : ( !internal::read_individual_release<clean_type>::value )
              ? DMemoryDiscipline::arena
      : (  internal::read_pointer_stable<clean_type>::value )
              ? DMemoryDiscipline::pooled
      :       DMemoryDiscipline::individual;

    using type = memory_discipline_constant<value>;
};

// discipline_of_t
//   type: convenience alias for discipline_of<Type>::type (a carrier).
template<typename Type>
using discipline_of_t = typename discipline_of<Type>::type;

// discipline_of_v
//   value: the `_v` companion (a memory_discipline, emitted by hand as it is
// not a bool; same language degradation as the meta layer's enum-valued _v's).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr DMemoryDiscipline::value discipline_of_v =
        discipline_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr DMemoryDiscipline::value discipline_of_v =
        discipline_of<Type>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_MEMORY_DISCIPLINE_HPP
