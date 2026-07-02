/******************************************************************************
* djinterp [container]                              container_storage_traits.hpp
*
*   Container-side classification on the STORAGE axis: given a container type,
* is its storage STATIC (cells inline, in-object) or DYNAMIC (cells out of
* line, allocator-backed)?  Where meta/storage.hpp owns the siting VOCABULARY
* (storage_duration and its algebra), this header is the one place that reads a
* CONTAINER's shape and reports its siting AS a storage_duration.  It is the
* spatial sibling of constexpr_container_traits.hpp / runtime_container_traits.hpp
* (the Lifetime axis): those answer WHEN a container's data are fixed, this
* answers WHERE its cells reside.
*
*   ONE module, not a static/dynamic split.  Storage is a single axis whose two
* values are complementary: a container is classified ONCE, by one shape
* analysis, and static / dynamic / hybrid fall out as the value of that one
* classification.  (Contrast the Lifetime axis, where constexpr and runtime
* containers warranted separate modules because they probe genuinely different
* surfaces - size-and-contents lifetime versus allocator-and-reserve.)
*
*   THE SIGNALS (structural, never opt-in-required):
*       static signal   has_constexpr_extent OR has_tuple_size
*                          - a compile-time-fixed inline aggregate: a member
*                            `extent` (span / djinterp arrays) or a
*                            std::tuple_size specialization (std::array, tuple,
*                            pair).  The cells are in the object's footprint.
*       dynamic signal  has_allocator_alias OR has_reserve_method_signal
*                          - an allocator_type or a reserve(n): the container
*                            draws its cells from an acquired, out-of-line
*                            region.  Per the formal model, static storage takes
*                            NO allocator, so either of these is decisive for
*                            dynamic siting.
*   The two signals feed make_storage_duration: static-only -> static_storage,
* dynamic-only -> dynamic_storage, BOTH -> hybrid_storage, NEITHER -> unknown.
*   In practice no standard container raises both signals, so shape inference
* never SYNTHESISES hybrid; small-storage (SBO) siting is not legible from a
* public surface and is declared, not detected, through the opt-in below.
*
*   PRIORITY: a type that exposes the meta opt-in
*       static constexpr djinterp::storage_duration storage_duration_category = ...;
* overrides shape inference entirely (this is how an SBO container pins
* hybrid_storage, or any type corrects a misread).
*
*   ORTHOGONAL TO LIFETIME, JOINED BY ONE ENTAILMENT:
*   Storage (where) and Lifetime (when) are independent.  std::array<std::string, 4>
* is the standing demonstration: its four std::string CELLS are inline, so its
* storage is STATIC, yet each string owns heap characters and is not a literal,
* so its container lifetime is RUNTIME - static storage paired with dynamic
* lifetime.  The cells' siting is the array's; the elements' internals are their
* own.  The single link between the axes is the formal entailment
*       site(c) = stat  =>  stage(|c|) = c
* (inline storage forces a compile-time SIZE).  This holds here STRUCTURALLY,
* not by luck: the very signals that mark static storage (extent / tuple_size)
* are compile-time-SIZE signals folded into size_lifetime, so a static-storage
* container is, by construction, compile-time-sized.  static_storage_implies_constexpr_size
* checks exactly this against meta/storage.hpp's storage_lifetime_consistent.
*
*   STORAGE IS NOT THE MEMORY STRATEGY.  This header classifies a container's
* siting only.  Whether that siting CONSTRAINS the container's memory strategy -
* the allocator discipline, and the memory layer's richer storage_kind that
* splits dynamic into runtime-fixed (`fixed_storage`) versus growable - is the
* business of the memory-strategy traits, the next layer to reconcile onto this
* axis.  This module deliberately does NOT include or redefine that layer's
* storage_kind, so it composes with memory_strategy_traits.hpp without the
* redefinition clash the old container_storage_traits.hpp incurred.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language exactly as the
* meta layer's do.
*
*
* path:      /inc/djinterp/core/container/traits/container_storage_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_STORAGE_TRAITS_
#define DJINTERP_CONTAINER_STORAGE_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                  // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"         // D_TYPE_TRAIT_* detection macros
#include "../../meta/storage.hpp"              // storage_duration, algebra, storage_of, bridge
#include "./runtime_container_traits.hpp"      // has_allocator_alias, has_reserve_method_signal,
                                               //   and (transitively) has_constexpr_extent,
                                               //   has_tuple_size, size_lifetime


NS_DJINTERP


// ===========================================================================
// I.   Storage-shape signals
// ===========================================================================
//   The two structural questions that decide a container's siting.  Each is a
// disjunction of the lower-level probes already owned by the Lifetime-axis
// trait modules - reused, not re-detected - so the two axes stay in agreement
// by sharing evidence (notably has_tuple_size, which is simultaneously the
// static-storage signal here and the compile-time-size signal in size_lifetime).

// has_static_storage_signal
//   trait: true iff _Type shows compile-time-fixed inline storage - a member
// `extent` or a std::tuple_size specialization.  Such cells sit in the
// container's own footprint and are counted at compile time.
template<typename _Type>
struct has_static_storage_signal
    : std::integral_constant<bool,
          ( has_constexpr_extent<clean_t<_Type>>::value ||
            has_tuple_size<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_static_storage_signal)

// has_dynamic_storage_signal
//   trait: true iff _Type shows out-of-line storage - an allocator_type or a
// reserve(n).  Per the formal model static storage takes no allocator, so
// either probe is decisive evidence that the cells are acquired, not inline.
template<typename _Type>
struct has_dynamic_storage_signal
    : std::integral_constant<bool,
          ( has_allocator_alias<clean_t<_Type>>::value ||
            has_reserve_method_signal<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_dynamic_storage_signal)


// ===========================================================================
// II.  The container siting classifier
// ===========================================================================

// storage_duration_of
//   trait: the storage_duration of a CONTAINER - its opt-in
// storage_duration_category if it declares one (highest priority, via the meta
// layer's storage_of), else the siting built from its two shape signals.  Both
// signals yield hybrid_storage; neither yields unknown.  Exposes the value plus
// a storage_duration_constant carrier as `::type`.
template<typename _Type>
struct storage_duration_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr storage_duration value =
        ( has_storage_duration_category<clean_type>::value
              ? storage_of<clean_type>::value
              : make_storage_duration(
                    has_static_storage_signal<clean_type>::value,
                    has_dynamic_storage_signal<clean_type>::value ) );

    using type = storage_duration_constant<value>;
};

// storage_duration_of_t
//   type: convenience alias for storage_duration_of<_Type>::type (a carrier).
template<typename _Type>
using storage_duration_of_t = typename storage_duration_of<_Type>::type;

// storage_duration_of_v
//   value: the `_v` companion (a storage_duration, emitted by hand as it is not
// a bool; same language degradation as the meta layer's storage_of_v).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr storage_duration storage_duration_of_v =
        storage_duration_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr storage_duration storage_duration_of_v =
        storage_duration_of<_Type>::value;
#endif


// ===========================================================================
// III. Container siting predicates
// ===========================================================================
//   Boolean projections of storage_duration_of, for SFINAE and requires-
// clauses.  The exclusive predicates (is_*) answer "is the storage purely
// static / dynamic / hybrid?"; the component predicates (has_*_component)
// answer "does the siting INCLUDE a static / dynamic part?" and so are the ones
// to use when hybrid storage should count on both sides.

// is_static_storage_container
//   trait: true iff the container's storage is static (inline) exclusively.
template<typename _Type>
struct is_static_storage_container
    : std::integral_constant<bool,
          is_static_storage(storage_duration_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_static_storage_container)

// is_dynamic_storage_container
//   trait: true iff the container's storage is dynamic (out of line)
// exclusively.
template<typename _Type>
struct is_dynamic_storage_container
    : std::integral_constant<bool,
          is_dynamic_storage(storage_duration_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_dynamic_storage_container)

// is_hybrid_storage_container
//   trait: true iff the container's storage spans both (small-storage / SBO);
// only ever true via the opt-in, as SBO is not legible from a public surface.
template<typename _Type>
struct is_hybrid_storage_container
    : std::integral_constant<bool,
          is_hybrid_storage(storage_duration_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hybrid_storage_container)

// has_static_storage_component_container
//   trait: true iff the siting includes a static (inline) part - static_storage
// or hybrid_storage.
template<typename _Type>
struct has_static_storage_component_container
    : std::integral_constant<bool,
          has_static_storage_component(storage_duration_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_static_storage_component_container)

// has_dynamic_storage_component_container
//   trait: true iff the siting includes a dynamic (out-of-line) part -
// dynamic_storage or hybrid_storage.
template<typename _Type>
struct has_dynamic_storage_component_container
    : std::integral_constant<bool,
          has_dynamic_storage_component(storage_duration_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_dynamic_storage_component_container)


// ===========================================================================
// IV.  Cross-axis entailment (Storage -> Lifetime)
// ===========================================================================

// static_storage_implies_constexpr_size
//   trait: the formal site(c) = stat => stage(|c|) = c, made checkable per
// type.  True iff the container's siting and its SIZE lifetime are consistent -
// i.e. the storage is not static, OR the size is compile-time-expressible.
// Because a static-storage verdict here rests on has_constexpr_extent /
// has_tuple_size, and those same signals make size_lifetime report compile-
// time, this is true BY CONSTRUCTION for every type: it is a coherence check on
// the two axes, asserting they cannot disagree, rather than a discriminating
// test.  (It is vacuously true for dynamic-storage containers, whose size may
// be either.)
template<typename _Type>
struct static_storage_implies_constexpr_size
    : std::integral_constant<bool,
          storage_lifetime_consistent(
              storage_duration_of<clean_t<_Type>>::value,
              size_lifetime<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(static_storage_implies_constexpr_size)


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

// container_storage_class
//   trait: a one-stop record of a container's position on the Storage axis -
// the two raw signals, the resolved siting and its name, the exclusive and
// component predicates, and the cross-axis coherence bit.  Useful as a single
// query point and for agent-facing summaries.
template<typename _Type>
struct container_storage_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool             has_static_signal  =
        has_static_storage_signal<clean_type>::value;
    static constexpr bool             has_dynamic_signal =
        has_dynamic_storage_signal<clean_type>::value;

    static constexpr storage_duration duration =
        storage_duration_of<clean_type>::value;
    static constexpr const char*      duration_name =
        storage_duration_name(duration);

    static constexpr bool is_static  = is_static_storage(duration);
    static constexpr bool is_dynamic = is_dynamic_storage(duration);
    static constexpr bool is_hybrid  = is_hybrid_storage(duration);
    static constexpr bool is_unknown = is_unknown_storage(duration);

    static constexpr bool has_static_component  =
        has_static_storage_component(duration);
    static constexpr bool has_dynamic_component =
        has_dynamic_storage_component(duration);

    // size-lifetime coherence: static siting must imply a compile-time size.
    static constexpr bool size_lifetime_consistent =
        static_storage_implies_constexpr_size<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_STORAGE_TRAITS_
