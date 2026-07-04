/******************************************************************************
* djinterp [container]                             ordered_container_traits.hpp
*
*   The ORDER axis, container-side traits: is a container's iteration order a
* meaningful, stable property (is_ordered_container), what KIND of order is it
* (ordering_kind: sequence-position vs sorted-by-key), and - for code that must
* dispatch on physical shape - how is that order laid out in memory
* (sequential_layout: contiguous / linked / tree / hashed).  These are the
* compile-time CLASSIFIERS of the order axis and nothing else; the order
* OPERATIONS (the ordered_base CRTP mixin and the free order algorithms) live in
* container/ordered_container.hpp, which includes this header.  A downstream unit
* that only needs to ask "is this ordered?" pays for the traits alone.
*
*   ordering is intrinsic here: whether position carries meaning, not whether a
* holder is permitted to observe it (that is the access axis,
* read_write_container_traits.hpp).
*
*   PORTABILITY:
*   C++11 baseline.  Each `_v` companion is emitted through the trait_detect
* macros (inline variable on C++17+, variable template on C++14, absent on
* C++11 - the `::value` / `::type` members are always present).
*
*
* path:      /inc/djinterp/core/container/traits/ordered_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#ifndef DJINTERP_ORDERED_CONTAINER_TRAITS_
#define DJINTERP_ORDERED_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                 // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"        // D_TYPE_TRAIT_* detection macros
#include "./iterable_container_traits.hpp"    // is_iterable_container


NS_DJINTERP


// ===========================================================================
// I.   Order-axis traits
// ===========================================================================

NS_INTERNAL

    // has_key_type_helper
    //   helper: detects a `key_type` alias - the associative / keyed tell.  A
    // keyed container has bag or keyed identity (permutation-invariant), so its
    // presence marks a container as UNORDERED in the positional sense.
    template<typename _Type,
             typename = void>
    struct has_key_type_helper : std::false_type
    {};

    template<typename _Type>
    struct has_key_type_helper<_Type,
        D_VOID_T<typename clean_t<_Type>::key_type>>
        : std::true_type
    {};

NS_END  // internal

// is_ordered_container
//   trait: true iff the container has positional identity - it is iterable and
// carries no associative key_type, so it is a positional sequence (e_1, ..., e_n).
template<typename _Type>
struct is_ordered_container
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Type>>::value
         && !internal::has_key_type_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_ordered_container)

// is_unordered_container
//   trait: true iff the container is iterable but permutation-invariant - a
// keyed / associative container, whose identity is its bag, not any order.
template<typename _Type>
struct is_unordered_container
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_Type>>::value
         && internal::has_key_type_helper<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_unordered_container)

// ordering_kind
//   enum: a container's position on the order axis.
enum class ordering_kind
{
    non_container,  // not an (iterable) container
    unordered,      // permutation-invariant (associative); no positional order
    ordered         // positional identity (a sequence)
};

// ordering_kind_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
ordering_kind_name(ordering_kind _k) noexcept
{
    return ( _k == ordering_kind::non_container ? "non_container"
           : _k == ordering_kind::unordered     ? "unordered"
           :                                       "ordered" );
}

// ordering_kind_of
//   trait: classifies a type - non_container when not iterable, unordered when
// keyed / associative, else ordered.
template<typename _Type>
struct ordering_kind_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr ordering_kind value =
        ( !is_iterable_container<clean_type>::value )
              ? ordering_kind::non_container
      : (  internal::has_key_type_helper<clean_type>::value )
              ? ordering_kind::unordered
      :         ordering_kind::ordered;

    using type = std::integral_constant<ordering_kind, value>;
};

template<typename _Type>
using ordering_kind_of_t = typename ordering_kind_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr ordering_kind ordering_kind_of_v =
        ordering_kind_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr ordering_kind ordering_kind_of_v =
        ordering_kind_of<_Type>::value;
#endif


// ---------------------------------------------------------------------------
//  sequential layout (the storage shape of an ordered container)
// ---------------------------------------------------------------------------

NS_INTERNAL

    // has_c_str_helper
    //   helper: detects a c_str() accessor - the mark of a text buffer.
    template<typename _Type,
             typename = void>
    struct has_c_str_helper : std::false_type
    {};

    template<typename _Type>
    struct has_c_str_helper<_Type,
        D_VOID_T<decltype(std::declval<const clean_t<_Type>&>().c_str())>>
        : std::true_type
    {};

NS_END  // internal

// sequential_layout
//   enum: the storage shape of an ordered container, as its iterator category and
// storage accessors reveal it.  This is a REPRESENTATION classification - the
// Order axis proper abstracts backing away - offered for code that must dispatch
// on storage shape.
enum class sequential_layout
{
    none,               // not an ordered container
    array_like,         // contiguous, random-access (array, vector)
    deque_like,         // random-access, non-contiguous (deque)
    list_like,          // bidirectional, not random-access (list)
    forward_list_like,  // forward only (forward_list)
    string_like         // a text buffer (has c_str())
};

// sequential_layout_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
sequential_layout_name(sequential_layout _l) noexcept
{
    return ( _l == sequential_layout::none              ? "none"
           : _l == sequential_layout::array_like        ? "array_like"
           : _l == sequential_layout::deque_like        ? "deque_like"
           : _l == sequential_layout::list_like         ? "list_like"
           : _l == sequential_layout::forward_list_like ? "forward_list_like"
           :                                              "string_like" );
}

// sequential_layout_of
//   trait: classifies an ordered container by storage shape.  Only an ordered
// container has a layout; the checks run strongest-category-first, so a deque
// (random-access, no data()) is not mistaken for a list, nor a text buffer for a
// bare array.
template<typename _Type>
struct sequential_layout_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr sequential_layout value =
        ( !is_ordered_container<clean_type>::value )
              ? sequential_layout::none
      : (  internal::has_c_str_helper<clean_type>::value )
              ? sequential_layout::string_like
      : (  is_contiguous_iterable<clean_type>::value )
              ? sequential_layout::array_like
      : (  is_random_access_iterable<clean_type>::value )
              ? sequential_layout::deque_like
      : (  is_bidirectional_iterable<clean_type>::value )
              ? sequential_layout::list_like
      : (  is_forward_iterable<clean_type>::value )
              ? sequential_layout::forward_list_like
      :         sequential_layout::none;

    using type = std::integral_constant<sequential_layout, value>;
};

template<typename _Type>
using sequential_layout_of_t = typename sequential_layout_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr sequential_layout sequential_layout_of_v =
        sequential_layout_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr sequential_layout sequential_layout_of_v =
        sequential_layout_of<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_ORDERED_CONTAINER_TRAITS_
