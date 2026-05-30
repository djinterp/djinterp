/******************************************************************************
* djinterp [container]                             container_memory_strategy.hpp
*
* Container-side binding for the memory-strategy axis  (Sub-option A).
*   This is the ONLY module on the axis that is container-aware.  It resolves
* WHAT supplies a container's memory and WITH WHAT discipline, with no opt-in
* required, by precedence:
*
*     1. explicit `using memory_strategy = ...;` member on the container
*     2. its allocator_type            -> wrapped as allocator_memory_strategy
*     3. compile-time extent / tuple_size -> static_extent_strategy
*     4. unknown
*
*   A resolved strategy's DECLARED storage_kind wins; otherwise the container's
* own shape (extent / tuple_size / capacity+reserve) is inferred locally into
* the core storage_kind.  This SUBSUMES the static/dynamic predicates of
* container_storage_traits.hpp and cross-validates against them.
*
*   Shape inference is reproduced here (a dozen lines) rather than taken from
* container_storage_traits.hpp ON PURPOSE: that header declares its own
* storage_kind enum, and including it alongside the core would be a redefinition
* clash until the two enums are unified.  See the note in
* memory_strategy_traits.hpp.
*
* DEPENDENCIES:
*   memory_strategy_traits.hpp      - core contract + storage_kind
*   allocator_memory_strategy.hpp   - allocator wrapping (precedence step 2)
*   static_buffer_strategy.hpp      - static_extent_strategy (precedence step 3)
*
*
* path:      /inc/djinterp/core/container/traits/container_memory_strategy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_MEMORY_STRATEGY_
#define DJINTERP_CONTAINER_MEMORY_STRATEGY_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../memory/strategy/memory_strategy_traits.hpp"
#include "../../memory/strategy/allocator_memory_strategy.hpp"
#include "../../memory/strategy/static_buffer_strategy.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Local container-shape detection
// ===========================================================================

NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct cms_has_extent : std::false_type {};
    template<typename _Type>
    struct cms_has_extent<_Type, void_t<decltype(_Type::extent)>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_tuple_size : std::false_type {};
    template<typename _Type>
    struct cms_has_tuple_size<_Type, void_t<
        decltype(std::tuple_size<_Type>::value)>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_capacity : std::false_type {};
    template<typename _Type>
    struct cms_has_capacity<_Type, void_t<
        decltype(std::declval<const _Type&>().capacity())>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_reserve : std::false_type {};
    template<typename _Type>
    struct cms_has_reserve<_Type, void_t<
        decltype(std::declval<_Type&>().reserve(std::size_t{}))>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_allocator : std::false_type {};
    template<typename _Type>
    struct cms_has_allocator<_Type, void_t<
        typename _Type::allocator_type>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_value_type : std::false_type {};
    template<typename _Type>
    struct cms_has_value_type<_Type, void_t<
        typename _Type::value_type>>
        : std::true_type {};

    template<typename _Type,
             typename = void>
    struct cms_has_strategy_alias : std::false_type {};
    template<typename _Type>
    struct cms_has_strategy_alias<_Type, void_t<
        typename _Type::memory_strategy>>
        : std::true_type {};

    // cms_infer_kind
    //   helper: container-shape inference into core storage_kind.
    template<typename _Type>
    struct cms_infer_kind
    {
        using c = clean_t<_Type>;
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
    template<typename _Type>
    using cms_value_type_alias = typename _Type::value_type;

NS_END  // internal


// ===========================================================================
// II.  Strategy resolution (precedence chain)
// ===========================================================================

NS_INTERNAL

    template<typename _Container,
             typename = void>
    struct strategy_resolver
    {
        // step 3 / 4: static extent -> descriptive static strategy; else void.
        using c = clean_t<_Container>;
        using type = typename std::conditional<
            ( cms_has_extent<c>::value || cms_has_tuple_size<c>::value ),
            static_extent_strategy<
                detected_or_t<unsigned char, cms_value_type_alias, c>,
                0>,    // extent tag only; true N not structurally recoverable
            void
        >::type;
    };

    // step 1: explicit member alias wins.
    template<typename _Container>
    struct strategy_resolver<_Container,
        typename std::enable_if<
            cms_has_strategy_alias<clean_t<_Container>>::value
        >::type>
    {
        using type = typename clean_t<_Container>::memory_strategy;
    };

    // step 2: allocator_type, when no explicit strategy alias.
    template<typename _Container>
    struct strategy_resolver<_Container,
        typename std::enable_if<
                !cms_has_strategy_alias<clean_t<_Container>>::value
             &&  cms_has_allocator<clean_t<_Container>>::value
        >::type>
    {
        using type = allocator_memory_strategy<
            typename clean_t<_Container>::allocator_type>;
    };

NS_END  // internal


// ===========================================================================
// III. container_memory_strategy
// ===========================================================================

template<typename _Container>
struct container_memory_strategy
{
private:
    using clean_type = clean_t<_Container>;

public:
    using type =
        typename internal::strategy_resolver<clean_type>::type;

    // declared strategy kind wins; else local shape inference.
    static constexpr storage_kind kind =
        ( !std::is_void<type>::value
          && has_strategy_kind_constant_v<clean_t<type>> )
            ? memory_strategy_kind_of<clean_t<type>>::value
            : internal::cms_infer_kind<clean_type>::value;

    static constexpr bool is_static  = ( kind == storage_kind::static_storage );
    static constexpr bool is_fixed   = ( kind == storage_kind::fixed_storage );
    static constexpr bool is_dynamic = ( kind == storage_kind::dynamic_storage );

    static constexpr bool pointer_stable =
        ( !std::is_void<type>::value
          && is_pointer_stable_strategy<clean_t<type>>::value );

    static constexpr bool resolved =
        !std::is_void<type>::value;
};

// container_memory_strategy_t
template<typename _Container>
using container_memory_strategy_t =
    typename container_memory_strategy<_Container>::type;


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Container>
    inline constexpr bool container_uses_static_storage_v =
        container_memory_strategy<_Container>::is_static;
    template<typename _Container>
    inline constexpr bool container_uses_fixed_storage_v =
        container_memory_strategy<_Container>::is_fixed;
    template<typename _Container>
    inline constexpr bool container_uses_dynamic_storage_v =
        container_memory_strategy<_Container>::is_dynamic;
#endif


// ===========================================================================
// IV.  Concepts
// ===========================================================================

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    template<typename _Type>
    concept strategy_resolvable_container =
        container_memory_strategy<clean_t<_Type>>::resolved;

    template<typename _Type>
    concept static_storage_container_strategy =
        container_uses_static_storage_v<clean_t<_Type>>;

    template<typename _Type>
    concept dynamic_storage_container_strategy =
        container_uses_dynamic_storage_v<clean_t<_Type>>;

#endif


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_MEMORY_STRATEGY_
