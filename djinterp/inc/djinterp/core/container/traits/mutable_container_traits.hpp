/******************************************************************************
* djinterp [container]                            mutable_container_traits.hpp
*
*   SFINAE structural traits for mutability classification.
*
*   The mutability axis is fully orthogonal to the constexpr axis.
* A container can be:
*     constexpr + immutable   - read-only literal type
*     constexpr + mutable     - relaxed-constexpr (C++14+) mutators
*     runtime   + immutable   - exposes only const accessors
*     runtime   + mutable     - exposes mutating member functions
*
*   "Mutable" here means the container exposes ANY mutating
* operation through its public interface.  Detected mutators:
*     push_back / push_front / pop_back / pop_front
*     insert / emplace / emplace_back / emplace_front
*     erase / clear / resize / assign
*     non-const operator[]  (mutable subscript)
*     non-const data()      (mutable contiguous access)
*     non-const begin()     (mutable iteration handle)
*
*   The presence of ANY of those signals classifies the container
* as mutable; absence of all of them, combined with the presence
* of a const surface (size/data const), classifies it as
* immutable.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/mutable_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_TRAITS_
#define DJINTERP_MUTABLE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method detection
// ===========================================================================
// Each detector is named with a `_signal` suffix to avoid clashing
// with any homonyms elsewhere in the trait family.  A "signal" is a
// single piece of evidence; the umbrella below combines them.

// has_push_back_signal
template<typename _Type,
         typename = void>
struct has_push_back_signal : std::false_type
{};

template<typename _Type>
struct has_push_back_signal<_Type, void_t<
    decltype(std::declval<_Type&>().push_back(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_push_front_signal
template<typename _Type,
         typename = void>
struct has_push_front_signal : std::false_type
{};

template<typename _Type>
struct has_push_front_signal<_Type, void_t<
    decltype(std::declval<_Type&>().push_front(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_insert_signal
template<typename _Type,
         typename = void>
struct has_insert_signal : std::false_type
{};

template<typename _Type>
struct has_insert_signal<_Type, void_t<
    decltype(std::declval<_Type&>().insert(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_erase_signal
template<typename _Type,
         typename = void>
struct has_erase_signal : std::false_type
{};

template<typename _Type>
struct has_erase_signal<_Type, void_t<
    decltype(std::declval<_Type&>().erase(
        std::declval<_Type&>().begin()))
>> : std::true_type
{};

// has_clear_signal
template<typename _Type,
         typename = void>
struct has_clear_signal : std::false_type
{};

template<typename _Type>
struct has_clear_signal<_Type, void_t<
    decltype(std::declval<_Type&>().clear())
>> : std::true_type
{};

// has_resize_signal
template<typename _Type,
         typename = void>
struct has_resize_signal : std::false_type
{};

template<typename _Type>
struct has_resize_signal<_Type, void_t<
    decltype(std::declval<_Type&>().resize(std::size_t{}))
>> : std::true_type
{};

// has_mutable_subscript_signal
//   trait: operator[] returns a non-const lvalue reference.
template<typename _Type,
         typename = void>
struct has_mutable_subscript_signal : std::false_type
{};

template<typename _Type>
struct has_mutable_subscript_signal<_Type, void_t<
    decltype(std::declval<_Type&>()[std::size_t{}] =
             std::declval<typename _Type::value_type>())
>> : std::true_type
{};

// has_mutable_data_signal
//   trait: data() returns a non-const pointer.
template<typename _Type,
         typename = void>
struct has_mutable_data_signal : std::false_type
{};

template<typename _Type>
struct has_mutable_data_signal<_Type, void_t<
    decltype(*std::declval<_Type&>().data() =
             std::declval<typename _Type::value_type>())
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_push_back_signal_v =
        has_push_back_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_push_front_signal_v =
        has_push_front_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_insert_signal_v =
        has_insert_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_erase_signal_v =
        has_erase_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_clear_signal_v =
        has_clear_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_resize_signal_v =
        has_resize_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_mutable_subscript_signal_v =
        has_mutable_subscript_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_mutable_data_signal_v =
        has_mutable_data_signal<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_mutable_container
//   trait: true if any mutator signal is present.
template<typename _Type>
struct is_mutable_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_push_back_signal<clean_type>::value
          || has_push_front_signal<clean_type>::value
          || has_insert_signal<clean_type>::value
          || has_erase_signal<clean_type>::value
          || has_clear_signal<clean_type>::value
          || has_resize_signal<clean_type>::value
          || has_mutable_subscript_signal<clean_type>::value
          || has_mutable_data_signal<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_mutable_container_v =
        is_mutable_container<_Type>::value;
#endif


// is_immutable_container
//   trait: true if the type "looks like a container" (presence of
// a size() accessor) but exposes NONE of the mutation signals.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct has_size_for_immut : std::false_type
    {};

    template<typename _Type>
    struct has_size_for_immut<_Type, void_t<
        decltype(std::declval<const _Type&>().size())
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_immutable_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    internal::has_size_for_immut<clean_type>::value
          && !is_mutable_container<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_immutable_container_v =
        is_immutable_container<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct mutable_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool can_push_back =
        has_push_back_signal<clean_type>::value;
    static constexpr bool can_push_front =
        has_push_front_signal<clean_type>::value;
    static constexpr bool can_insert =
        has_insert_signal<clean_type>::value;
    static constexpr bool can_erase =
        has_erase_signal<clean_type>::value;
    static constexpr bool can_clear =
        has_clear_signal<clean_type>::value;
    static constexpr bool can_resize =
        has_resize_signal<clean_type>::value;
    static constexpr bool is_mutable =
        is_mutable_container<clean_type>::value;
    static constexpr bool is_immutable =
        is_immutable_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_MUTABLE_CONTAINER_TRAITS_