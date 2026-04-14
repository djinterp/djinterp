/******************************************************************************
* djinterp [container]                                   iterator_traits.hpp
*
* Container-level iterator classification traits.
*   Provides SFINAE-based compile-time detection of iterator properties
* at both the iterator level (what kind of iterator is it?) and the
* container level (what kind of iterators does it provide?).
*
*   Iterator-level traits delegate to the legacy iterator traits from
* cpp_named11.hpp (djinterp::traits::is_legacy_*_iterator) where
* possible, and supplement them with container-specific detection for
* begin()/end(), cbegin()/cend(), rbegin()/rend(), and constexpr
* iteration.
*
*   Container-level iterability traits combine iterator detection with
* begin/end availability to answer "can I iterate this container with
* at least X-category iterators?"
*
* TABLE OF CONTENTS
* =================
* I.      Iterator-Level Traits
* II.     Iterator Category Extraction
* III.    Container-Level Iterability
* IV.     Const / Reverse / Constexpr Iteration Detection
* V.      Iterator Compatibility
* VI.     Convenience Aliases
* VII.    Combined Classification
*
*
* path:      /inc/container/meta/iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2025.05.20
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ITERATOR_TRAITS_
#define DJINTERP_CONTAINER_ITERATOR_TRAITS_ 1

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "../djinterp.hpp"
#include "../type_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Iterator-Level Traits
// =============================================================================
// SFINAE detection of iterator category by structural
// probing.  These are the container module's own detectors
// that do not depend on cpp_named*.hpp — they work in any
// C++11+ context.  For named-requirement-compliant
// detection, prefer djinterp::traits::is_legacy_*_iterator
// from cpp_named11.hpp.

// is_input_iterator
//   type trait: true if _Type satisfies the structural
// requirements of an InputIterator: has the five nested
// types, supports dereference, pre/post-increment, and
// equality/inequality comparison.
template<typename _Type,
         typename = void>
struct is_input_iterator : std::false_type
{};

template<typename _Type>
struct is_input_iterator<_Type,
    std::void_t<
        typename _Type::value_type,
        typename _Type::difference_type,
        typename _Type::pointer,
        typename _Type::reference,
        typename _Type::iterator_category,
        decltype(++std::declval<_Type&>()),
        decltype(std::declval<_Type&>()++),
        decltype(*std::declval<_Type&>()),
        decltype(std::declval<const _Type&>() ==
                 std::declval<const _Type&>()),
        decltype(std::declval<const _Type&>() !=
                 std::declval<const _Type&>())
    >
> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_input_iterator_v = is_input_iterator<_Type>::value;

// is_output_iterator
//   type trait: true if _Type supports assignment through
// dereference and pre/post-increment, and its category
// derives from output_iterator_tag.
template<typename _Type,
         typename = void>
struct is_output_iterator : std::false_type
{};

template<typename _Type>
struct is_output_iterator<_Type,
    std::void_t<
        decltype(*std::declval<_Type&>() =
            std::declval<typename std::iterator_traits<_Type>::value_type>()),
        decltype(++std::declval<_Type&>()),
        decltype(std::declval<_Type&>()++)
    >> : std::is_base_of<
             std::output_iterator_tag,
             typename std::iterator_traits<_Type>::iterator_category>
{};

template<typename _Type>
inline constexpr bool is_output_iterator_v =
    is_output_iterator<_Type>::value;

// is_forward_iterator
//   type trait: true if _Type is default-constructible,
// has the standard nested types, and its category derives
// from forward_iterator_tag.
template<typename _Type,
         typename = void>
struct is_forward_iterator : std::false_type
{};

template<typename _Type>
struct is_forward_iterator<_Type,
    std::void_t<
        typename std::iterator_traits<_Type>::value_type,
        typename std::iterator_traits<_Type>::difference_type,
        typename std::iterator_traits<_Type>::reference,
        typename std::iterator_traits<_Type>::pointer,
        decltype(_Type())>
> : std::is_base_of<std::forward_iterator_tag,
                    typename std::iterator_traits<_Type>::iterator_category>
{};

template<typename _Type>
inline constexpr bool is_forward_iterator_v =
    is_forward_iterator<_Type>::value;

// is_bidirectional_iterator
//   type trait: true if _Type supports pre/post-decrement
// and its category derives from bidirectional_iterator_tag.
template<typename _Type,
         typename = void>
struct is_bidirectional_iterator : std::false_type
{};

template<typename _Type>
struct is_bidirectional_iterator<_Type,
    std::void_t<
        decltype(--std::declval<_Type&>()),
        decltype(std::declval<_Type&>()--)
    >> : std::is_base_of<
             std::bidirectional_iterator_tag,
             typename std::iterator_traits<
                 _Type>::iterator_category>
{};

template<typename _Type>
inline constexpr bool is_bidirectional_iterator_v =
    is_bidirectional_iterator<_Type>::value;

// is_random_access_iterator
//   type trait: true if _Type supports +=, -=, +, -, [],
// and relational comparisons, and its category derives
// from random_access_iterator_tag.
template<typename _Type,
         typename = void>
struct is_random_access_iterator : std::false_type
{};

template<typename _Type>
struct is_random_access_iterator<_Type,
    std::void_t<
        decltype(std::declval<_Type&>() +=
            std::declval<
                typename std::iterator_traits<
                    _Type>::difference_type>()),
        decltype(std::declval<_Type&>() -=
            std::declval<
                typename std::iterator_traits<
                    _Type>::difference_type>()),
        decltype(std::declval<const _Type&>() +
            std::declval<
                typename std::iterator_traits<
                    _Type>::difference_type>()),
        decltype(std::declval<const _Type&>() -
            std::declval<
                typename std::iterator_traits<
                    _Type>::difference_type>()),
        decltype(std::declval<const _Type&>() -
            std::declval<const _Type&>()),
        decltype(std::declval<const _Type&>()[
            std::declval<
                typename std::iterator_traits<
                    _Type>::difference_type>()]),
        decltype(std::declval<const _Type&>() <
            std::declval<const _Type&>()),
        decltype(std::declval<const _Type&>() >=
            std::declval<const _Type&>())
    >> : std::is_base_of<
             std::random_access_iterator_tag,
             typename std::iterator_traits<
                 _Type>::iterator_category>
{};

template<typename _Type>
inline constexpr bool is_random_access_iterator_v =
    is_random_access_iterator<_Type>::value;

// is_contiguous_iterator
//   type trait: true if _Type is a random-access iterator
// over contiguous memory.  Raw pointers always qualify.
// Class iterators require contiguous_iterator_tag (C++20).
template<typename _Type>
struct is_contiguous_iterator
{
    static constexpr bool value =
        ( is_random_access_iterator_v<_Type> &&
          std::is_pointer_v<_Type> );
};

// C++20: also check contiguous_iterator_tag
#if (__cplusplus >= 202002L)

NS_INTERNAL

    template<typename _I, typename = void>
    struct has_contiguous_tag : std::false_type
    {};

    template<typename _I>
    struct has_contiguous_tag<_I,
        std::enable_if_t<std::is_base_of_v<
            std::contiguous_iterator_tag,
            typename std::iterator_traits<
                _I>::iterator_category>>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_contiguous_iterator
{
    static constexpr bool value =
        ( is_random_access_iterator_v<_Type> &&
          ( std::is_pointer_v<_Type> ||
            internal::has_contiguous_tag<
                _Type>::value ) );
};

#endif  // C++20

template<typename _Type>
inline constexpr bool is_contiguous_iterator_v =
    is_contiguous_iterator<_Type>::value;


// =============================================================================
// II.  Iterator Category Extraction
// =============================================================================

NS_INTERNAL

    template<typename _Iter, typename = void>
    struct safe_iterator_category
    {
        using type = void;
    };

    template<typename _Iter>
    struct safe_iterator_category<_Iter,
        std::void_t<
            typename std::iterator_traits<
                _Iter>::iterator_category>>
    {
        using type =
            typename std::iterator_traits<
                _Iter>::iterator_category;
    };

    template<typename _Iter>
    using safe_iterator_category_t =
        typename safe_iterator_category<_Iter>::type;

    // safe_begin_iterator: extracts the iterator type
    // from begin(), or void.
    template<typename _C, typename = void>
    struct safe_begin_iterator
    {
        using type = void;
    };

    template<typename _C>
    struct safe_begin_iterator<_C,
        std::void_t<decltype(
            std::begin(std::declval<_C&>()))>>
    {
        using type = decltype(
            std::begin(std::declval<_C&>()));
    };

    template<typename _C>
    using safe_begin_iterator_t =
        typename safe_begin_iterator<_C>::type;

NS_END  // internal

// iterator_category_of
//   type trait: extracts the iterator category of a
// container's begin() iterator, yielding void if
// unavailable.
template<typename _Container>
struct iterator_category_of
{
    using clean_type = clean_t<_Container>;
    using iter_type  =
        internal::safe_begin_iterator_t<clean_type>;

    using type =
        internal::safe_iterator_category_t<iter_type>;
};

template<typename _Container>
using iterator_category_of_t =
    typename iterator_category_of<_Container>::type;


// =============================================================================
// III. Container-Level Iterability
// =============================================================================
// Determines the strongest iterator category a container
// provides through its begin()/end() interface.

// is_iterable
//   type trait: true if begin(c) and end(c) are
// well-formed.
template<typename _Type,
         typename = void>
struct is_iterable : std::false_type
{};

template<typename _Type>
struct is_iterable<_Type,
    std::void_t<
        decltype(std::begin(std::declval<_Type&>())),
        decltype(std::end(std::declval<_Type&>()))
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_iterable_v =
    is_iterable<_Type>::value;

// is_input_iterable
//   type trait: true if container provides at least
// input iterators.
template<typename _Type,
         typename = void>
struct is_input_iterable : std::false_type
{};

template<typename _Type>
struct is_input_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_input_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_input_iterable_v =
    is_input_iterable<_Type>::value;

// is_output_iterable
//   type trait: true if container provides output
// iterators.
template<typename _Type,
         typename = void>
struct is_output_iterable : std::false_type
{};

template<typename _Type>
struct is_output_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_output_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_output_iterable_v =
    is_output_iterable<_Type>::value;

// is_forward_iterable
//   type trait: true if container provides at least
// forward iterators.
template<typename _Type,
         typename = void>
struct is_forward_iterable : std::false_type
{};

template<typename _Type>
struct is_forward_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_forward_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_forward_iterable_v =
    is_forward_iterable<_Type>::value;

// is_bidirectional_iterable
//   type trait: true if container provides bidirectional
// iterators.
template<typename _Type,
         typename = void>
struct is_bidirectional_iterable : std::false_type
{};

template<typename _Type>
struct is_bidirectional_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_bidirectional_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_bidirectional_iterable_v =
    is_bidirectional_iterable<_Type>::value;

// is_random_access_iterable
//   type trait: true if container provides random-access
// iterators.
template<typename _Type,
         typename = void>
struct is_random_access_iterable : std::false_type
{};

template<typename _Type>
struct is_random_access_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_random_access_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_random_access_iterable_v =
    is_random_access_iterable<_Type>::value;

// is_contiguous_iterable
//   type trait: true if container provides contiguous
// iterators (raw pointer or contiguous_iterator_tag).
template<typename _Type,
         typename = void>
struct is_contiguous_iterable : std::false_type
{};

template<typename _Type>
struct is_contiguous_iterable<_Type,
    std::enable_if_t<
        is_iterable_v<_Type> &&
        is_contiguous_iterator_v<
            internal::safe_begin_iterator_t<_Type>>
    >> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_contiguous_iterable_v =
    is_contiguous_iterable<_Type>::value;


// =============================================================================
// IV.  Const / Reverse / Constexpr Iteration Detection
// =============================================================================

// --- const iteration ---

D_TYPE_TRAIT_TRUE(has_cbegin,
    decltype(std::declval<const _Type&>().cbegin()))

D_TYPE_TRAIT_TRUE(has_cend,
    decltype(std::declval<const _Type&>().cend()))

// has_const_iteration
//   type trait: true if container supports const iteration
// via cbegin()/cend().
template<typename _Type>
struct has_const_iteration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_cbegin_v<clean_type> &&
          has_cend_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_const_iteration_v =
    has_const_iteration<_Type>::value;

// --- reverse iteration ---

D_TYPE_TRAIT_TRUE(has_rbegin,
    decltype(std::declval<_Type&>().rbegin()))

D_TYPE_TRAIT_TRUE(has_rend,
    decltype(std::declval<_Type&>().rend()))

D_TYPE_TRAIT_TRUE(has_crbegin,
    decltype(std::declval<const _Type&>().crbegin()))

D_TYPE_TRAIT_TRUE(has_crend,
    decltype(std::declval<const _Type&>().crend()))

// has_reverse_iteration
//   type trait: true if container supports reverse
// iteration via rbegin()/rend().
template<typename _Type>
struct has_reverse_iteration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_rbegin_v<clean_type> &&
          has_rend_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_reverse_iteration_v =
    has_reverse_iteration<_Type>::value;

// has_const_reverse_iteration
//   type trait: true if container supports const reverse
// iteration via crbegin()/crend().
template<typename _Type>
struct has_const_reverse_iteration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_crbegin_v<clean_type> &&
          has_crend_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_const_reverse_iteration_v =
    has_const_reverse_iteration<_Type>::value;

// --- constexpr iteration detection ---
// framework containers may expose constexpr_begin() /
// constexpr_end() for compile-time iteration, or expose
// a constexpr_iterator nested type.

D_TYPE_TRAIT_TRUE(has_constexpr_iterator_type,
    typename _Type::constexpr_iterator)

D_TYPE_TRAIT_TRUE(has_constexpr_begin,
    decltype(std::declval<const _Type&>()
        .constexpr_begin()))

D_TYPE_TRAIT_TRUE(has_constexpr_end,
    decltype(std::declval<const _Type&>()
        .constexpr_end()))

// has_constexpr_iteration
//   type trait: true if container supports compile-time
// constexpr iteration.
template<typename _Type>
struct has_constexpr_iteration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_constexpr_iterator_type_v<clean_type> ||
          ( has_constexpr_begin_v<clean_type> &&
            has_constexpr_end_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool has_constexpr_iteration_v =
    has_constexpr_iteration<_Type>::value;


// =============================================================================
// V.   Iterator Compatibility
// =============================================================================

// iterators_compatible
//   type trait: true if two containers provide iterators
// over the same value_type.
template<typename _A,
         typename _B,
         typename = void>
struct iterators_compatible : std::false_type
{};

template<typename _A,
         typename _B>
struct iterators_compatible<_A, _B,
    std::void_t<
        typename _A::value_type,
        typename _B::value_type
    >> : std::is_same<
             typename _A::value_type,
             typename _B::value_type>
{};

template<typename _A,
         typename _B>
inline constexpr bool iterators_compatible_v =
    iterators_compatible<_A, _B>::value;


// =============================================================================
// VI.  DIteratorLevel Enum
// =============================================================================

// DIteratorLevel
//   enum: classifies the strongest iterator category a
// container provides.
enum class DIteratorLevel
{
    none,
    input,
    output,
    forward,
    bidirectional,
    random_access,
    contiguous
};

NS_INTERNAL

    template<typename _Type>
    struct iterator_level_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DIteratorLevel value =
            is_contiguous_iterable_v<clean_type>
                ? DIteratorLevel::contiguous

            : is_random_access_iterable_v<clean_type>
                ? DIteratorLevel::random_access

            : is_bidirectional_iterable_v<clean_type>
                ? DIteratorLevel::bidirectional

            : is_forward_iterable_v<clean_type>
                ? DIteratorLevel::forward

            : is_output_iterable_v<clean_type>
                ? DIteratorLevel::output

            : is_input_iterable_v<clean_type>
                ? DIteratorLevel::input

            : DIteratorLevel::none;
    };

NS_END  // internal

// container_iterator_level
//   type trait: determines the strongest iterator category
// the container provides.
template<typename _Type>
struct container_iterator_level
{
    static constexpr DIteratorLevel value =
        internal::iterator_level_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DIteratorLevel
    container_iterator_level_v =
        container_iterator_level<_Type>::value;


// =============================================================================
// VII. Combined Classification
// =============================================================================

// container_iterator_class
//   struct: complete iterator classification of a container
// type.
template<typename _Type>
struct container_iterator_class
{
    // iterability by category
    static constexpr bool is_iterable =
        is_iterable_v<_Type>;
    static constexpr bool input_iterable =
        is_input_iterable_v<_Type>;
    static constexpr bool output_iterable =
        is_output_iterable_v<_Type>;
    static constexpr bool forward_iterable =
        is_forward_iterable_v<_Type>;
    static constexpr bool bidirectional_iterable =
        is_bidirectional_iterable_v<_Type>;
    static constexpr bool random_access_iterable =
        is_random_access_iterable_v<_Type>;
    static constexpr bool contiguous_iterable =
        is_contiguous_iterable_v<_Type>;

    // iteration variants
    static constexpr bool has_const_iter =
        has_const_iteration_v<_Type>;
    static constexpr bool has_reverse_iter =
        has_reverse_iteration_v<_Type>;
    static constexpr bool has_const_reverse_iter =
        has_const_reverse_iteration_v<_Type>;
    static constexpr bool has_constexpr_iter =
        has_constexpr_iteration_v<_Type>;

    // level
    static constexpr DIteratorLevel level =
        container_iterator_level_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ITERATOR_TRAITS_
