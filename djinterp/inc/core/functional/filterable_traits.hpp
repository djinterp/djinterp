/******************************************************************************
* djinterp [functional]                                  filterable_traits.hpp
*
* SFINAE-based traits for filterable container requirements.
*   Provides type traits that detect whether a container type supports
* filtering operations: iteration (begin/end), element access (value_type),
* push-back or insertion, and an optional native filter method. The
* top-level trait `is_filterable` aggregates these into a single boolean
* that can gate template overloads or static assertions.
*
* No tag dispatching or concepts are used; all detection is performed via
* the void_t / detector SFINAE idiom.
*
*   Supersedes filterable.hpp and filter_traits.hpp which contained
* identical definitions under different include guards.
*
* TABLE OF CONTENTS
* =================
* I.    SFINAE DETECTION PRIMITIVES
*       1.  void_t
*       2.  nonesuch
*       3.  detector (primary + specialization)
*       4.  is_detected / detected_t / detected_or_t aliases
*
* II.   LOW-LEVEL MEMBER DETECTION
*       1.  has_begin           (begin/end iterability)
*       2.  has_end
*       3.  has_value_type      (nested value_type alias)
*       4.  has_push_back       (push_back member function)
*       5.  has_insert          (insert member function)
*       6.  has_size            (size member function)
*       7.  has_empty           (empty member function)
*
* III.  ITERATOR PROPERTY DETECTION
*       1.  has_iterator        (nested iterator alias)
*       2.  has_const_iterator  (nested const_iterator alias)
*
* IV.   NATIVE FILTER DETECTION
*       1.  has_filter_method   (detect .filter() member)
*
* V.    COMPOSITE TRAITS
*       1.  is_iterable         (has begin + end)
*       2.  is_output_capable   (has push_back or insert)
*       3.  is_filterable       (iterable + value_type + output capable)
*
* VI.   CONVENIENCE ALIASES
*       1.  is_filterable_v
*       2.  is_iterable_v
*       3.  has_filter_method_v
*       4.  filterable_value_t
*
* VII.  SFINAE-GATED FILTER FUNCTION
*       1.  filter              (generic)
*       2.  filter              (native dispatch)
*
*
* path:      /inc/functional/filterable_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.12
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_
#define DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include <iterator>


///////////////////////////////////////////////////////////////////////////////
///        I.    SFINAE DETECTION PRIMITIVES                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // void_t_impl
    //   trait: helper that maps any types to void.
    template<typename... _Ts>
    struct void_t_impl
    {
        using type = void;
    };

NS_END  // internal

// void_t
//   type: maps any well-formed type pack to void for SFINAE.
template<typename... _Ts>
using void_t = typename internal::void_t_impl<_Ts...>::type;

// nonesuch
//   type: placeholder type for the detection idiom representing
// "no such type".
struct nonesuch
{
    nonesuch()                         = delete;
    ~nonesuch()                        = delete;
    nonesuch(const nonesuch&)          = delete;
    void operator=(const nonesuch&)    = delete;
};

NS_INTERNAL

    // detector
    //   trait: primary template for SFINAE-based type detection
    // (failure case).
    template<typename                       _Default,
             typename                       _AlwaysVoid,
             template<typename...> typename _Op,
             typename...                    _Args>
    struct detector
    {
        using value_t = std::false_type;
        using type    = _Default;
    };

    // detector specialization (success case)
    //   trait: partial specialization when _Op<_Args...> is
    // well-formed.
    template<typename                       _Default,
             template<typename...> typename _Op,
             typename...                    _Args>
    struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
    {
        using value_t = std::true_type;
        using type    = _Op<_Args...>;
    };

NS_END  // internal

// is_detected
//   trait: true_type if _Op<_Args...> is well-formed, false_type
// otherwise.
template<template<typename...> typename _Op,
         typename...                    _Args>
using is_detected =
    typename internal::detector<nonesuch, void, _Op, _Args...>::value_t;

// detected_t
//   type: yields _Op<_Args...> if well-formed, nonesuch otherwise.
template<template<typename...> typename _Op,
         typename...                    _Args>
using detected_t =
    typename internal::detector<nonesuch, void, _Op, _Args...>::type;

// detected_or_t
//   type: yields _Op<_Args...> if well-formed, _Default otherwise.
template<typename                       _Default,
         template<typename...> typename _Op,
         typename...                    _Args>
using detected_or_t =
    typename internal::detector<_Default, void, _Op, _Args...>::type;


///////////////////////////////////////////////////////////////////////////////
///        II.   LOW-LEVEL MEMBER DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // begin_expr
    //   trait: expression alias for begin() detection.
    template<typename _T>
    using begin_expr = decltype(std::begin(std::declval<_T&>()));

    // end_expr
    //   trait: expression alias for end() detection.
    template<typename _T>
    using end_expr = decltype(std::end(std::declval<_T&>()));

    // value_type_expr
    //   trait: expression alias for nested value_type detection.
    template<typename _T>
    using value_type_expr = typename _T::value_type;

    // push_back_expr
    //   trait: expression alias for push_back() detection.
    template<typename _T>
    using push_back_expr = decltype(
        std::declval<_T&>().push_back(
            std::declval<typename _T::value_type>()));

    // insert_expr
    //   trait: expression alias for insert() detection.
    template<typename _T>
    using insert_expr = decltype(
        std::declval<_T&>().insert(
            std::declval<_T&>().end(),
            std::declval<typename _T::value_type>()));

    // size_expr
    //   trait: expression alias for size() detection.
    template<typename _T>
    using size_expr = decltype(std::declval<const _T&>().size());

    // empty_expr
    //   trait: expression alias for empty() detection.
    template<typename _T>
    using empty_expr = decltype(std::declval<const _T&>().empty());

    // iterator_expr
    //   trait: expression alias for nested iterator detection.
    template<typename _T>
    using iterator_expr = typename _T::iterator;

    // const_iterator_expr
    //   trait: expression alias for nested const_iterator detection.
    template<typename _T>
    using const_iterator_expr = typename _T::const_iterator;

NS_END  // internal

// has_begin
//   trait: detects whether std::begin(_T&) is well-formed.
template<typename _T>
struct has_begin
{
    static constexpr bool value = is_detected<internal::begin_expr, _T>::value;
};

// has_end
//   trait: detects whether std::end(_T&) is well-formed.
template<typename _T>
struct has_end
{
    static constexpr bool value = is_detected<internal::end_expr, _T>::value;
};

// has_value_type
//   trait: detects whether _T::value_type exists.
template<typename _T>
struct has_value_type
{
    static constexpr bool value =
        is_detected<internal::value_type_expr, _T>::value;
};

// has_push_back
//   trait: detects whether _T has a push_back(value_type) member.
template<typename _T>
struct has_push_back
{
    static constexpr bool value =
        is_detected<internal::push_back_expr, _T>::value;
};

// has_insert
//   trait: detects whether _T has an insert(iterator, value_type)
// member.
template<typename _T>
struct has_insert
{
    static constexpr bool value =
        is_detected<internal::insert_expr, _T>::value;
};

// has_size
//   trait: detects whether _T has a size() const member.
template<typename _T>
struct has_size
{
    static constexpr bool value = is_detected<internal::size_expr, _T>::value;
};

// has_empty
//   trait: detects whether _T has an empty() const member.
template<typename _T>
struct has_empty
{
    static constexpr bool value =
        is_detected<internal::empty_expr, _T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        III.  ITERATOR PROPERTY DETECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// has_iterator
//   trait: detects whether _T::iterator exists.
template<typename _T>
struct has_iterator
{
    static constexpr bool value =
        is_detected<internal::iterator_expr, _T>::value;
};

// has_const_iterator
//   trait: detects whether _T::const_iterator exists.
template<typename _T>
struct has_const_iterator
{
    static constexpr bool value =
        is_detected<internal::const_iterator_expr, _T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        IV.   NATIVE FILTER DETECTION                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // filter_method_expr
    //   trait: expression alias detecting a .filter() member that
    // accepts a unary predicate.  The predicate signature is
    // bool(const value_type&).
    template<typename _T>
    using filter_method_expr = decltype(
        std::declval<const _T&>().filter(
            std::declval<bool(*)(const typename _T::value_type&)>()));

NS_END  // internal

// has_filter_method
//   trait: detects whether _T has a filter(predicate) member
// function.
template<typename _T>
struct has_filter_method
{
    static constexpr bool value =
        is_detected<internal::filter_method_expr, _T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        V.    COMPOSITE TRAITS                                          ///
///////////////////////////////////////////////////////////////////////////////

// is_iterable
//   trait: true when _T supports range-based iteration via
// std::begin and std::end.
template<typename _T>
struct is_iterable
{
    static constexpr bool value =
        ( has_begin<_T>::value &&
          has_end<_T>::value );
};

// is_output_capable
//   trait: true when _T supports at least one insertion method
// (push_back or iterator-based insert), enabling construction of
// a filtered result container.
template<typename _T>
struct is_output_capable
{
    static constexpr bool value =
        ( has_push_back<_T>::value ||
          has_insert<_T>::value );
};

// is_filterable
//   trait: true when _T satisfies the complete filterable contract:
// iterable, exposes value_type, and supports result construction.
template<typename _T>
struct is_filterable
{
private:
    using clean_type = typename std::remove_cv<
                           typename std::remove_reference<_T>::type>::type;

public:
    static constexpr bool value =
        ( is_iterable<clean_type>::value    &&
          has_value_type<clean_type>::value &&
          is_output_capable<clean_type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        VI.   CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

// is_filterable_v
//   constant: shorthand for is_filterable<_T>::value.
template<typename _T>
static constexpr bool is_filterable_v = is_filterable<_T>::value;

// is_iterable_v
//   constant: shorthand for is_iterable<_T>::value.
template<typename _T>
static constexpr bool is_iterable_v = is_iterable<_T>::value;

// has_filter_method_v
//   constant: shorthand for has_filter_method<_T>::value.
template<typename _T>
static constexpr bool has_filter_method_v = has_filter_method<_T>::value;

// filterable_value_t
//   type: extracts value_type from a filterable container, or
// nonesuch if unavailable.
template<typename _T>
using filterable_value_t =
    detected_or_t<nonesuch, internal::value_type_expr, _T>;


///////////////////////////////////////////////////////////////////////////////
///        VII.  SFINAE-GATED FILTER FUNCTION                              ///
///////////////////////////////////////////////////////////////////////////////

// filter
//   function: returns a new container of the same type containing only
// elements for which _predicate returns true.  Enabled only when
// _Container satisfies is_filterable.
template<typename _Container,
         typename _Predicate>
typename std::enable_if<
    is_filterable<_Container>::value,
    _Container
>::type
filter
(
    const _Container& _source,
    _Predicate        _predicate
);


// filter (native dispatch)
//   function: overload that delegates to the container's own
// .filter() method when one exists.  Preferred over the generic
// version by SFINAE priority (more constrained).
template<typename _Container,
         typename _Predicate>
typename std::enable_if<
    ( is_filterable<_Container>::value &&
      has_filter_method<_Container>::value ),
    _Container
>::type
filter
(
    const _Container& _source,
    _Predicate        _predicate
);


#endif  // DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_
