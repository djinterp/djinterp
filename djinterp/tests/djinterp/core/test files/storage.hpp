/******************************************************************************
* djinterp [meta]                                                  storage.hpp
*
*   The framework's foundational STORAGE vocabulary - the static / dynamic
* classification of WHERE a container sites its cells, factored out so every
* subsystem draws its "inline, or out of line?" answer from one place.  This is
* the C++ embodiment of the Storage axis: the siting of a container's cells,
* and the lattice those sitings form.  It is the spatial complement of
* meta/lifetime.hpp (WHEN data are fixed) - storage answers WHERE they reside.
*
*   THE MODEL (two sitings, aligned with the two Lifetime stages):
*   A container's cells are sited in one of two ways:
*       static  (stat)   reserved INLINE, within the container object's own
*                        footprint, fixed before execution, never separately
*                        acquired or released - an in-object array;
*       dynamic (dyn)    sited OUT OF LINE, in a region acquired during
*                        execution through an allocator and named by a handle -
*                        a heap-backed vector, list, or map.
*   The two combine: a SMALL-STORAGE (SBO) container reserves an inline buffer
* for up to k cells and SPILLS to dynamic storage beyond k, and so is BOTH.
* Encoding one bit per siting, a container's storage is one of:
*       storage_duration::static_storage    inline only           (stat);
*       storage_duration::dynamic_storage   out of line only      (dyn);
*       storage_duration::hybrid_storage    both (small-storage / SBO);
*       storage_duration::unknown           siting undetermined   (bottom).
*
*   COMPOSITION (join == union of sitings):
*   Where a composite's LIFETIME is the MEET of its parts (constexpr only if
* EVERY part is), its STORAGE is the JOIN of the sitings present: a container
* has the static bit if any cells are inline and the dynamic bit if any cells
* are out of line.  storage_join is therefore the canonical combiner here - the
* DUAL of lifetime_meet on the lifetime lattice.
*
*   THE ENTAILMENT (the one link to Lifetime):
*   Storage and Lifetime are distinct axes (where versus when), and a
* statically-lived value may be held in dynamic storage.  The ONLY entailment
* runs one way: inline (static) storage forces a compile-time-expressible SIZE,
*       site(c) = stat  =>  stage(|c|) = c        (but NOT conversely),
* because an inline footprint fixed before execution cannot hold a runtime-
* chosen number of cells.  This header expresses that bridge at the value level
* (storage_forces_compile_time_size / storage_lifetime_consistent); the
* per-type check, which needs a container's size lifetime, lives with the
* container storage traits.
*
*   STORAGE IS NOT THE ALLOCATOR, NOR GROWABILITY:
*   The allocator a container draws upon, and the disciplines it implements, are
* the subject of the Memory Strategies layer; this axis fixes only the siting.
* In particular a runtime-FIXED, non-growable out-of-line buffer is still
* DYNAMIC siting (its cells are out of line) - growability is a Boundedness
* concern, orthogonal to where the cells live.  The memory layer's richer
* storage_kind (which splits dynamic into fixed / growable) PROJECTS onto this
* siting axis; unifying the two is the business of that layer, not this one.
*
*   CONTAINER-AGNOSTIC:
*   This header owns only the siting vocabulary, its algebra, a carrier, the
* cross-axis bridge, an opt-in classifier, and derived predicates.  It performs
* NO shape inference: deciding a container's siting from an extent, an
* allocator, or a reserve() is container-domain and lives with the container
* storage traits, which express their finding AS a storage_duration.
*
*   PORTABILITY:
*   C++11 baseline.  The `_v` companions degrade with the language (inline
* variable on C++17+, plain variable template on C++14, absent on C++11 - where
* the `::value` member is always present).
*
*
* path:      /inc/djinterp/core/meta/storage.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_META_STORAGE_
#define DJINTERP_META_STORAGE_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"      // clean_t, void_t, NS_*, D_ENV_* feature macros
#include "./trait_detect.hpp"   // D_VOID_T, D_TYPE_TRAIT_* detection macros
#include "./lifetime.hpp"       // lifetime, is_compile_time (cross-axis bridge)


NS_DJINTERP


// ===========================================================================
// I.   The storage-duration lattice
// ===========================================================================

// storage_duration
//   enum: a container's cell SITING - static (inline) or dynamic (out of line)
// - with one bit per siting so the four values form a lattice.  static storage
// aligns with the compile-time stage (cells reserved, trivially, at stage c);
// dynamic storage with the runtime stage (cells acquired at stage r).
enum class storage_duration : unsigned
{
    unknown         = 0u,                                  // siting undetermined (bottom)
    static_storage  = (1u << 0),                           // inline / in-object (formal stat)
    dynamic_storage = (1u << 1),                           // out of line / allocated (formal dyn)
    hybrid_storage  = (static_storage | dynamic_storage)   // both: small-storage / SBO (top)
};


// ===========================================================================
// II.  Storage-duration algebra
// ===========================================================================
//   join/meet over the lattice, the siting predicates, and a builder.  Plain
// `constexpr` (not D_CONSTEXPR) so the algebra is always usable in constant
// expressions - including the static initializers of the traits below -
// regardless of the constexpr-instrumentation toggle.

// storage_join
//   function: the lattice join (union of sitings) - the storage of a container
// whose cells include both inline parts (siting _a) and out-of-line parts
// (siting _b).  A container has the static bit if any cells are inline and the
// dynamic bit if any are out of line; combining a static part with a dynamic
// part yields hybrid.  This is the canonical COMPOSITION operator for storage,
// the DUAL of lifetime_meet.
constexpr storage_duration
storage_join(
    storage_duration _a,
    storage_duration _b
) noexcept
{
    return static_cast<storage_duration>(
        static_cast<unsigned>(_a) | static_cast<unsigned>(_b) );
}

// storage_meet
//   function: the lattice meet (intersection of sitings) - provided for lattice
// completeness and cross-axis intersection; storage composition itself uses the
// join.
constexpr storage_duration
storage_meet(
    storage_duration _a,
    storage_duration _b
) noexcept
{
    return static_cast<storage_duration>(
        static_cast<unsigned>(_a) & static_cast<unsigned>(_b) );
}

// make_storage_duration
//   function: builds a storage_duration from two independent siting facts (does
// the container site any cells inline? any cells out of line?).  Both true
// yields hybrid; neither yields unknown.
constexpr storage_duration
make_storage_duration(
    bool _has_static_cells,
    bool _has_dynamic_cells
) noexcept
{
    return static_cast<storage_duration>(
        ( _has_static_cells  ? static_cast<unsigned>(storage_duration::static_storage)  : 0u ) |
        ( _has_dynamic_cells ? static_cast<unsigned>(storage_duration::dynamic_storage) : 0u ) );
}

// has_static_storage_component
//   function: true iff the siting includes the static (inline) bit - i.e. some
// cells are inline (static_storage or hybrid_storage).
constexpr bool
has_static_storage_component(storage_duration _dur) noexcept
{
    return ( ( static_cast<unsigned>(_dur) &
               static_cast<unsigned>(storage_duration::static_storage) ) != 0u );
}

// has_dynamic_storage_component
//   function: true iff the siting includes the dynamic (out-of-line) bit
// (dynamic_storage or hybrid_storage).
constexpr bool
has_dynamic_storage_component(storage_duration _dur) noexcept
{
    return ( ( static_cast<unsigned>(_dur) &
               static_cast<unsigned>(storage_duration::dynamic_storage) ) != 0u );
}

// is_static_storage
//   function: true iff the siting is static (inline) EXCLUSIVELY - the cells
// are wholly in-object.
constexpr bool
is_static_storage(storage_duration _dur) noexcept
{
    return ( _dur == storage_duration::static_storage );
}

// is_dynamic_storage
//   function: true iff the siting is dynamic (out of line) EXCLUSIVELY - the
// cells wholly live in an acquired region.
constexpr bool
is_dynamic_storage(storage_duration _dur) noexcept
{
    return ( _dur == storage_duration::dynamic_storage );
}

// is_hybrid_storage
//   function: true iff the siting spans both - an inline buffer that spills to
// an out-of-line region (small-storage / SBO).
constexpr bool
is_hybrid_storage(storage_duration _dur) noexcept
{
    return ( _dur == storage_duration::hybrid_storage );
}

// is_unknown_storage
//   function: true iff the siting could not be determined (the bottom).
constexpr bool
is_unknown_storage(storage_duration _dur) noexcept
{
    return ( _dur == storage_duration::unknown );
}

// storage_duration_name
//   function: a stable human-readable spelling of a storage_duration, for
// diagnostics and agent-facing summaries.
constexpr const char*
storage_duration_name(storage_duration _dur) noexcept
{
    return ( _dur == storage_duration::hybrid_storage  ? "hybrid_storage"
           : _dur == storage_duration::static_storage  ? "static_storage"
           : _dur == storage_duration::dynamic_storage ? "dynamic_storage"
           :                                             "unknown" );
}


// ===========================================================================
// III. Type-level carrier
// ===========================================================================
//   Lets a storage_duration travel through templates as a type and serve as
// the `::type` of the classifier below.

// storage_duration_constant
//   type: an integral_constant specialized to a storage_duration value (the
// storage analogue of std::bool_constant).
template<storage_duration _Dur>
using storage_duration_constant =
    std::integral_constant<storage_duration, _Dur>;

// unknown_storage_duration / static_storage_duration / ...
//   type: named carriers for the four lattice values, for tag dispatch.
using unknown_storage_duration = storage_duration_constant<storage_duration::unknown>;
using static_storage_duration  = storage_duration_constant<storage_duration::static_storage>;
using dynamic_storage_duration = storage_duration_constant<storage_duration::dynamic_storage>;
using hybrid_storage_duration  = storage_duration_constant<storage_duration::hybrid_storage>;


// ===========================================================================
// IV.  Cross-axis bridge to Lifetime
// ===========================================================================
//   The two intrinsic axes are distinct, and meet only in a single one-way
// entailment: static siting forces a compile-time-expressible size.  These
// value-level helpers state it; the per-type check lives with the container
// storage traits (it needs a container's size lifetime).

// storage_forces_compile_time_size
//   function: the formal site(c) = stat => stage(|c|) = c.  True iff the siting
// has a static (inline) component, which an inline footprint requires its size
// be fixed at compile time.
constexpr bool
storage_forces_compile_time_size(storage_duration _dur) noexcept
{
    return has_static_storage_component(_dur);
}

// storage_lifetime_consistent
//   function: tests the entailment for a (siting, size-lifetime) pair - holds
// iff the siting is not static, OR the size is compile-time-expressible.  A
// static siting paired with a runtime-only size violates the axis and yields
// false; every other combination is consistent.
constexpr bool
storage_lifetime_consistent(
    storage_duration _dur,
    lifetime         _size_life
) noexcept
{
    return ( ( !storage_forces_compile_time_size(_dur) ) ||
             is_compile_time(_size_life) );
}


// ===========================================================================
// V.   Opt-in detection
// ===========================================================================
//   A type declares its own siting by exposing a static member
//       static constexpr djinterp::storage_duration storage_duration_category = ...;
// This is the highest-priority signal: it overrides any structural default.
// (A small-storage type, whose SBO siting cannot be read from its public
// surface, declares hybrid_storage here.)

// has_storage_duration_category
//   trait: detects the opt-in `storage_duration_category` static member (cv-ref
// stripped via clean_t, so the answer agrees for T, const T, T&).
D_TYPE_TRAIT_TRUE(has_storage_duration_category,
                  decltype(clean_t<_Type>::storage_duration_category))

NS_INTERNAL

    // storage_duration_category_member
    //   trait: yields the opt-in `storage_duration_category` when declared, else
    // storage_duration::unknown (primary template - member absent).  The
    // fallback is observed only when has_storage_duration_category is false,
    // where the classifier discards it.
    template<typename _Type,
             typename = void>
    struct storage_duration_category_member
    {
        static constexpr storage_duration value = storage_duration::unknown;
    };

    // storage_duration_category_member (opt-in present)
    //   trait: reads clean_t<_Type>::storage_duration_category.
    template<typename _Type>
    struct storage_duration_category_member<_Type,
        D_VOID_T<decltype(clean_t<_Type>::storage_duration_category)>>
    {
        static constexpr storage_duration value =
            clean_t<_Type>::storage_duration_category;
    };

NS_END  // internal


// ===========================================================================
// VI.  Classification umbrella
// ===========================================================================

// storage_of
//   trait: the container-agnostic siting of a type - its opt-in
// `storage_duration_category` if declared, else storage_duration::unknown.
// This header knows nothing of container shape; the structural inference (an
// extent, an allocator, a reserve()) that resolves the unknown case lives with
// the container storage traits, which feed this opt-in priority.  Exposes the
// value plus a storage_duration_constant carrier as `::type`.
template<typename _Type>
struct storage_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr storage_duration value =
        ( has_storage_duration_category<clean_type>::value
              ? internal::storage_duration_category_member<clean_type>::value
              : storage_duration::unknown );

    using type = storage_duration_constant<value>;
};

// storage_of_t
//   type: convenience alias for storage_of<_Type>::type (a carrier).
template<typename _Type>
using storage_of_t = typename storage_of<_Type>::type;

// storage_of_v
//   value: the `_v` companion of storage_of (a storage_duration, not a bool, so
// it is emitted by hand rather than via D_TYPE_TRAIT_VALUE_BOOL - same
// degradation).
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr storage_duration storage_of_v = storage_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr storage_duration storage_of_v = storage_of<_Type>::value;
#endif


// ===========================================================================
// VII. Derived siting predicates
// ===========================================================================
//   Boolean trait projections of storage_of, for SFINAE branches and
// requires-clauses.  At this container-agnostic layer they fire only for types
// that opt in via storage_duration_category; the container storage traits
// provide the shape-based predicates for the std:: and djinterp containers.
// Each emits its `_v` companion through the canonical trait_detect macro.

// is_static_storage_type
//   trait: true iff _Type's declared siting is static (inline) exclusively.
template<typename _Type>
struct is_static_storage_type
    : std::integral_constant<bool,
          is_static_storage(storage_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_static_storage_type)

// is_dynamic_storage_type
//   trait: true iff _Type's declared siting is dynamic (out of line)
// exclusively.
template<typename _Type>
struct is_dynamic_storage_type
    : std::integral_constant<bool,
          is_dynamic_storage(storage_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_dynamic_storage_type)

// is_hybrid_storage_type
//   trait: true iff _Type's declared siting spans both (small-storage / SBO).
template<typename _Type>
struct is_hybrid_storage_type
    : std::integral_constant<bool,
          is_hybrid_storage(storage_of<_Type>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_hybrid_storage_type)


NS_END  // djinterp


#endif  // DJINTERP_META_STORAGE_
