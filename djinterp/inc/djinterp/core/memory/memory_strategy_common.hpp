/*******************************************************************************
* djinterp [core]                                     memory_strategy_common.hpp
*
* The memory-strategy contract -- C++ notation over the C allocators.
*   A STRATEGY is a uniform surface over "where storage comes from", so that a
* container can be written once and instantiated over an arena, a pool, a
* std::allocator, or an inline buffer without knowing which. It publishes two
* things: an allocate/deallocate pair, and a set of DESCRIPTIVE CONSTANTS the
* container reads to decide what it is allowed to do.
*
*   THIS TIER HAS NO C COUNTERPART, AND CORRECTLY SO. It is structural
* detection over a duck-typed surface, resolved entirely at translation time,
* and C has no notion of any of that -- so per the framework's phase rule
* (AGENT_README.md section 8) it is C++-only NOTATION rather than a second
* implementation. Nothing here allocates, computes an offset, or decides a
* policy. Every fact it publishes about a djinterp allocator is READ OFF the C
* kernel's configuration, not asserted independently:
*
*     an arena is pointer-stable        <- D_INTERNAL_ARENA_CHAIN
*     a pool releases individually      <- its d_pool_policy
*     a pool sweeps generations         <- D_INTERNAL_POOL_GENERATIONAL
*
* That is what "derived from the C" means at this tier: the C config is the
* single source of truth, and the constants are a projection of it.
*
*   WHY THIS IS A _common.hpp: arena_memory_strategy, pool_memory_strategy,
* buffer_memory_strategy and allocator_memory_strategy all need storage_kind,
* the detection traits, and the classification. Four modules, so the suffix is
* earned rather than decorative.
*
* THE CONTRACT, in full:
*
*   using value_type                              // element or byte
*   static constexpr storage_kind strategy_storage_kind
*   static constexpr bool         pointer_stable
*   static constexpr bool         supports_individual_release
*   value_type* allocate(size_type _count)
*   void        deallocate(value_type* _ptr, size_type _count)
*
*   optional:
*   static constexpr bool        supports_generational_sweep
*   static constexpr std::size_t extent            // static storage only
*   void reset()
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/memory_strategy_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    storage_kind vocabulary
      -----------------------
II.   structural detection
      --------------------
      a. has_strategy_allocate / _deallocate / _reset
      b. has_storage_kind / has_pointer_stable / has_individual_release
III.  classification
      --------------
      a. is_memory_strategy / is_byte_strategy / is_element_strategy
      b. is_stable_strategy / is_releasing_strategy / is_static_strategy
IV.   descriptive accessors
      ---------------------
      a. strategy_storage_kind_of / strategy_extent_of
V.    CONCEPTS  (C++20, gated)
      -----------------------
*/

#ifndef DJINTERP_MEMORY_MEMORY_STRATEGY_COMMON_HPP
#define DJINTERP_MEMORY_MEMORY_STRATEGY_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./mem_common.hpp"
#include "../meta/member_types.hpp"  // has_value_type


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                 I.   storage_kind vocabulary                            ///
///////////////////////////////////////////////////////////////////////////////

// storage_kind
//   enum: the storage discipline a strategy declares.
//   A container reads this to decide what it may do -- whether it can grow,
// whether it must bound itself, whether it can be constructed without
// touching an allocator at all. The values are ordered from least to most
// capable, which lets a container compare rather than switch.
enum class storage_kind
{
    // compile-time fixed capacity, living inside the object. Never allocates
    static_storage,

    // runtime-fixed capacity, non-growable. Allocates once, then reports
    // exhaustion rather than growing
    fixed_storage,

    // growable. Obtains more storage on demand
    dynamic_storage,

    // not a recognizable strategy
    unknown
};


NS_INTERNAL

    // void_t
    //   type: the detection-idiom sink. Spelled locally because std::void_t
    // is C++17 and this tier's floor is C++11.
    template<typename...>
    struct strategy_make_void
    {
        using type = void;
    };

    template<typename... Args>
    using strategy_void_t = typename strategy_make_void<Args...>::type;

    // strategy_clean
    //   type: the strategy type with references and cv-qualifiers stripped,
    // so a trait applied to `const S&` answers about S.
    template<typename Type>
    struct strategy_clean
    {
        using type = typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type;
    };

    template<typename Type>
    using strategy_clean_t = typename strategy_clean<Type>::type;

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                   II.   STRUCTURAL DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////
//   Every trait below duck-types: it asks whether an EXPRESSION compiles, not
// whether a tag is present or a base is inherited. A caller's own strategy
// therefore satisfies these without including anything from here and without
// naming djinterp at all, which is the property that makes the contract a
// contract rather than a class hierarchy.

// has_strategy_allocate
//   trait: detects allocate(size_type).
template<typename Type,
         typename Enable = void>
struct has_strategy_allocate : std::false_type
{};

template<typename Type>
struct has_strategy_allocate<
    Type,
    internal::strategy_void_t<decltype(
        std::declval<internal::strategy_clean_t<Type>&>().allocate(
            std::declval<std::size_t>()))>> : std::true_type
{};

// has_strategy_deallocate
//   trait: detects deallocate(pointer, size_type).
template<typename Type,
         typename Enable = void>
struct has_strategy_deallocate : std::false_type
{};

template<typename Type>
struct has_strategy_deallocate<
    Type,
    internal::strategy_void_t<decltype(
        std::declval<internal::strategy_clean_t<Type>&>().deallocate(
            std::declval<typename internal::strategy_clean_t<
                Type>::value_type*>(),
            std::declval<std::size_t>()))>> : std::true_type
{};

// has_strategy_reset
//   trait: detects reset(), the bulk reclamation an arena-backed strategy
// offers and a std::allocator-backed one cannot.
template<typename Type,
         typename Enable = void>
struct has_strategy_reset : std::false_type
{};

template<typename Type>
struct has_strategy_reset<
    Type,
    internal::strategy_void_t<decltype(
        std::declval<internal::strategy_clean_t<Type>&>().reset())>>
    : std::true_type
{};

// has_storage_kind
//   trait: detects the strategy_storage_kind constant.
template<typename Type,
         typename Enable = void>
struct has_storage_kind : std::false_type
{};

template<typename Type>
struct has_storage_kind<
    Type,
    internal::strategy_void_t<decltype(
        internal::strategy_clean_t<Type>::strategy_storage_kind)>>
    : std::true_type
{};

// has_pointer_stable
//   trait: detects the pointer_stable constant.
template<typename Type,
         typename Enable = void>
struct has_pointer_stable : std::false_type
{};

template<typename Type>
struct has_pointer_stable<
    Type,
    internal::strategy_void_t<decltype(
        internal::strategy_clean_t<Type>::pointer_stable)>> : std::true_type
{};

// has_individual_release
//   trait: detects the supports_individual_release constant.
template<typename Type,
         typename Enable = void>
struct has_individual_release : std::false_type
{};

template<typename Type>
struct has_individual_release<
    Type,
    internal::strategy_void_t<decltype(
        internal::strategy_clean_t<Type>::supports_individual_release)>>
    : std::true_type
{};

// has_generational_sweep
//   trait: detects the supports_generational_sweep constant, which only a
// pool-backed strategy over a generational pool declares true.
template<typename Type,
         typename Enable = void>
struct has_generational_sweep : std::false_type
{};

template<typename Type>
struct has_generational_sweep<
    Type,
    internal::strategy_void_t<decltype(
        internal::strategy_clean_t<Type>::supports_generational_sweep)>>
    : std::true_type
{};

// has_extent
//   trait: detects the extent constant, which only a static strategy has.
template<typename Type,
         typename Enable = void>
struct has_extent : std::false_type
{};

template<typename Type>
struct has_extent<
    Type,
    internal::strategy_void_t<decltype(
        internal::strategy_clean_t<Type>::extent)>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                    III.   CLASSIFICATION                                ///
///////////////////////////////////////////////////////////////////////////////

// is_memory_strategy
//   trait: true when a type satisfies the whole contract -- the operational
// pair AND the three mandatory descriptive constants.
//   BOTH HALVES ARE REQUIRED, and the reason is worth stating: a type with
// allocate/deallocate but no constants would compile against a container that
// never queried them and fail against one that did, so the container's
// behaviour would depend on which of its members were instantiated. Demanding
// the constants up front turns that into one diagnostic at one place.
template<typename Type>
struct is_memory_strategy
{
    static D_CONSTEXPR const bool value =
        ( has_value_type<internal::strategy_clean_t<Type> >::value            &&
          has_strategy_allocate<Type>::value     &&
          has_strategy_deallocate<Type>::value   &&
          has_storage_kind<Type>::value          &&
          has_pointer_stable<Type>::value        &&
          has_individual_release<Type>::value );
};

NS_INTERNAL

    // byte_like
    //   trait: whether a value_type is a byte rather than an element. A
    // byte-typed strategy vends untyped storage the caller will place objects
    // into; an element-typed one vends storage already shaped for its type.
    template<typename Type>
    struct byte_like
    {
        static D_CONSTEXPR const bool value =
            ( std::is_same<Type, char>::value          ||
              std::is_same<Type, unsigned char>::value ||
              std::is_same<Type, signed char>::value );
    };

    // strategy_value_type_or_void
    //   type: a strategy's value_type, or void when it has none, so the
    // classifications below can be written without a second specialization
    // for the non-strategy case.
    template<typename Type,
             bool HasValue =
                 has_value_type<internal::strategy_clean_t<Type> >::value>
    struct strategy_value_type_or_void
    {
        using type = typename strategy_clean_t<Type>::value_type;
    };

    template<typename Type>
    struct strategy_value_type_or_void<Type, false>
    {
        using type = void;
    };

NS_END  // internal

// strategy_value_type
//   trait: the element type a strategy vends, or void when it is not a
// strategy.
template<typename Type>
struct strategy_value_type
{
    using type = typename internal::strategy_value_type_or_void<Type>::type;
};

// is_byte_strategy
//   trait: true when a strategy vends raw bytes -- the shape an arena-backed
// strategy takes, since an arena has no element type.
template<typename Type>
struct is_byte_strategy
{
    static D_CONSTEXPR const bool value =
        ( is_memory_strategy<Type>::value &&
          internal::byte_like<
              typename strategy_value_type<Type>::type>::value );
};

// is_element_strategy
//   trait: true when a strategy vends storage already shaped for a type --
// the shape a pool-backed or allocator-backed strategy takes.
template<typename Type>
struct is_element_strategy
{
    static D_CONSTEXPR const bool value =
        ( is_memory_strategy<Type>::value &&
          !is_byte_strategy<Type>::value );
};

NS_INTERNAL

    // constant readers
    //   helper: read a descriptive constant, or a stated default when the
    // type does not declare it. Written as a specialization pair rather than
    // a ternary because the ternary would still require the member to name.
    template<typename Type,
             bool Has = has_pointer_stable<Type>::value>
    struct read_pointer_stable
    {
        static D_CONSTEXPR const bool value =
            strategy_clean_t<Type>::pointer_stable;
    };

    template<typename Type>
    struct read_pointer_stable<Type, false>
    {
        static D_CONSTEXPR const bool value = false;
    };

    template<typename Type,
             bool Has = has_individual_release<Type>::value>
    struct read_individual_release
    {
        static D_CONSTEXPR const bool value =
            strategy_clean_t<Type>::supports_individual_release;
    };

    template<typename Type>
    struct read_individual_release<Type, false>
    {
        static D_CONSTEXPR const bool value = false;
    };

    template<typename Type,
             bool Has = has_generational_sweep<Type>::value>
    struct read_generational_sweep
    {
        static D_CONSTEXPR const bool value =
            strategy_clean_t<Type>::supports_generational_sweep;
    };

    template<typename Type>
    struct read_generational_sweep<Type, false>
    {
        static D_CONSTEXPR const bool value = false;
    };

    template<typename Type,
             bool Has = has_storage_kind<Type>::value>
    struct read_storage_kind
    {
        static D_CONSTEXPR const storage_kind value =
            strategy_clean_t<Type>::strategy_storage_kind;
    };

    template<typename Type>
    struct read_storage_kind<Type, false>
    {
        static D_CONSTEXPR const storage_kind value = storage_kind::unknown;
    };

    template<typename Type,
             bool Has = has_extent<Type>::value>
    struct read_extent
    {
        static D_CONSTEXPR const std::size_t value =
            strategy_clean_t<Type>::extent;
    };

    template<typename Type>
    struct read_extent<Type, false>
    {
        static D_CONSTEXPR const std::size_t value = 0;
    };

NS_END  // internal

// strategy_storage_kind_of
//   trait: a strategy's declared storage discipline, or unknown.
template<typename Type>
struct strategy_storage_kind_of
{
    static D_CONSTEXPR const storage_kind value =
        internal::read_storage_kind<Type>::value;
};

// strategy_extent_of
//   trait: a static strategy's compile-time capacity, or 0.
template<typename Type>
struct strategy_extent_of
{
    static D_CONSTEXPR const std::size_t value =
        internal::read_extent<Type>::value;
};

// is_stable_strategy
//   trait: true when storage the strategy has vended never moves. A container
// reads this to decide whether it may hand out interior pointers, or must
// hand out indices instead.
template<typename Type>
struct is_stable_strategy
{
    static D_CONSTEXPR const bool value =
        internal::read_pointer_stable<Type>::value;
};

// is_releasing_strategy
//   trait: true when a single allocation can be returned on its own. False
// for an arena, which reclaims only in bulk -- so a container over one must
// not pretend to erase.
template<typename Type>
struct is_releasing_strategy
{
    static D_CONSTEXPR const bool value =
        internal::read_individual_release<Type>::value;
};

// is_sweeping_strategy
//   trait: true when the strategy's backing store can invalidate outstanding
// references by advancing a generation.
template<typename Type>
struct is_sweeping_strategy
{
    static D_CONSTEXPR const bool value =
        internal::read_generational_sweep<Type>::value;
};

// is_static_strategy / is_fixed_strategy / is_dynamic_strategy
//   trait: the storage discipline, as three booleans for the common tests.
template<typename Type>
struct is_static_strategy
{
    static D_CONSTEXPR const bool value =
        (strategy_storage_kind_of<Type>::value == storage_kind::static_storage);
};

template<typename Type>
struct is_fixed_strategy
{
    static D_CONSTEXPR const bool value =
        (strategy_storage_kind_of<Type>::value == storage_kind::fixed_storage);
};

template<typename Type>
struct is_dynamic_strategy
{
    static D_CONSTEXPR const bool value =
        (strategy_storage_kind_of<Type>::value ==
         storage_kind::dynamic_storage);
};

// is_growable_strategy
//   trait: true when the strategy can obtain more storage on demand. Exactly
// the dynamic case today, named separately because it is the question a
// container actually asks and the answer should not depend on the caller
// remembering which kinds grow.
template<typename Type>
struct is_growable_strategy
{
    static D_CONSTEXPR const bool value = is_dynamic_strategy<Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                       V.   CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A face over the traits above and nothing else. Turning them off changes
// what is CHECKED, never what is computed.

#if (D_INTERNAL_MEM_USE_CONCEPTS == 1) &&                                     \
    defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// memory_strategy_c
//   concept: constrains types satisfying the whole strategy contract.
template<typename Type>
concept memory_strategy_c = is_memory_strategy<Type>::value;

// byte_strategy_c / element_strategy_c
//   concept: the two typing disciplines.
template<typename Type>
concept byte_strategy_c = is_byte_strategy<Type>::value;

template<typename Type>
concept element_strategy_c = is_element_strategy<Type>::value;

// stable_strategy_c / releasing_strategy_c / growable_strategy_c
//   concept: the three capability refinements a container constrains on.
template<typename Type>
concept stable_strategy_c = (memory_strategy_c<Type> &&
                             is_stable_strategy<Type>::value);

template<typename Type>
concept releasing_strategy_c = (memory_strategy_c<Type> &&
                                is_releasing_strategy<Type>::value);

template<typename Type>
concept growable_strategy_c = (memory_strategy_c<Type> &&
                               is_growable_strategy<Type>::value);

#endif  // concepts


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_MEMORY_MEMORY_STRATEGY_COMMON_HPP
