/******************************************************************************
* djinterp [container]                               container_memory_traits.hpp
*
*   Container-side classification on the MEMORY-DISCIPLINE axis: given a
* container type, what kind of memory does it draw upon - a general-purpose heap
* (individual), a pool, an arena, or none at all (its cells are inline)?  Where
* meta/memory_discipline.hpp owns the discipline VOCABULARY and the strategy-
* level classifier discipline_of, this header is the one place that RESOLVES
* which strategy a CONTAINER uses and reports its discipline.  It is the
* discipline sibling of container_storage_traits.hpp (siting): that asks WHERE a
* container's cells live, this asks by WHAT discipline an out-of-line region is
* governed.
*
*   RESOLUTION (the same precedence the container memory binding has always
* used, here producing a DISCIPLINE rather than a strategy object):
*       1. an explicit `using memory_strategy = S;` member  -> discipline_of<S>;
*       2. else an `allocator_type` A -> discipline_of<A> if A itself declares a
*          strategy contract (a djinterp allocator forwarding pool / arena
*          policy), otherwise INDIVIDUAL - a plain Allocator (std::allocator and
*          kin) is the general-purpose heap;
*       3. else a compile-time inline aggregate (extent / tuple_size, no
*          allocator) -> NONE: the container embeds its cells and allocates
*          nothing;
*       4. else an out-of-line shape with no visible allocator (a bare reserve())
*          -> INDIVIDUAL;
*       5. else UNKNOWN.
*   Steps 1-2 are precise; step 2's plain-Allocator default is why every standard
* allocator-aware container (vector, list, map, string) reads as individual, and
* a container reads as POOLED only when it SAYS so - through an explicit
* memory_strategy alias, or an allocator that advertises the pool discipline.
* Recognising a std-style pool allocator that does NOT self-declare its
* discipline would need the pool layer and is left to the memory-strategy
* reconciliation; it does not block the precise paths here.
*
*   PINNING the pool/heap distinction: a pool and a heap both release per object,
* so they are told apart by POINTER STABILITY (a pool's slots never move) - the
* discrimination performed in discipline_of, not by naming any concrete pool
* type.  A container therefore inherits "pooled" the moment its resolved strategy
* is pointer-stable and individually releasing.
*
*   THE STORAGE TIE-IN (does the discipline impinge on the siting?):
*   Partly, and checkably.  A NONE discipline is inline, so it must be STATIC
* storage; an INDIVIDUAL or POOLED discipline draws from an acquired region, so
* it must be DYNAMIC storage; an ARENA fixes nothing (a bump may run over inline
* or heap).  container_memory_consistent cross-checks a container's resolved
* discipline against the siting that container_storage_traits.hpp independently
* reports, via meta/memory_discipline.hpp's discipline_siting_consistent - so the
* two container-side axes cannot silently disagree.
*
*   RELATION TO THE EXISTING container memory binding: this header is the
* discipline-classifying view; the strategy-RESOLVING view (which materialises a
* strategy object and reports the memory core's storage_kind) is its companion.
* This module reads strategy contracts through the new D_TYPE_TRAIT_* detection
* family and the formal discipline vocabulary, and deliberately does NOT pull in
* the memory core's storage_kind, so it composes without the redefinition clash;
* folding the two container-memory views together, and upgrading the strategy
* core onto these detectors, is the remaining memory-reconciliation step.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language exactly as the
* meta layer's do.
*
*
* path:      /inc/djinterp/core/container/traits/container_memory_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_MEMORY_TRAITS_
#define DJINTERP_CONTAINER_MEMORY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"               // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"      // D_TYPE_TRAIT_* detection macros
#include "../../meta/memory_discipline.hpp" // memory_discipline, discipline_of, storage bridge
#include "../../meta/storage.hpp"           // storage_duration (siting consistency)
#include "./container_storage_traits.hpp"   // has_static_storage_signal, has_dynamic_storage_signal,
                                            //   storage_duration_of, and (transitively) has_allocator_alias


NS_DJINTERP


// ===========================================================================
// I.   Container strategy-alias detection
// ===========================================================================

// has_memory_strategy_alias
//   trait: detects an explicit `using memory_strategy = S;` member - the
// container's own, highest-priority declaration of how it acquires storage.
D_TYPE_TRAIT_HAS_TYPE(has_memory_strategy_alias, memory_strategy)


// ===========================================================================
// II.  Container discipline classifier
// ===========================================================================

NS_INTERNAL

    // alias_discipline
    //   trait: the discipline of a container's explicit memory_strategy alias,
    // or unknown when it declares none.  The member is read only in the `true`
    // specialization, so discipline_of is never instantiated on a missing type.
    template<typename _Container,
             bool _HasAlias = has_memory_strategy_alias<clean_t<_Container>>::value>
    struct alias_discipline
        : std::integral_constant<memory_discipline, memory_discipline::unknown>
    {};

    template<typename _Container>
    struct alias_discipline<_Container, true>
        : std::integral_constant<memory_discipline,
              discipline_of<typename clean_t<_Container>::memory_strategy>::value>
    {};

    // allocator_discipline
    //   trait: the discipline of a container's allocator_type when that
    // allocator itself declares a strategy contract, else unknown (a plain
    // Allocator advertises no discipline).  Again the member is touched only in
    // the `true` specialization.
    template<typename _Container,
             bool _HasAllocator = has_allocator_alias<clean_t<_Container>>::value>
    struct allocator_discipline
        : std::integral_constant<memory_discipline, memory_discipline::unknown>
    {};

    template<typename _Container>
    struct allocator_discipline<_Container, true>
        : std::integral_constant<memory_discipline,
              discipline_of<typename clean_t<_Container>::allocator_type>::value>
    {};

NS_END  // internal

// container_memory_discipline
//   trait: the memory_discipline a CONTAINER draws upon, by the resolution
// precedence (explicit strategy alias; else allocator - declared discipline or
// individual; else inline aggregate -> none; else bare out-of-line shape ->
// individual; else unknown).  Exposes the value plus a memory_discipline_constant
// carrier as `::type`.
template<typename _Container>
struct container_memory_discipline
{
private:
    using clean_type = clean_t<_Container>;

public:
    static constexpr memory_discipline value =
        ( has_memory_strategy_alias<clean_type>::value )
              ? internal::alias_discipline<clean_type>::value
      : ( has_allocator_alias<clean_type>::value )
              ? ( internal::allocator_discipline<clean_type>::value != memory_discipline::unknown
                      ? internal::allocator_discipline<clean_type>::value
                      : memory_discipline::individual )
      : ( has_static_storage_signal<clean_type>::value )
              ? memory_discipline::none
      : ( has_dynamic_storage_signal<clean_type>::value )
              ? memory_discipline::individual
      :         memory_discipline::unknown;

    using type = memory_discipline_constant<value>;
};

// container_memory_discipline_t
//   type: convenience alias for container_memory_discipline<_Container>::type.
template<typename _Container>
using container_memory_discipline_t =
    typename container_memory_discipline<_Container>::type;

// container_memory_discipline_v
//   value: the `_v` companion (a memory_discipline, emitted by hand; same
// language degradation as the meta layer's enum-valued _v's).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Container>
    inline constexpr memory_discipline container_memory_discipline_v =
        container_memory_discipline<_Container>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Container>
    constexpr memory_discipline container_memory_discipline_v =
        container_memory_discipline<_Container>::value;
#endif


// ===========================================================================
// III. Container discipline predicates
// ===========================================================================

// container_has_inline_storage
//   trait: true iff the container allocates nothing - its cells are embedded
// (the NONE discipline).
template<typename _Container>
struct container_has_inline_storage
    : std::integral_constant<bool,
          is_no_allocation_discipline(container_memory_discipline<_Container>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_has_inline_storage)

// container_uses_individual_heap
//   trait: true iff the container draws on a general-purpose, per-object,
// non-pointer-stable heap (the INDIVIDUAL discipline).
template<typename _Container>
struct container_uses_individual_heap
    : std::integral_constant<bool,
          is_individual_discipline(container_memory_discipline<_Container>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_uses_individual_heap)

// container_uses_pool
//   trait: true iff the container draws on a pointer-stable pool (the POOLED
// discipline).
template<typename _Container>
struct container_uses_pool
    : std::integral_constant<bool,
          is_pooled_discipline(container_memory_discipline<_Container>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_uses_pool)

// container_uses_arena
//   trait: true iff the container draws on an arena / region (the ARENA
// discipline).
template<typename _Container>
struct container_uses_arena
    : std::integral_constant<bool,
          is_arena_discipline(container_memory_discipline<_Container>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_uses_arena)

// container_allocates
//   trait: true iff the container acquires storage at runtime at all -
// individual, pooled, OR arena.
template<typename _Container>
struct container_allocates
    : std::integral_constant<bool,
          discipline_allocates(container_memory_discipline<_Container>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_allocates)

// container_memory_resolved
//   trait: true iff the discipline was determined (not unknown).
template<typename _Container>
struct container_memory_resolved
    : std::integral_constant<bool,
          ( !is_unknown_discipline(container_memory_discipline<_Container>::value) )>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_memory_resolved)


// ===========================================================================
// IV.  Cross-axis consistency (Discipline <-> Storage siting)
// ===========================================================================

// container_memory_consistent
//   trait: true iff the container's resolved DISCIPLINE and its independently
// classified SITING (container_storage_traits.hpp) agree under the bridge - a
// none discipline with static storage, an individual / pooled discipline with
// dynamic storage, or an arena (which constrains nothing).  An allocator-backed
// discipline reported over purely static storage is the contradiction this
// would catch; it holds for every well-formed container.
template<typename _Container>
struct container_memory_consistent
    : std::integral_constant<bool,
          discipline_siting_consistent(
              container_memory_discipline<clean_t<_Container>>::value,
              storage_duration_of<clean_t<_Container>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(container_memory_consistent)


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

// container_memory_class
//   trait: a one-stop record of a container's position on the memory axis - the
// resolved discipline and its name, the discipline predicates, the siting it is
// paired with (from container_storage_traits.hpp), and the cross-axis
// consistency bit.  Useful as a single query point and for agent-facing
// summaries.
template<typename _Container>
struct container_memory_class
{
private:
    using clean_type = clean_t<_Container>;

public:
    static constexpr memory_discipline discipline =
        container_memory_discipline<clean_type>::value;
    static constexpr const char*       discipline_name =
        memory_discipline_name(discipline);

    static constexpr bool is_none       = is_no_allocation_discipline(discipline);
    static constexpr bool is_individual = is_individual_discipline(discipline);
    static constexpr bool is_pooled     = is_pooled_discipline(discipline);
    static constexpr bool is_arena      = is_arena_discipline(discipline);
    static constexpr bool allocates     = discipline_allocates(discipline);
    static constexpr bool resolved      = !is_unknown_discipline(discipline);

    // the siting this discipline is paired with, and their coherence.
    static constexpr storage_duration siting =
        storage_duration_of<clean_type>::value;
    static constexpr const char*      siting_name =
        storage_duration_name(siting);
    static constexpr bool             siting_consistent =
        discipline_siting_consistent(discipline, siting);
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_MEMORY_TRAITS_
