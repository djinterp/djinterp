/*******************************************************************************
* djinterp [core]                                            iterator_traits.hpp
*
* djinterp iterator_traits.hpp
*
* Container-level iterator classification traits.
*   Provides SFINAE-based compile-time detection of iterator properties
* at both the iterator level (what kind of iterator is it?) and the
* container level (what kind of iterators does it provide?).
*
*   Iterator-level traits supplement the standard iterator_traits with
* container-specific detection for begin()/end(), cbegin()/cend(),
* rbegin()/rend(), and constexpr iteration.
*
*   Container-level iterability traits combine iterator detection with
* begin/end availability to answer "can I iterate this container with
* at least X-category iterators?"
*
*   DIVISION OF RESPONSIBILITY:
*   The container-level CATEGORY traits - is_forward_iterable,
* is_bidirectional_iterable, is_random_access_iterable,
* is_contiguous_iterable, and iterator_category_of - are owned by
* iterator_category_traits.hpp and re-exported here via the include below.
* Earlier revisions defined them in this header as well; the duplicate
* definitions caused one-definition-rule conflicts in any translation unit
* that pulled both headers (e.g. container_filter_traits.hpp, which reaches
* iterator_traits.hpp through constexpr_container_traits.hpp and also
* includes iterator_category_traits.hpp directly).  This header retains only
* the iterator-LEVEL traits, the begin()/category extraction helpers, the
* base/input/output iterability probes, const/reverse detection, iterator
* compatibility, and the strongest-category summary.
*
*   PORTABILITY:
*   C++11 baseline.  `_v` variable templates are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES (C++14+).  The contiguous-
* iterator detection probes std::contiguous_iterator_tag only on
* C++20+.
*
*
* path:      /inc/djinterp/core/container/iterator/iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Iterator-Level Traits
      ---------------------

II.   Iterator Category Extraction (helpers; public category trait delegated)
      -----------------------------------------------------------------------

III.  Container-Level Iterability (base/input/output; category traits
      ---------------------------------------------------------------

      delegated)

IV.   Const / Reverse Iteration Detection
      -----------------------------------

V.    Iterator Compatibility
      ----------------------

VI.   iterator_level Enum
      -------------------

VII.  Combined Classification
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_ITERATOR_ITERATOR_TRAITS_HPP
#define DJINTERP_CONTAINER_ITERATOR_ITERATOR_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/trait_detect.hpp"
#include "../../meta/type_traits.hpp"
#include "./iterator_category_traits.hpp"  // container-level is_*_iterable,
                                           // iterator_category_of (canonical
                                           // home)


NS_DJINTERP

// ===========================================================================
// I.   Iterator-Level Traits
// ===========================================================================
// SFINAE detection of iterator category by structural
// probing.  Each trait yields std::true_type or
// std::false_type and is paired with a `_v` variable
// template alias when variable templates are available.

// is_input_iterator
//   trait: true if Type satisfies the structural
// requirements of an InputIterator: has the five nested types, supports
// dereference, pre/post-increment, and equality/inequality comparison.
template<typename Type,
         typename = void>
struct is_input_iterator : std::false_type
{};

template<typename Type>
struct is_input_iterator<Type, void_t<
    typename Type::value_type,
    typename Type::difference_type,
    typename Type::pointer,
    typename Type::reference,
    typename Type::iterator_category,
    decltype(++std::declval<Type&>()),
    decltype(std::declval<Type&>()++),
    decltype(*std::declval<Type&>()),
    decltype(std::declval<const Type&>() ==
             std::declval<const Type&>()),
    decltype(std::declval<const Type&>() !=
             std::declval<const Type&>())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_input_iterator_v
    //   variable template: value of is_input_iterator<Type>.
    template<typename Type>
    constexpr bool is_input_iterator_v =
        is_input_iterator<Type>::value;
#endif

// is_output_iterator
//   trait: true if Type supports assignment through dereference and
// pre/post-increment, and its category derives from output_iterator_tag.
template<typename Type,
         typename = void>
struct is_output_iterator : std::false_type
{};

template<typename Type>
struct is_output_iterator<Type, void_t<
    decltype(*std::declval<Type&>() =
        std::declval<typename
            std::iterator_traits<Type>::value_type>()),
    decltype(++std::declval<Type&>()),
    decltype(std::declval<Type&>()++)
>> : std::is_base_of<
         std::output_iterator_tag,
         typename std::iterator_traits<
             Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_output_iterator_v
    //   variable template: value of is_output_iterator<Type>.
    template<typename Type>
    constexpr bool is_output_iterator_v =
        is_output_iterator<Type>::value;
#endif

// is_forward_iterator
//   trait: true if Type is default-constructible, has the standard nested
// types, and its category derives from forward_iterator_tag.
template<typename Type,
         typename = void>
struct is_forward_iterator : std::false_type
{};

template<typename Type>
struct is_forward_iterator<Type, void_t<
    typename std::iterator_traits<Type>::value_type,
    typename std::iterator_traits<Type>::difference_type,
    typename std::iterator_traits<Type>::reference,
    typename std::iterator_traits<Type>::pointer,
    decltype(Type())
>> : std::is_base_of<
         std::forward_iterator_tag,
         typename std::iterator_traits<
             Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_forward_iterator_v
    //   variable template: value of is_forward_iterator<Type>.
    template<typename Type>
    constexpr bool is_forward_iterator_v =
        is_forward_iterator<Type>::value;
#endif

// is_bidirectional_iterator
//   trait: true if Type supports pre/post-decrement and its category derives
// from bidirectional_iterator_tag.
template<typename Type,
         typename = void>
struct is_bidirectional_iterator : std::false_type
{};

template<typename Type>
struct is_bidirectional_iterator<Type, void_t<
    decltype(--std::declval<Type&>()),
    decltype(std::declval<Type&>()--)
>> : std::is_base_of<
         std::bidirectional_iterator_tag,
         typename std::iterator_traits<
             Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_bidirectional_iterator_v
    //   variable template: value of is_bidirectional_iterator<Type>.
    template<typename Type>
    constexpr bool is_bidirectional_iterator_v =
        is_bidirectional_iterator<Type>::value;
#endif

// is_random_access_iterator
//   trait: true if Type supports +=, -=, +, -, [], and
// relational comparisons, and its category derives from
// random_access_iterator_tag.
template<typename Type,
         typename = void>
struct is_random_access_iterator : std::false_type
{};

template<typename Type>
struct is_random_access_iterator<Type, void_t<
    decltype(std::declval<Type&>() +=
        std::declval<typename std::iterator_traits<
            Type>::difference_type>()),
    decltype(std::declval<Type&>() -=
        std::declval<typename std::iterator_traits<
            Type>::difference_type>()),
    decltype(std::declval<const Type&>() +
        std::declval<typename std::iterator_traits<
            Type>::difference_type>()),
    decltype(std::declval<const Type&>() -
        std::declval<typename std::iterator_traits<
            Type>::difference_type>()),
    decltype(std::declval<const Type&>() -
        std::declval<const Type&>()),
    decltype(std::declval<const Type&>()[
        std::declval<typename std::iterator_traits<
            Type>::difference_type>()]),
    decltype(std::declval<const Type&>() <
        std::declval<const Type&>()),
    decltype(std::declval<const Type&>() >=
        std::declval<const Type&>())
>> : std::is_base_of<
         std::random_access_iterator_tag,
         typename std::iterator_traits<
             Type>::iterator_category>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_random_access_iterator_v
    //   variable template: value of is_random_access_iterator<Type>.
    template<typename Type>
    constexpr bool is_random_access_iterator_v =
        is_random_access_iterator<Type>::value;
#endif

// is_contiguous_iterator
//   trait: true if Type is a random-access iterator over contiguous memory.
// Raw pointers always qualify; class iterators must additionally carry
// contiguous_iterator_tag (C++20).
NS_INTERNAL

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // has_contiguous_tag
    //   trait: detects whether the iterator's category is (or derives from)
    // std::contiguous_iterator_tag.
    template<typename Iter,
             typename = void>
    struct has_contiguous_tag : std::false_type
    {};

    // has_contiguous_tag<Iter, typename std::enable_if<std::is_base_of<
    // std::contiguous_iterator_tag, typename std::iterator_traits< Iter>
    //   trait: the `typename std::enable_if<std::is_base_of<
    // std::contiguous_iterator_tag, typename std::iterator_traits< Iter`
    // case; it reports true.
    template<typename Iter>
    struct has_contiguous_tag<Iter,
        typename std::enable_if<std::is_base_of<
            std::contiguous_iterator_tag,
            typename std::iterator_traits<
                Iter>::iterator_category>::value
        >::type> : std::true_type
    {};
#else
    // pre-C++20 fallback: contiguous_iterator_tag does not exist, so only raw
    // pointers qualify.
    template<typename Iter>
    struct has_contiguous_tag : std::false_type
    {};
#endif

NS_END  // internal

template<typename Type>
struct is_contiguous_iterator
{
    static constexpr bool value =
        ( is_random_access_iterator<Type>::value          &&
          ( std::is_pointer<Type>::value                  ||
            internal::has_contiguous_tag<Type>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_contiguous_iterator_v
    //   variable template: value of is_contiguous_iterator<Type>.
    template<typename Type>
    constexpr bool is_contiguous_iterator_v =
        is_contiguous_iterator<Type>::value;
#endif


// ===========================================================================
// II.  Iterator Category Extraction
// ===========================================================================
//   The internal helpers below extract a container's begin() iterator type and
// that iterator's std category tag.  They back the input/output iterability
// probes in section III and remain the low-level primitive for anyone needing
// the raw tag TYPE of a container's iterator
// (internal::safe_iterator_category_t
// applied to internal::safe_begin_iterator_t).
//
//   The PUBLIC container-category trait `iterator_category_of` is owned by
// iterator_category_traits.hpp (included above) and re-exported through it.
// That version classifies a container into an `iterator_category_kind` enum;
// an earlier revision here exposed a same-named trait yielding the raw std tag
// type instead, which collided with the canonical one.  It has been removed to
// end the one-definition-rule conflict.

NS_INTERNAL

    // safe_iterator_category
    //   helper: extracts iterator_category, or void on mismatch.
    template<typename Iter,
             typename = void>
    struct safe_iterator_category
    {
        using type = void;
    };

    // safe_iterator_category<Iter, void_t< typename std::iterator_traits<
    // Iter>
    //   trait: the `void_t< typename std::iterator_traits< Iter` case; it
    // maps to `typename std::iterator_traits< Iter>::iterator_category`.
    template<typename Iter>
    struct safe_iterator_category<Iter, void_t<
        typename std::iterator_traits<
            Iter>::iterator_category>>
    {
        using type = typename std::iterator_traits<
            Iter>::iterator_category;
    };

    template<typename Iter>
    using safe_iterator_category_t =
        typename safe_iterator_category<Iter>::type;

    // safe_begin_iterator
    //   helper: extracts the iterator type from begin(), or void.
    template<typename C,
             typename = void>
    struct safe_begin_iterator
    {
        using type = void;
    };

    // safe_begin_iterator<C, void_t<
    // decltype(std::begin(std::declval<C&>())) >>
    //   trait: the `void_t< decltype(std::begin(std::declval<C&>())) >` case;
    // it maps to `decltype(std::begin(std::declval<C&>()))`.
    template<typename C>
    struct safe_begin_iterator<C, void_t<
        decltype(std::begin(std::declval<C&>()))
    >>
    {
        using type =
            decltype(std::begin(std::declval<C&>()));
    };

    template<typename C>
    using safe_begin_iterator_t =
        typename safe_begin_iterator<C>::type;

NS_END  // internal


// ===========================================================================
// III. Container-Level Iterability
// ===========================================================================
//   This section owns the base iterability probe (is_iterable) and the
// input/output rungs, which gate on the iterator-LEVEL traits from section I.
// The stronger CATEGORY rungs - is_forward_iterable, is_bidirectional_iterable,
// is_random_access_iterable, is_contiguous_iterable - are owned by
// iterator_category_traits.hpp (included above) and re-exported through it.
// Earlier revisions defined those four here as well; the duplicate definitions
// caused one-definition-rule conflicts wherever both headers were pulled into
// the same TU.  The canonical versions read the category of the container's
// begin() iterator through std::iterator_traits (so a pointer iterator resolves
// to random-access) and gate on member begin()/end(); they agree with the
// probes below for every class-type container that exposes begin()/end().

// is_iterable
//   trait: true if begin(c) and end(c) are well-formed.
template<typename Type,
         typename = void>
struct is_iterable : std::false_type
{};

// is_iterable<Type, void_t< decltype(std::begin(std::declval<Type&>())),
// decltype(std::end(std::declval<Type&>())) >>
//   trait: the `void_t< decltype(std::begin(std::declval<Type&>())),
// decltype(std::end(std::declval<Type&>())) >` case; it reports true.
template<typename Type>
struct is_iterable<Type, void_t<
    decltype(std::begin(std::declval<Type&>())),
    decltype(std::end(std::declval<Type&>()))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_iterable_v
    //   variable template: value of is_iterable<Type>.
    template<typename Type>
    constexpr bool is_iterable_v = is_iterable<Type>::value;
#endif

// is_input_iterable
//   trait: true if container provides at least input iterators.
template<typename Type,
         typename = void>
struct is_input_iterable : std::false_type
{};

// is_input_iterable<Type, typename std::enable_if< is_iterable<Type>
//   trait: the `typename std::enable_if< is_iterable<Type` case; it reports
// true.
template<typename Type>
struct is_input_iterable<Type,
    typename std::enable_if<
        is_iterable<Type>::value  &&
        is_input_iterator<
            internal::safe_begin_iterator_t<Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_input_iterable_v
    //   variable template: value of is_input_iterable<Type>.
    template<typename Type>
    constexpr bool is_input_iterable_v =
        is_input_iterable<Type>::value;
#endif

// is_output_iterable
//   trait: true if container provides output iterators.
template<typename Type,
         typename = void>
struct is_output_iterable : std::false_type
{};

// is_output_iterable<Type, typename std::enable_if< is_iterable<Type>
//   trait: the `typename std::enable_if< is_iterable<Type` case; it reports
// true.
template<typename Type>
struct is_output_iterable<Type,
    typename std::enable_if<
        is_iterable<Type>::value  &&
        is_output_iterator<
            internal::safe_begin_iterator_t<Type>>::value
    >::type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_output_iterable_v
    //   variable template: value of is_output_iterable<Type>.
    template<typename Type>
    constexpr bool is_output_iterable_v =
        is_output_iterable<Type>::value;
#endif

// is_forward_iterable, is_bidirectional_iterable, is_random_access_iterable,
// is_contiguous_iterable
//   These container-level category traits are owned by
// iterator_category_traits.hpp (included above) and re-exported through it.
// See the section III banner comment; the duplicates that used to live here
// were removed to resolve the ODR conflict.


// ===========================================================================
// IV.  Const / Reverse Iteration Detection
// ===========================================================================

// --- const iteration ---
D_TYPE_TRAIT_TRUE(has_cbegin,
    decltype(std::declval<const Type&>().cbegin()))

D_TYPE_TRAIT_TRUE(has_cend,
    decltype(std::declval<const Type&>().cend()))

// has_const_iteration
//   trait: true if container supports const iteration via cbegin()/cend().
template<typename Type>
struct has_const_iteration
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_cbegin<cleaned>::value  &&
          has_cend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_const_iteration_v
    //   variable template: value of has_const_iteration<Type>.
    template<typename Type>
    constexpr bool has_const_iteration_v =
        has_const_iteration<Type>::value;
#endif

// --- reverse iteration ---

D_TYPE_TRAIT_TRUE(has_rbegin,
    decltype(std::declval<Type&>().rbegin()))

D_TYPE_TRAIT_TRUE(has_rend,
    decltype(std::declval<Type&>().rend()))

D_TYPE_TRAIT_TRUE(has_crbegin,
    decltype(std::declval<const Type&>().crbegin()))

D_TYPE_TRAIT_TRUE(has_crend,
    decltype(std::declval<const Type&>().crend()))

// has_reverse_iteration
//   trait: true if container supports reverse iteration via rbegin()/rend().
template<typename Type>
struct has_reverse_iteration
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_rbegin<cleaned>::value &&
          has_rend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_reverse_iteration_v
    //   variable template: value of has_reverse_iteration<Type>.
    //
    //   Previously this alias was supplied by a duplicate definition over in
    // container_traits.hpp; that duplicate
    // has been removed in favour of this canonical home, which sits next to
    // the struct it aliases.
    template<typename Type>
    constexpr bool has_reverse_iteration_v =
        has_reverse_iteration<Type>::value;
#endif

// has_const_reverse_iteration
//   trait: true if container supports const reverse iteration via
// crbegin()/crend().
template<typename Type>
struct has_const_reverse_iteration
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_crbegin<cleaned>::value &&
          has_crend<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_const_reverse_iteration_v
    //   variable template: value of has_const_reverse_iteration<Type>.
    template<typename Type>
    inline constexpr bool has_const_reverse_iteration_v = has_const_reverse_iteration<Type>::value;
#endif


// ===========================================================================
// V.   Iterator Compatibility
// ===========================================================================

// iterators_compatible
//   trait: true if two containers provide iterators over the same value_type.
template<typename A,
         typename B,
         typename = void>
struct iterators_compatible : std::false_type
{};

template<typename A,
         typename B>
struct iterators_compatible<A, B, void_t<
    typename A::value_type,
    typename B::value_type
>> : std::is_same<
         typename A::value_type,
         typename B::value_type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // iterators_compatible_v
    //   variable template: value of iterators_compatible<A, B>.
    template<typename A,
             typename B>
    constexpr bool iterators_compatible_v =
        iterators_compatible<A, B>::value;
#endif


// ===========================================================================
// VI.  iterator_level Enum
// ===========================================================================

// iterator_level
//   enum: classifies the strongest iterator category a container provides.
enum class iterator_level
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

    // iterator_level_helper
    //   trait: priority cascade resolving the strongest iteration category
    // supported by a container. The
    // forward/bidirectional/random-access/contiguous rungs resolve to the
    // traits in iterator_category_traits.hpp.
    template<typename Type>
    struct iterator_level_helper
    {
    private:
        using cleaned = clean_t<Type>;

    public:
        static constexpr iterator_level value =
            is_contiguous_iterable<cleaned>::value
                ? iterator_level::contiguous

            : is_random_access_iterable<cleaned>::value
                ? iterator_level::random_access

            : is_bidirectional_iterable<cleaned>::value
                ? iterator_level::bidirectional

            : is_forward_iterable<cleaned>::value
                ? iterator_level::forward

            : is_output_iterable<cleaned>::value
                ? iterator_level::output

            : is_input_iterable<cleaned>::value
                ? iterator_level::input

            : iterator_level::none;
    };

NS_END  // internal

// container_iterator_level
//   trait: determines the strongest iterator category the container provides.
template<typename Type>
struct container_iterator_level
{
    static constexpr iterator_level value =
        internal::iterator_level_helper<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // container_iterator_level_v
    //   variable template: value of container_iterator_level<Type>.
    template<typename Type>
    constexpr iterator_level container_iterator_level_v =
        container_iterator_level<Type>::value;
#endif


// ===========================================================================
// VII. Combined Classification
// ===========================================================================

// container_iterator_class
//   struct: complete iterator classification of a container type.
template<typename Type>
struct container_iterator_class
{
    // iterability by category
    static constexpr bool is_iter =
        is_iterable<Type>::value;
    static constexpr bool input_iter =
        is_input_iterable<Type>::value;
    static constexpr bool output_iter =
        is_output_iterable<Type>::value;
    static constexpr bool forward_iter =
        is_forward_iterable<Type>::value;
    static constexpr bool bidir_iter =
        is_bidirectional_iterable<Type>::value;
    static constexpr bool random_access_iter =
        is_random_access_iterable<Type>::value;
    static constexpr bool contiguous_iter =
        is_contiguous_iterable<Type>::value;

    // iteration variants
    static constexpr bool has_const_iter =
        has_const_iteration<Type>::value;
    static constexpr bool has_reverse_iter =
        has_reverse_iteration<Type>::value;
    static constexpr bool has_const_reverse_iter =
        has_const_reverse_iteration<Type>::value;

    // strongest category
    static constexpr iterator_level level =
        container_iterator_level<Type>::value;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ITERATOR_ITERATOR_TRAITS_HPP
