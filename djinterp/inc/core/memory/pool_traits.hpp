/******************************************************************************
* djinterp [container]                                       pool_traits.hpp
*
* Pool SFINAE detection traits:
*   This header provides compile-time structural traits for detecting
* and classifying pool resources and pool-backed allocators.  Detection
* is purely structural — no tagging, no base-class checks.
*
*   All traits operate on the `clean_t` (cv-ref stripped) form of the
* type and produce `static constexpr bool` values.  C++17 `_v`
* variable templates are provided for every public trait.
*
* Traits provided:
*   POOL RESOURCE DETECTION
*   - is_pool_resource<T>            does T satisfy pool resource protocol?
*   - has_acquire<T>                 does T expose acquire()?
*   - has_release<T>                 does T expose release(void*)?
*   - has_pool_reset<T>             does T expose reset()?
*   - is_pointer_stable_pool<T>      does T guarantee pointer stability?
*   - is_monotonic_pool<T>           is release a no-op?
*   - has_generational_sweep<T>      does T support sweep(gen)?
*
*   ALLOCATOR CLASSIFICATION
*   - is_pool_allocator<T>           is T a pool-backed allocator?
*   - is_pool_backed_container<T>    does T use a pool-backed allocator?
*
*   POOL TYPE EXTRACTION
*   - pool_resource_type<T>          extracts the pool_resource type
*                                    from a pool_allocator (SFINAE-safe)
*
*   COMBINED CLASSIFICATION
*   - pool_class<T>                  aggregate classification struct
*
* TABLE OF CONTENTS
* =================
* 1.   Method Detection
* 2.   Pool Resource Classification
* 3.   Pool Stability and Release Classification
* 4.   Allocator Classification
* 5.   Type Extraction
* 6.   Combined Classification
*
*
* path:      /inc/container/pool/pool_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_POOL_TRAITS_
#define DJINTERP_CONTAINER_POOL_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./pool.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =============================================================================
// I.   Method Detection
// =============================================================================

// --- basic container protocol (local definitions to
//     avoid hard dependency on container_traits.hpp) ---

D_TYPE_TRAIT_TRUE(has_value_type,
    typename _Type::value_type)

D_TYPE_TRAIT_TRUE(has_size_accessor,
    decltype(std::declval<const _Type&>().size()))

D_TYPE_TRAIT_TRUE(has_allocator_type,
    typename _Type::allocator_type)

// --- pool resource protocol ---

D_TYPE_TRAIT_TRUE(has_acquire,
    decltype(std::declval<_Type&>().acquire()))

D_TYPE_TRAIT_TRUE(has_release,
    decltype(std::declval<_Type&>().release(
        std::declval<void*>())))

D_TYPE_TRAIT_TRUE(has_pool_reset,
    decltype(std::declval<_Type&>().reset()))

D_TYPE_TRAIT_TRUE(has_pool_reserve,
    decltype(std::declval<_Type&>().reserve(
        std::declval<std::size_t>())))

D_TYPE_TRAIT_TRUE(has_bytes_per_slot,
    decltype(_Type::bytes_per_slot()))

D_TYPE_TRAIT_TRUE(has_slot_alignment,
    decltype(_Type::alignment()))

// --- policy constant detection ---

D_TYPE_TRAIT_TRUE(has_pointer_stable_constant,
    decltype(_Type::pointer_stable))

D_TYPE_TRAIT_TRUE(has_supports_individual_release_constant,
    decltype(_Type::supports_individual_release))

D_TYPE_TRAIT_TRUE(has_supports_generational_sweep_constant,
    decltype(_Type::supports_generational_sweep))

// --- allocator resource detection ---

D_TYPE_TRAIT_TRUE(has_resource_method,
    decltype(std::declval<const _Type&>().resource()))

// --- generation detection ---

D_TYPE_TRAIT_TRUE(has_current_generation,
    decltype(std::declval<const _Type&>().current_generation()))

D_TYPE_TRAIT_TRUE(has_advance_generation,
    decltype(std::declval<_Type&>().advance_generation()))

// --- memory accounting ---

D_TYPE_TRAIT_TRUE(has_bytes_allocated,
    decltype(std::declval<const _Type&>().bytes_allocated()))

D_TYPE_TRAIT_TRUE(has_bytes_in_use,
    decltype(std::declval<const _Type&>().bytes_in_use()))

D_TYPE_TRAIT_TRUE(has_utilization,
    decltype(std::declval<const _Type&>().utilization()))


// =============================================================================
// II.  Pool Resource Classification
// =============================================================================
// A pool resource is any type that exposes the minimum pool
// protocol: acquire() returning void*, release(void*),
// size(), and value_type.  Detection is purely structural.

// is_pool_resource
//   trait: detects whether _Type satisfies the minimum pool
// resource protocol.
template<typename _Type>
struct is_pool_resource
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_acquire_v<clean_type>       &&
          has_release_v<clean_type>       &&
          has_size_accessor_v<clean_type> &&
          has_value_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_pool_resource_v =
    is_pool_resource<_Type>::value;


// =============================================================================
// III. Pool Stability and Release Classification
// =============================================================================
// These traits inspect the policy constants exposed by
// pool_resource to classify its stability and release
// semantics.

NS_INTERNAL

    // pointer_stable_check
    //   trait: reads _Type::pointer_stable when available.
    template<typename _Type,
             bool     _HasConstant =
                 has_pointer_stable_constant_v<clean_t<_Type>>>
    struct pointer_stable_check : std::false_type
    {};

    template<typename _Type>
    struct pointer_stable_check<_Type, true>
        : std::bool_constant<clean_t<_Type>::pointer_stable>
    {};

    // individual_release_check
    //   trait: reads _Type::supports_individual_release.
    template<typename _Type,
             bool     _HasConstant =
                 has_supports_individual_release_constant_v<
                     clean_t<_Type>>>
    struct individual_release_check : std::false_type
    {};

    template<typename _Type>
    struct individual_release_check<_Type, true>
        : std::bool_constant<
              clean_t<_Type>::supports_individual_release>
    {};

    // generational_sweep_check
    //   trait: reads _Type::supports_generational_sweep.
    template<typename _Type,
             bool     _HasConstant =
                 has_supports_generational_sweep_constant_v<
                     clean_t<_Type>>>
    struct generational_sweep_check : std::false_type
    {};

    template<typename _Type>
    struct generational_sweep_check<_Type, true>
        : std::bool_constant<
              clean_t<_Type>::supports_generational_sweep>
    {};

NS_END  // internal

// is_pointer_stable_pool
//   trait: true if pool guarantees that pointers remain
// valid across growth (chunked block layout).
template<typename _Type>
struct is_pointer_stable_pool
    : std::bool_constant<
          internal::pointer_stable_check<_Type>::value>
{};

template<typename _Type>
inline constexpr bool is_pointer_stable_pool_v =
    is_pointer_stable_pool<_Type>::value;

// supports_individual_release
//   trait: true if pool supports per-slot deallocation
// (free list or generational).
template<typename _Type>
struct supports_individual_release
    : std::bool_constant<
          internal::individual_release_check<_Type>::value>
{};

template<typename _Type>
inline constexpr bool supports_individual_release_v =
    supports_individual_release<_Type>::value;

// is_monotonic_pool
//   trait: true if pool does not support individual
// release.  All memory is reclaimed only on reset()
// or destruction.
template<typename _Type>
struct is_monotonic_pool
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_pool_resource_v<clean_type> &&
          !supports_individual_release_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_monotonic_pool_v =
    is_monotonic_pool<_Type>::value;

// has_generational_sweep
//   trait: true if pool supports sweep(gen) for batch
// reclamation by generation.
template<typename _Type>
struct has_generational_sweep
    : std::bool_constant<
          internal::generational_sweep_check<_Type>::value>
{};

template<typename _Type>
inline constexpr bool has_generational_sweep_v =
    has_generational_sweep<_Type>::value;

// has_memory_accounting
//   trait: true if pool exposes byte-level memory
// accounting (bytes_allocated, bytes_in_use, utilization).
template<typename _Type>
struct has_memory_accounting
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_bytes_allocated_v<clean_type> &&
          has_bytes_in_use_v<clean_type>    &&
          has_utilization_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_memory_accounting_v =
    has_memory_accounting<_Type>::value;


// =============================================================================
// IV.  Allocator Classification
// =============================================================================
// Detects whether an allocator wraps a pool_resource,
// and whether a container uses such an allocator.

// is_pool_allocator
//   trait: true if _Type is a pool-backed allocator.
// Detection: the allocator exposes a resource() method.
template<typename _Type>
struct is_pool_allocator
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_value_type_v<clean_type>    &&
          has_resource_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_pool_allocator_v =
    is_pool_allocator<_Type>::value;

// is_pool_backed_container
//   trait: true if _Type is a container whose allocator
// is a pool allocator.
template<typename _Type>
struct is_pool_backed_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_allocator_type_v<clean_type> &&
          is_pool_allocator_v<
              typename clean_type::allocator_type> );
};

template<typename _Type>
inline constexpr bool is_pool_backed_container_v =
    is_pool_backed_container<_Type>::value;


// =============================================================================
// V.   Type Extraction
// =============================================================================

NS_INTERNAL

    // pool_resource_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct pool_resource_type_helper
    {
        using type = void;
    };

    // pool_resource_type_helper (success case)
    //   trait: extracts the pool_resource type from a
    // pool allocator via its resource() return type.
    template<typename _Type>
    struct pool_resource_type_helper<
        _Type,
        void_t<decltype(std::declval<const _Type&>()
                   .resource())>
    >
    {
        using type = std::remove_pointer_t<
            decltype(std::declval<const _Type&>()
                .resource())>;
    };

NS_END  // internal

// pool_resource_type
//   trait: SFINAE-safe extraction of the pool_resource
// type from a pool allocator.  Produces void if _Type
// does not expose resource().
template<typename _Type>
struct pool_resource_type
{
    using type =
        typename internal::pool_resource_type_helper<
            clean_t<_Type>>::type;
};

// pool_resource_type_t
//   type: convenience alias.
template<typename _Type>
using pool_resource_type_t =
    typename pool_resource_type<_Type>::type;


// =============================================================================
// VI.  Combined Classification
// =============================================================================

// pool_class
//   struct: complete classification of a pool resource
// type.  All classification is compile-time using static
// constexpr bool members.
template<typename _Type>
struct pool_class
{
    // identity
    static constexpr bool is_pool             = is_pool_resource_v<_Type>;
    // pointer stability
    static constexpr bool pointer_stable      = is_pointer_stable_pool_v<_Type>;
    // release semantics
    static constexpr bool individual_release  = supports_individual_release_v<_Type>;
    static constexpr bool monotonic           = is_monotonic_pool_v<_Type>;
    static constexpr bool generational        = has_generational_sweep_v<_Type>;
    // capacity
    static constexpr bool has_reset           = has_pool_reset_v<_Type>;
    static constexpr bool has_reserve         = has_pool_reserve_v<_Type>;
    // memory accounting
    static constexpr bool has_accounting      = has_memory_accounting_v<_Type>;
    // type extraction
    using resource_type                       = pool_resource_type_t<_Type>;
};

// pool_allocator_class
//   struct: complete classification of a pool allocator.
template<typename _Type>
struct pool_allocator_class
{
    // identity
    static constexpr bool is_pool_alloc       = is_pool_allocator_v<_Type>;
    // underlying pool classification
    using resource_type                       = pool_resource_type_t<_Type>;
    static constexpr bool pointer_stable      = is_pointer_stable_pool_v<resource_type>;
    static constexpr bool individual_release  = supports_individual_release_v<resource_type>;
    static constexpr bool monotonic           = is_monotonic_pool_v<resource_type>;
    static constexpr bool generational        = has_generational_sweep_v<resource_type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_POOL_TRAITS_
