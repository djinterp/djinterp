/******************************************************************************
* djinterp [functional]                                          semigroup.hpp
*
* Semigroup protocol and its associative combine (C++).
*   A semigroup is a type T with one operation: an associative binary combine
*   T x T -> T. "Associative" means combine(a, combine(b, c)) ==
* combine(combine(a, b), c); there is no required identity element (that is the
* Monoid refinement, monoid.hpp). Where Functor unified the per-type map and
* Foldable the per-type fold, Semigroup unifies the per-type combine: a view's
* concat, a result's combine, string and vector concatenation, numeric
* addition -- all are the one associative operation under different names.
*
*   Because C++ has no native type classes, a semigroup is recognized by
* specializing semigroup_traits<T> with a single static combine. The free
* function mappend(a, b) dispatches to it. The name combine is deliberately
* NOT used for the free function: accumulator.hpp already owns combine(...)
* (variadic parallel folds), so the monoid vocabulary mappend / mempty /
* mconcat is used throughout this layer to stay collision-free on the umbrella.
*
*   Concrete instances (string, vector, and the numeric / boolean newtypes in
* namespace monoids) live in monoid.hpp, where each type's full algebra -- its
* semigroup combine together with its monoid identity -- is defined in one
* place. This header is the protocol and the associative operation alone.
*
* USAGE:
*   // any two values of a semigroup type combine associatively:
*   std::string s = mappend(std::string("foo"), std::string("bar"));  // "foobar"
*
*   // generic over any semigroup:
*   template<typename _S>
*   _S thrice(const _S& _x) { return mappend(_x, mappend(_x, _x)); }
*
* 
* path:      /inc/djinterp/core/functional/semigroup.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SEMIGROUP PROTOCOL
      1.  semigroup_traits<T>                     (primary, undefined)
      2.  is_semigroup<T>                         (detection trait)
II.   GENERIC SEMIGROUP OPERATION
      1.  mappend                                 (associative combine)
*/


#ifndef DJINTERP_FUNCTIONAL_SEMIGROUP_
#define DJINTERP_FUNCTIONAL_SEMIGROUP_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    SEMIGROUP PROTOCOL                                    ///
///////////////////////////////////////////////////////////////////////////////

// semigroup_traits
//   trait: primary template, undefined by default. Each concrete
// semigroup specializes semigroup_traits<T> to expose:
//
//     - combine(a, b)  : static T combine(const T&, const T&) -- the
//                        associative binary operation
//     - is_specialized = true_type (marker)
//
//   The second parameter is a SFINAE hook used by family instances (e.g.
// the numeric newtypes in monoid.hpp) that key on a structural trait rather
// than a concrete type. The primary is left undefined so a use on a
// non-semigroup produces a clean resolution error.
template<typename _Semigroup,
         typename _Enable = void>
struct semigroup_traits;


NS_INTERNAL

    // is_semigroup_helper
    //   helper: SFINAE detector for whether semigroup_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename _Type>
    struct is_semigroup_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename semigroup_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_semigroup
//   trait: true if _Type has a specialization of semigroup_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic operations.
template<typename _Type>
struct is_semigroup
    : internal::is_semigroup_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_semigroup_v
//   value: convenience alias for is_semigroup<_Type>::value.
template<typename _Type>
static constexpr bool is_semigroup_v = is_semigroup<_Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Semigroup
    //   concept: satisfied when _Type is a specialized semigroup. The
    // PascalCase typeclass face, alongside Functor / Applicative / Foldable.
    template<typename _Type>
    concept Semigroup = is_semigroup<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC SEMIGROUP OPERATION                           ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN. mappend is D_CONSTEXPR and delegates to
// semigroup_traits<T>::combine; it folds at compile time under C++20 (and
// earlier, wherever the instance's combine is itself constexpr over a literal
// type) and runs at runtime otherwise -- the same conditional-constexpr
// behaviour as the rest of the layer.

// mappend
//   function: the associative binary combine of two values of a semigroup,
//   T x T -> T (Haskell's `<>`). Both operands must be the same semigroup
// type. Dispatches to semigroup_traits<T>::combine.
//
//   Example: mappend(std::string("a"), std::string("b")) -> "ab"
template<typename _Semigroup>
D_NODISCARD
D_CONSTEXPR
auto mappend
(
    const _Semigroup& _a,
    const _Semigroup& _b
)
-> decltype(semigroup_traits<typename std::decay<_Semigroup>::type>::combine(
       _a, _b))
{
    return semigroup_traits<typename std::decay<_Semigroup>::type>::combine(
        _a, _b);
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_SEMIGROUP_
