/******************************************************************************
* djinterp [meta]                                        memory_discipline.hpp
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
* path:      /inc/djinterp/core/meta/memory_discipline.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_META_MEMORY_DISCIPLINE_
#define DJINTERP_META_MEMORY_DISCIPLINE_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"      // clean_t, void_t, NS_*, D_ENV_* feature macros
#include "./trait_detect.hpp"   // D_VOID_T, D_TYPE_TRAIT_* detection macros
#include "./storage.hpp"        // storage_duration, components (storage bridge)


NS_DJINTERP


// ===========================================================================
// I.   The memory-discipline taxonomy
// ===========================================================================

// memory_discipline
//   enum: the acquisition-and-release discipline a strategy governs storage by.
// The values are MUTUALLY EXCLUSIVE - a strategy is classified once - unlike
// the storage and lifetime lattices, since a discipline is a single verdict on
// the release pattern, not a set of independent facts.
enum class memory_discipline
{
    unknown,        // not a recognisable strategy
    none,           // allocates nothing: the container embeds its cells (inline / descriptive)
    individual,     // per-object alloc & free, any order, NOT pointer-stable (general heap)
    pooled,         // per-object alloc & free, POINTER-STABLE fixed slots (pool: individual refinement)
    arena           // cohort reclaimed together; no per-object free (region / bump)
};


// ===========================================================================
// II.  Discipline algebra
// ===========================================================================
//   Plain `constexpr` predicates and a name, usable in constant expressions
// (including the static initializers of the traits below) regardless of the
// constexpr-instrumentation toggle.

// is_no_allocation_discipline
//   function: true iff the strategy allocates nothing (descriptive / inline).
constexpr bool
is_no_allocation_discipline(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::none );
}

// is_individual_discipline
//   function: true iff the discipline is the plain (NOT pointer-stable) heap.
constexpr bool
is_individual_discipline(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::individual );
}

// is_pooled_discipline
//   function: true iff the discipline is a pointer-stable pool.
constexpr bool
is_pooled_discipline(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::pooled );
}

// is_arena_discipline
//   function: true iff the discipline is an arena / region (cohort release).
constexpr bool
is_arena_discipline(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::arena );
}

// is_unknown_discipline
//   function: true iff the discipline could not be determined.
constexpr bool
is_unknown_discipline(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::unknown );
}

// is_individual_family_discipline
//   function: true iff the discipline releases PER OBJECT - individual OR
// pooled.  Use this when "frees one object at a time" matters and pointer
// stability does not.
constexpr bool
is_individual_family_discipline(memory_discipline _d) noexcept
{
    return ( ( _d == memory_discipline::individual ) ||
             ( _d == memory_discipline::pooled ) );
}

// discipline_allocates
//   function: true iff the discipline actually acquires storage at runtime -
// individual, pooled, OR arena (i.e. everything but none / unknown).
constexpr bool
discipline_allocates(memory_discipline _d) noexcept
{
    return ( ( _d == memory_discipline::individual ) ||
             ( _d == memory_discipline::pooled )     ||
             ( _d == memory_discipline::arena ) );
}

// memory_discipline_name
//   function: a stable human-readable spelling, for diagnostics and agent-
// facing summaries.
constexpr const char*
memory_discipline_name(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::none       ? "none"
           : _d == memory_discipline::individual ? "individual"
           : _d == memory_discipline::pooled     ? "pooled"
           : _d == memory_discipline::arena      ? "arena"
           :                                       "unknown" );
}


// ===========================================================================
// III. Type-level carrier
// ===========================================================================

// memory_discipline_constant
//   type: an integral_constant specialized to a memory_discipline value.
template<memory_discipline _Discipline>
using memory_discipline_constant =
    std::integral_constant<memory_discipline, _Discipline>;

// unknown_discipline / none_discipline / ...
//   type: named carriers for the five values, for tag dispatch.
using unknown_discipline    = memory_discipline_constant<memory_discipline::unknown>;
using none_discipline       = memory_discipline_constant<memory_discipline::none>;
using individual_discipline = memory_discipline_constant<memory_discipline::individual>;
using pooled_discipline     = memory_discipline_constant<memory_discipline::pooled>;
using arena_discipline      = memory_discipline_constant<memory_discipline::arena>;


// ===========================================================================
// IV.  Cross-axis bridge to Storage
// ===========================================================================
//   The discipline partly fixes the siting (meta/storage.hpp).  These value-
// level helpers state which; the per-type check lives with the container memory
// traits (it needs a container's resolved siting).

// discipline_implies_siting
//   function: the storage_duration a discipline forces, or unknown when it
// forces none.  none -> static (inline, no allocator); individual / pooled ->
// dynamic (an acquired region; static storage takes no allocator); arena ->
// unknown (a bump may run over inline OR heap storage).
constexpr storage_duration
discipline_implies_siting(memory_discipline _d) noexcept
{
    return ( _d == memory_discipline::none       ? storage_duration::static_storage
           : _d == memory_discipline::individual ? storage_duration::dynamic_storage
           : _d == memory_discipline::pooled     ? storage_duration::dynamic_storage
           :                                       storage_duration::unknown );
}

// discipline_siting_consistent
//   function: tests a (discipline, siting) pair against the bridge - holds iff
// the discipline forces no siting, OR the siting is undetermined, OR the actual
// siting carries the component the discipline forces.  An individual / pooled
// discipline paired with purely static storage (an allocator over inline cells)
// is the contradiction this catches.
constexpr bool
discipline_siting_consistent(
    memory_discipline _d,
    storage_duration  _s
) noexcept
{
    return ( ( discipline_implies_siting(_d) == storage_duration::unknown ) ||
             ( is_unknown_storage(_s) )                                     ||
             ( discipline_implies_siting(_d) == storage_duration::static_storage
                   ? has_static_storage_component(_s)
                   : has_dynamic_storage_component(_s) ) );
}


// ===========================================================================
// V.   Strategy-contract detection
// ===========================================================================
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
    decltype(std::declval<clean_t<_Type>&>().allocate(
        std::declval<std::size_t>())))

// has_byte_allocate_signal
//   trait: detects a byte-typed allocate(size_t, size_t) surface (pmr-shaped).
D_TYPE_TRAIT_TRUE(has_byte_allocate_signal,
    decltype(std::declval<clean_t<_Type>&>().allocate(
        std::declval<std::size_t>(),
        std::declval<std::size_t>())))

// has_pointer_stable_constant_signal / has_individual_release_constant_signal
//   trait: detect the optional `pointer_stable` / `supports_individual_release`
// constants.
D_TYPE_TRAIT_HAS_STATIC_MEMBER(has_pointer_stable_constant_signal, pointer_stable)
D_TYPE_TRAIT_HAS_STATIC_MEMBER(has_individual_release_constant_signal,
                               supports_individual_release)

NS_INTERNAL

    // read_pointer_stable
    //   trait: yields clean_t<_Type>::pointer_stable, or the conservative
    // default false when the constant is absent.
    template<typename _Type,
             typename = void>
    struct read_pointer_stable
    {
        static constexpr bool value = false;
    };

    template<typename _Type>
    struct read_pointer_stable<_Type,
        D_VOID_T<decltype(clean_t<_Type>::pointer_stable)>>
    {
        static constexpr bool value = clean_t<_Type>::pointer_stable;
    };

    // read_individual_release
    //   trait: yields clean_t<_Type>::supports_individual_release, or the
    // conservative default false (absent -> monotonic / arena) when absent.
    template<typename _Type,
             typename = void>
    struct read_individual_release
    {
        static constexpr bool value = false;
    };

    template<typename _Type>
    struct read_individual_release<_Type,
        D_VOID_T<decltype(clean_t<_Type>::supports_individual_release)>>
    {
        static constexpr bool value = clean_t<_Type>::supports_individual_release;
    };

NS_END  // internal


// ===========================================================================
// VI.  Strategy-level classifier
// ===========================================================================

// discipline_of
//   trait: the memory_discipline of a STRATEGY type, read from its contract.
// A type that does not declare strategy_storage_kind is not a recognisable
// strategy (unknown); one that declares it but exposes no allocate() surface is
// descriptive-only (none); otherwise the release pattern (individual-release,
// pointer-stable) selects arena / pooled / individual.  Exposes the value plus
// a memory_discipline_constant carrier as `::type`.
template<typename _Type>
struct discipline_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr memory_discipline value =
        ( !has_strategy_kind_signal<clean_type>::value )
              ? memory_discipline::unknown
      : ( !( has_element_allocate_signal<clean_type>::value ||
             has_byte_allocate_signal<clean_type>::value ) )
              ? memory_discipline::none
      : ( !internal::read_individual_release<clean_type>::value )
              ? memory_discipline::arena
      : (  internal::read_pointer_stable<clean_type>::value )
              ? memory_discipline::pooled
      :       memory_discipline::individual;

    using type = memory_discipline_constant<value>;
};

// discipline_of_t
//   type: convenience alias for discipline_of<_Type>::type (a carrier).
template<typename _Type>
using discipline_of_t = typename discipline_of<_Type>::type;

// discipline_of_v
//   value: the `_v` companion (a memory_discipline, emitted by hand as it is
// not a bool; same language degradation as the meta layer's enum-valued _v's).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr memory_discipline discipline_of_v = discipline_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr memory_discipline discipline_of_v = discipline_of<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_META_MEMORY_DISCIPLINE_
