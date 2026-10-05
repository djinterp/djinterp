/*******************************************************************************
* djinterp [core]                                  container_memory_strategy.hpp
*
* Container-side binding for the memory-strategy axis  (Sub-option A).
*   This is the ONLY module on the axis that is container-aware, and the
* dependency runs ONE WAY: this header knows the memory-strategy core, and the
* core knows nothing of it.  Nothing under inc/djinterp/core/memory names a
* container, and no memory suite tests a container shape.
*
*   It resolves WHAT supplies a container's memory and WITH WHAT discipline,
* with no opt-in required, by precedence:
*
*     1. explicit `using memory_strategy = ...;` member on the container
*     2. its allocator_type            -> wrapped as allocator_memory_strategy
*     3. compile-time extent / tuple_size -> static_extent_strategy
*     4. unknown
*
*   A resolved strategy's DECLARED storage_kind wins; otherwise the
* container's
* own shape (extent / tuple_size / capacity+reserve) is inferred locally into
* the core storage_kind.  This SUBSUMES the static/dynamic predicates of
* container_storage_traits.hpp and cross-validates against them.
*
*   Shape inference is reproduced here (a dozen lines) rather than taken from
* container_storage_traits.hpp ON PURPOSE: that header declares its own
* storage_kind enum, and including it alongside the core would be a
* redefinition
* clash until the two enums are unified.
*
* DEPENDENCIES (the C-backed strategy layer, canonical since 2026.10.01):
*   memory_strategy_common.hpp - storage_kind and the traits read here
*   memory_strategy.hpp        - allocator_memory_strategy (precedence step 2)
*   static_extent_strategy, the descriptive strategy of precedence step 3,
* is defined below.
*
*   Both sit flat in inc/djinterp/core/memory, with the rest of the layer.
*
* path:      /inc/djinterp/core/container/traits/container_memory_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_CONTAINER_MEMORY_STRATEGY_HPP
#define DJINTERP_CONTAINER_TRAITS_CONTAINER_MEMORY_STRATEGY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../memory/memory_strategy_common.hpp"  // storage_kind, has_storage_kind,
                                                   // strategy_storage_kind_of,
                                                   // is_stable_strategy
#include "../../memory/memory_strategy.hpp"         // allocator_memory_strategy


NS_DJINTERP


// ===========================================================================
// I.   Local container-shape detection
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct cms_has_extent : std::false_type {};
    template<typename Type>
    struct cms_has_extent<Type, void_t<decltype(Type::extent)>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_tuple_size : std::false_type {};
    template<typename Type>
    struct cms_has_tuple_size<Type, void_t<
        decltype(std::tuple_size<Type>::value)>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_capacity : std::false_type {};
    template<typename Type>
    struct cms_has_capacity<Type, void_t<
        decltype(std::declval<const Type&>().capacity())>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_reserve : std::false_type {};
    template<typename Type>
    struct cms_has_reserve<Type, void_t<
        decltype(std::declval<Type&>().reserve(std::size_t{}))>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_allocator : std::false_type {};
    template<typename Type>
    struct cms_has_allocator<Type, void_t<
        typename Type::allocator_type>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_value_type : std::false_type {};
    template<typename Type>
    struct cms_has_value_type<Type, void_t<
        typename Type::value_type>>
        : std::true_type {};

    template<typename Type,
             typename = void>
    struct cms_has_strategy_alias : std::false_type {};
    template<typename Type>
    struct cms_has_strategy_alias<Type, void_t<
        typename Type::memory_strategy>>
        : std::true_type {};

    // cms_infer_kind
    //   helper: container-shape inference into core storage_kind.
    template<typename Type>
    struct cms_infer_kind
    {
        using c = clean_t<Type>;
        static constexpr storage_kind value =
            ( cms_has_extent<c>::value || cms_has_tuple_size<c>::value )
                ? storage_kind::static_storage
          : ( cms_has_capacity<c>::value && cms_has_reserve<c>::value )
                ? storage_kind::dynamic_storage
          : ( cms_has_capacity<c>::value && !cms_has_reserve<c>::value )
                ? storage_kind::fixed_storage
          : storage_kind::unknown;
    };

    // cms_value_type_alias
    template<typename Type>
    using cms_value_type_alias = typename Type::value_type;

NS_END  // internal


// static_extent_strategy
//   type: a descriptive strategy for a container with a static extent and no
// declared strategy. It states what the strategy layer's traits read -- the
// storage kind, pointer stability, the extent -- and holds nothing. The
// C-backed buffer_memory_strategy cannot stand in for it: it owns its
// storage, and the resolver does not know the extent, so the tag form
// (Count 0) would declare a zero-size array. It moved here, to its only
// user, from the retired memory/static_buffer_strategy.hpp, and the owner's
// ruling of 2026.10.01 keeps it here.
template<typename    Type,
         std::size_t Count>
struct static_extent_strategy
{
    using value_type = Type;

    static D_CONSTEXPR const storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static D_CONSTEXPR const bool         pointer_stable = true;
    static D_CONSTEXPR const std::size_t  extent         = Count;
};

// The resolver's step 3 relies on the strategy layer's traits reading this
// tag as pointer-stable static storage, in its tag form (Count 0) too; the
// assumption is stated here, where the tag is defined.
static_assert(has_storage_kind<static_extent_strategy<int, 0> >::value,
              "static_extent_strategy must declare strategy_storage_kind, "
              "the member the strategy layer's traits read");
static_assert( (strategy_storage_kind_of<static_extent_strategy<int, 0> >::value ==
                storage_kind::static_storage),
              "static_extent_strategy must describe static storage");
static_assert(is_stable_strategy<static_extent_strategy<int, 0> >::value,
              "static_extent_strategy must describe pointer-stable storage");


// ===========================================================================
// II.  Strategy resolution (precedence chain)
// ===========================================================================

NS_INTERNAL

    template<typename Container,
             typename = void>
    struct strategy_resolver
    {
        // step 3 / 4: static extent -> descriptive static strategy; else void.
        using c = clean_t<Container>;
        using type = typename std::conditional<
            ( cms_has_extent<c>::value || cms_has_tuple_size<c>::value ),
            static_extent_strategy<
                detected_or_t<unsigned char, cms_value_type_alias, c>,
                0>,    // extent tag only; true N not structurally recoverable
            void
        >::type;
    };

    // step 1: explicit member alias wins.
    template<typename Container>
    struct strategy_resolver<Container,
        typename std::enable_if<
            cms_has_strategy_alias<clean_t<Container>>::value
        >::type>
    {
        using type = typename clean_t<Container>::memory_strategy;
    };

    // step 2: allocator_type, when no explicit strategy alias.
    template<typename Container>
    struct strategy_resolver<Container,
        typename std::enable_if<
                !cms_has_strategy_alias<clean_t<Container>>::value
             &&  cms_has_allocator<clean_t<Container>>::value
        >::type>
    {
        using type = allocator_memory_strategy<
            typename clean_t<Container>::allocator_type>;
    };

NS_END  // internal


// ===========================================================================
// III. container_memory_strategy
// ===========================================================================

template<typename Container>
struct container_memory_strategy
{
private:
    using clean_type = clean_t<Container>;

public:
    using type =
        typename internal::strategy_resolver<clean_type>::type;

    // declared strategy kind wins; else local shape inference.
    static constexpr storage_kind kind =
        ( !std::is_void<type>::value
          && has_storage_kind<clean_t<type>>::value )
            ? strategy_storage_kind_of<clean_t<type>>::value
            : internal::cms_infer_kind<clean_type>::value;

    static constexpr bool is_static  = ( kind == storage_kind::static_storage );
    static constexpr bool is_fixed   = ( kind == storage_kind::fixed_storage );
    static constexpr bool is_dynamic = ( kind == storage_kind::dynamic_storage );

    static constexpr bool pointer_stable =
        ( !std::is_void<type>::value
          && is_stable_strategy<clean_t<type>>::value );

    static constexpr bool resolved =
        !std::is_void<type>::value;
};

// container_memory_strategy_t
//   type: the carrier of container_memory_strategy -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Container>
using container_memory_strategy_t =
    typename container_memory_strategy<Container>::type;


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Container>
    inline constexpr bool container_uses_static_storage_v =
        container_memory_strategy<Container>::is_static;
    template<typename Container>
    inline constexpr bool container_uses_fixed_storage_v =
        container_memory_strategy<Container>::is_fixed;
    template<typename Container>
    inline constexpr bool container_uses_dynamic_storage_v =
        container_memory_strategy<Container>::is_dynamic;
#endif


// ===========================================================================
// IV.  Concepts
// ===========================================================================

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    template<typename Type>
    concept strategy_resolvable_container =
        container_memory_strategy<clean_t<Type>>::resolved;

    template<typename Type>
    concept static_storage_container_strategy =
        container_uses_static_storage_v<clean_t<Type>>;

    template<typename Type>
    concept dynamic_storage_container_strategy =
        container_uses_dynamic_storage_v<clean_t<Type>>;

#endif


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_CONTAINER_MEMORY_STRATEGY_HPP
