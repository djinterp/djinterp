/******************************************************************************
* djinterp                                          constexpr_iterator_traits.hpp
*
* Compile-time iterability classification traits.
*   Detects whether a type can be iterated at compile time, either by
* exposing constexpr-friendly begin/end methods, by exposing dedicated
* constexpr_begin/constexpr_end accessors, or by declaring a nested
* constexpr_iterator type alias.
*
*   The umbrella concept "constexpr-iterable" is defined as:
*     A) iterable, AND
*     B) at compile time
*
*   Detection is purely structural: types declare their constexpr
* iteration capability through their public interface.  No tag types
* or base classes are required.
*
*   PORTABILITY:
*   C++11 baseline.  Variable template `_v` aliases are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES (C++14+).  The detection
* idiom uses void_t (C++17 std, polyfilled to void_t for
* earlier standards).
*
* TABLE OF CONTENTS
* =================
* I.    Method Detection (constexpr_begin / constexpr_end)
* II.   Nested Type Detection (constexpr_iterator)
* III.  Iteration Probes (compile-time well-formedness of begin/end)
* IV.   has_constexpr_iteration
* V.    is_constexpr_iterable
* VI.   Combined Classification
*
*
* path:      /inc/djinterp/core/container/iterator/constexpr_iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_ITERATOR_TRAITS_
#define DJINTERP_CONSTEXPR_ITERATOR_TRAITS_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../core/djinterp.hpp"
#include "../../../core/meta/type_traits.hpp"
#include "./iterator_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Method Detection
// ===========================================================================
// Structural detection of constexpr_begin() / constexpr_end()
// member functions.  Containers wishing to opt into compile-
// time iteration may expose either of these accessors in
// addition to (or instead of) a nested constexpr_iterator
// type alias.

// has_constexpr_begin_method
//   trait: detects a const member `constexpr_begin()`.
template<typename _Type,
         typename = void>
struct has_constexpr_begin_method : std::false_type
{};

template<typename _Type>
struct has_constexpr_begin_method<_Type, void_t<
    decltype(std::declval<const _Type&>().constexpr_begin())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_constexpr_begin_method_v
    //   variable template: value of has_constexpr_begin_method<_Type>.
    template<typename _Type>
    constexpr bool has_constexpr_begin_method_v =
        has_constexpr_begin_method<_Type>::value;
#endif

// has_constexpr_end_method
//   trait: detects a const member `constexpr_end()`.
template<typename _Type,
         typename = void>
struct has_constexpr_end_method : std::false_type
{};

template<typename _Type>
struct has_constexpr_end_method<_Type, void_t<
    decltype(std::declval<const _Type&>().constexpr_end())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_constexpr_end_method_v
    //   variable template: value of has_constexpr_end_method<_Type>.
    template<typename _Type>
    constexpr bool has_constexpr_end_method_v =
        has_constexpr_end_method<_Type>::value;
#endif


// ===========================================================================
// II.  Nested Type Detection
// ===========================================================================

// has_constexpr_iterator_alias
//   trait: detects a nested type alias `constexpr_iterator`.
template<typename _Type,
         typename = void>
struct has_constexpr_iterator_alias : std::false_type
{};

template<typename _Type>
struct has_constexpr_iterator_alias<_Type, void_t<
    typename _Type::constexpr_iterator
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_constexpr_iterator_alias_v
    //   variable template: value of
    // has_constexpr_iterator_alias<_Type>.
    template<typename _Type>
    constexpr bool has_constexpr_iterator_alias_v =
        has_constexpr_iterator_alias<_Type>::value;
#endif


// ===========================================================================
// III. Iteration Probes
// ===========================================================================
// Probes that are well-formed in a constant-evaluated context.
// We do NOT attempt to invoke std::begin/std::end at compile
// time directly here - instead we rely on the structural
// indicators above plus a simple iterability check.

NS_INTERNAL

    // has_constexpr_iter_pair
    //   trait: detects that both constexpr_begin and
    // constexpr_end exist.
    template<typename _Type,
             typename = void>
    struct has_constexpr_iter_pair : std::false_type
    {};

    template<typename _Type>
    struct has_constexpr_iter_pair<_Type, void_t<
        decltype(std::declval<const _Type&>().constexpr_begin()),
        decltype(std::declval<const _Type&>().constexpr_end())
    >> : std::true_type
    {};

NS_END  // internal


// ===========================================================================
// IV.  has_constexpr_iteration
// ===========================================================================

// has_constexpr_iteration
//   trait: true if the container exposes a compile-time
// iteration interface, either via:
//     - a nested `constexpr_iterator` type alias, OR
//     - a `constexpr_begin()` / `constexpr_end()` pair.
template<typename _Type>
struct has_constexpr_iteration
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_constexpr_iterator_alias<cleaned>::value ||
          internal::has_constexpr_iter_pair<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_constexpr_iteration_v
    //   variable template: value of has_constexpr_iteration<_Type>.
    template<typename _Type>
    constexpr bool has_constexpr_iteration_v =
        has_constexpr_iteration<_Type>::value;
#endif


// ===========================================================================
// V.   is_constexpr_iterable
// ===========================================================================

// is_constexpr_iterable
//   trait: true if the type satisfies BOTH:
//     A) is iterable (has begin/end), AND
//     B) supports compile-time iteration
// (has_constexpr_iteration is true).
//
//   The two criteria correspond directly to the umbrella
// definition: constexpr-iterable means iterable AT compile
// time.  A type with a runtime-only begin()/end() is iterable
// but not constexpr-iterable; a type advertising
// constexpr_begin/constexpr_end without a corresponding
// runtime begin/end is constexpr-iterable through its
// dedicated entry points.
template<typename _Type>
struct is_constexpr_iterable
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( ( is_iterable<cleaned>::value                                     ||
            internal::has_constexpr_iter_pair<cleaned>::value               ||
            has_constexpr_iterator_alias<cleaned>::value )                  &&
          has_constexpr_iteration<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_constexpr_iterable_v
    //   variable template: value of is_constexpr_iterable<_Type>.
    template<typename _Type>
    constexpr bool is_constexpr_iterable_v =
        is_constexpr_iterable<_Type>::value;
#endif


// ===========================================================================
// VI.  Combined Classification
// ===========================================================================

// constexpr_iterator_class
//   struct: aggregate compile-time classification of a type's
// constexpr-iteration capabilities.
template<typename _Type>
struct constexpr_iterator_class
{
    static constexpr bool has_constexpr_begin =
        has_constexpr_begin_method<_Type>::value;
    static constexpr bool has_constexpr_end =
        has_constexpr_end_method<_Type>::value;
    static constexpr bool has_constexpr_iter_alias =
        has_constexpr_iterator_alias<_Type>::value;
    static constexpr bool has_constexpr_iter =
        has_constexpr_iteration<_Type>::value;
    static constexpr bool is_constexpr_iter_able =
        is_constexpr_iterable<_Type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONSTEXPR_ITERATOR_TRAITS_