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
*
* path:      /inc/functional/meta/filterable_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LOW-LEVEL MEMBER DETECTION
      1.  has_begin           (begin/end iterability)
      2.  has_end
      3.  has_value_type      (nested value_type alias)
      4.  has_push_back       (push_back member function)
      5.  has_insert          (insert member function)
      6.  has_size            (size member function)
      7.  has_empty           (empty member function)
II.   ITERATOR PROPERTY DETECTION
      1.  has_iterator        (nested iterator alias)
      2.  has_const_iterator  (nested const_iterator alias)
III.  NATIVE FILTER DETECTION
      1.  has_filter_method   (detect .filter() member)
IV.   COMPOSITE TRAITS
      1.  is_iterable         (has begin + end)
      2.  is_output_capable   (has push_back or insert)
      3.  is_filterable       (iterable + value_type + output capable)
V.    CONVENIENCE ALIASES
      1.  is_filterable_v
      2.  is_iterable_v
      3.  has_filter_method_v
      4.  filterable_value_t
VI.   SFINAE-GATED FILTER FUNCTION
      1.  filter              (generic)
      2.  filter              (native dispatch)
*/

#ifndef DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_
#define DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <iterator>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///        I.   LOW-LEVEL MEMBER DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // begin_expression
    //   trait: expression alias for begin() detection.
    template<typename _Type>
    using begin_expression = decltype(std::begin(std::declval<_Type&>()));

    // end_expression
    //   trait: expression alias for end() detection.
    template<typename _Type>
    using end_expression = decltype(std::end(std::declval<_Type&>()));

    // value_type_expr
    //   trait: expression alias for nested value_type detection.
    template<typename _Type>
    using value_type_expr = typename _Type::value_type;

    // push_back_expression
    //   trait: expression alias for push_back() detection.
    template<typename _Type>
    using push_back_expression = decltype(
        std::declval<_Type&>().push_back(
            std::declval<typename _Type::value_type>()));

    // insert_expression
    //   trait: expression alias for insert() detection.
    template<typename _Type>
    using insert_expression = decltype(
        std::declval<_Type&>().insert(
            std::declval<_Type&>().end(),
            std::declval<typename _Type::value_type>()));

    // size_expression
    //   trait: expression alias for size() detection.
    template<typename _Type>
    using size_expression = decltype(std::declval<const _Type&>().size());

    // empty_expression
    //   trait: expression alias for empty() detection.
    template<typename _Type>
    using empty_expression = decltype(std::declval<const _Type&>().empty());

    // iterator_expression
    //   trait: expression alias for nested iterator detection.
    template<typename _Type>
    using iterator_expression = typename _Type::iterator;

    // const_iterator_expression
    //   trait: expression alias for nested const_iterator detection.
    template<typename _Type>
    using const_iterator_expression = typename _Type::const_iterator;

NS_END  // internal

// has_begin
//   trait: detects whether std::begin(_Type&) is well-formed.
template<typename _Type>
struct has_begin
{
    static constexpr bool value = 
        is_detected<internal::begin_expression, _Type>::value;
};

// has_end
//   trait: detects whether std::end(_Type&) is well-formed.
template<typename _Type>
struct has_end
{
    static constexpr bool value = 
        is_detected<internal::end_expression, _Type>::value;
};

// has_value_type
//   trait: detects whether _Type::value_type exists.
template<typename _Type>
struct has_value_type
{
    static constexpr bool value =
        is_detected<internal::value_type_expr, _Type>::value;
};

// has_push_back
//   trait: detects whether _Type has a push_back(value_type) member.
template<typename _Type>
struct has_push_back
{
    static constexpr bool value =
        is_detected<internal::push_back_expression, _Type>::value;
};

// has_insert
//   trait: detects whether _Type has an insert(iterator, value_type)
// member.
template<typename _Type>
struct has_insert
{
    static constexpr bool value =
        is_detected<internal::insert_expression, _Type>::value;
};

// has_size
//   trait: detects whether _Type has a size() const member.
template<typename _Type>
struct has_size
{
    static constexpr bool value = 
        is_detected<internal::size_expression, _Type>::value;
};

// has_empty
//   trait: detects whether _Type has an empty() const member.
template<typename _Type>
struct has_empty
{
    static constexpr bool value =
        is_detected<internal::empty_expression, _Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        II.  ITERATOR PROPERTY DETECTION                                 ///
///////////////////////////////////////////////////////////////////////////////

// has_iterator
//   trait: detects whether _Type::iterator exists.
template<typename _Type>
struct has_iterator
{
    static constexpr bool value =
        is_detected<internal::iterator_expression, _Type>::value;
};

// has_const_iterator
//   trait: detects whether _Type::const_iterator exists.
template<typename _Type>
struct has_const_iterator
{
    static constexpr bool value =
        is_detected<internal::const_iterator_expression, _Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        III.   NATIVE FILTER DETECTION                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // filter_method_expr
    //   trait: expression alias detecting a .filter() member that
    // accepts a unary predicate.  The predicate signature is
    // bool(const value_type&).
    template<typename _Type>
    using filter_method_expr = decltype(
        std::declval<const _Type&>().filter(
            std::declval<bool(*)(const typename _Type::value_type&)>()));

NS_END  // internal

// has_filter_method
//   trait: detects whether _Type has a filter(predicate) member
// function.
template<typename _Type>
struct has_filter_method
{
    static constexpr bool value =
        is_detected<internal::filter_method_expr, _Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        V.    COMPOSITE TRAITS                                           ///
///////////////////////////////////////////////////////////////////////////////

// is_iterable
//   trait: true when _Type supports range-based iteration via
// std::begin and std::end.
template<typename _Type>
struct is_iterable
{
    static constexpr bool value =
        ( has_begin<_Type>::value &&
          has_end<_Type>::value );
};

// is_output_capable
//   trait: true when _Type supports at least one insertion method
// (push_back or iterator-based insert), enabling construction of
// a filtered result container.
template<typename _Type>
struct is_output_capable
{
    static constexpr bool value =
        ( has_push_back<_Type>::value ||
          has_insert<_Type>::value );
};

// is_filterable
//   trait: true when _Type satisfies the complete filterable contract:
// iterable, exposes value_type, and supports result construction.
template<typename _Type>
struct is_filterable
{
private:
    using clean_type = typename std::remove_cv<
                           typename std::remove_reference<_Type>::type>::type;

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
//   constant: shorthand for is_filterable<_Type>::value.
template<typename _Type>
static constexpr bool is_filterable_v = is_filterable<_Type>::value;

// is_iterable_v
//   constant: shorthand for is_iterable<_Type>::value.
template<typename _Type>
static constexpr bool is_iterable_v = is_iterable<_Type>::value;

// has_filter_method_v
//   constant: shorthand for has_filter_method<_Type>::value.
template<typename _Type>
static constexpr bool has_filter_method_v = has_filter_method<_Type>::value;

// filterable_value_t
//   type: extracts value_type from a filterable container, or
// nonesuch if unavailable.
template<typename _Type>
using filterable_value_t =
    detected_or_t<nonesuch, internal::value_type_expr, _Type>;


///////////////////////////////////////////////////////////////////////////////
///        VI.  SFINAE-GATED FILTER FUNCTION                               ///
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
filter(
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
filter(
    const _Container& _source,
    _Predicate        _predicate
);


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FILTERABLE_TRAITS_