/******************************************************************************
* djinterp [functional]                                        alternative.hpp
*
* Alternative protocol: a monoid on a functor -- empty and choice (C++).
*   An Alternative is a context F<A> that carries a monoid structure at every
* element type: a distinguished empty / failure value, and an associative
* binary choice (Haskell's `<|>`) that picks between two contexts. Where Monoid
* unified combining for ordinary values, Alternative unifies the per-type
* notion of "fall back / try the other one" that already lives, under different
* names, in maybe (or_else), and in the sequence types as concatenation: a
* maybe chooses the first that holds a value; a view or a producer chooses by
* running the first sequence and then the second.
*
*   Because C++ has no native type classes, an Alternative is recognized by
* specializing alternative_traits<F> with two operations -- empty() and
* choice(a, b). The free functions are aempty<F>() (the failure, F explicit,
* like mempty) and alt(a, b) (the choice). asum folds a Foldable of
* alternatives with alt from aempty -- the Alternative counterpart of mconcat
* ("take the first that succeeds").
*
*   Alternative is conceptually a refinement of Applicative, but the concept
* here requires only alternative_traits, not a registered Applicative instance:
* the sequence types (view, producer) carry the empty / choice structure
* through their own empty / concat without otherwise being applicative in this
* framework. maybe -- which is applicative via the monad bridge -- satisfies
* both. result is intentionally NOT an Alternative: like Haskell's Either it has
* no canonical empty (its failure carries a specific error, so there is no
* identity err to choose).
*
*   For a uniform Alternative such as maybe, empty() and choice() both return
* F<A>, so they compose and asum is well-typed. For the lazy sequence families
* the result of choice is a concat view / producer -- a different type with the
* same value_type, exactly as functor_map yields a transform view -- so the
* laws hold over observable elements rather than over the static type, and asum
* is intended for the uniform case.
*
* USAGE:
*   using namespace djinterp;
*   maybe<int> a = nothing<int>(), b = just(7);
*   maybe<int> r = alt(a, b);                       // just(7)
*   maybe<int> z = aempty<maybe<int> >();           // nothing
*
*   // first option that holds a value, from a list of options:
*   std::vector<maybe<int> > opts{ nothing<int>(), just(3), just(9) };
*   maybe<int> first = asum(opts);                  // just(3)
*
* 
* path:      /inc/djinterp/core/functional/alternative.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ALTERNATIVE PROTOCOL
      1.  alternative_traits<F>                   (primary, undefined)
      2.  is_alternative<T>                       (detection trait)
II.   GENERIC ALTERNATIVE OPERATIONS
      1.  aempty<F>                               (the empty / failure)
      2.  alt                                     (associative choice, <|>)
      3.  asum                                    (choose across a foldable)
*/


#ifndef DJINTERP_FUNCTIONAL_ALTERNATIVE_
#define DJINTERP_FUNCTIONAL_ALTERNATIVE_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./foldable.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    ALTERNATIVE PROTOCOL                                  ///
///////////////////////////////////////////////////////////////////////////////

// alternative_traits
//   trait: primary template, undefined by default. Each concrete
// alternative specializes alternative_traits<F> to expose:
//
//     - value_type     : the inner type A of F<A>
//     - empty()         : static -- the failure / identity for choice
//     - choice(a, b)     : static -- associative choice between two contexts
//     - is_specialized   = true_type (marker)
//
//   The second parameter is a SFINAE hook used by the family instances
// (view, producer) that key on a structural trait. The primary is left
// undefined so a use on a non-alternative produces a clean resolution error.
template<typename _Alternative,
         typename _Enable = void>
struct alternative_traits;


NS_INTERNAL

    // is_alternative_helper
    //   helper: SFINAE detector for whether alternative_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename _Type>
    struct is_alternative_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename alternative_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_alternative
//   trait: true if _Type has a specialization of alternative_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic operations.
template<typename _Type>
struct is_alternative
    : internal::is_alternative_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_alternative_v
//   value: convenience alias for is_alternative<_Type>::value.
template<typename _Type>
static constexpr bool is_alternative_v = is_alternative<_Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Alternative
    //   concept: satisfied when _Type is a specialized alternative. The
    // PascalCase typeclass face, alongside Functor / Applicative / Foldable /
    // Monoid. (Conceptually a refinement of Applicative; see the header note.)
    template<typename _Type>
    concept Alternative = is_alternative<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS                    ///
///////////////////////////////////////////////////////////////////////////////
//   The inner value type of an alternative, mirroring functor_value_type /
// foldable_value_type. Internal helpers carry a unique alternative_ prefix to
// keep the umbrella build collision-free.

NS_INTERNAL

    // alternative_value_type_helper
    //   helper: SFINAE extractor for alternative_traits<F>::value_type.
    template<typename _AlwaysVoid,
             typename _Alternative>
    struct alternative_value_type_helper
    {};

    template<typename _Alternative>
    struct alternative_value_type_helper<
        void_t<typename alternative_traits<_Alternative>::value_type>,
        _Alternative>
    {
        using type = typename alternative_traits<_Alternative>::value_type;
    };

NS_END  // internal


// alternative_value_type
//   trait: the inner value type A of an alternative F. SFINAE-friendly.
template<typename _Alternative>
struct alternative_value_type
{
    using type = typename internal::alternative_value_type_helper<
        void, typename std::decay<_Alternative>::type>::type;
};

// alternative_value_type_t
//   type: convenience alias for alternative_value_type<F>::type.
template<typename _Alternative>
using alternative_value_type_t =
    typename alternative_value_type<_Alternative>::type;


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC ALTERNATIVE OPERATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   aempty delegates to alternative_traits<F>::empty; alt to ::choice; asum
// folds a Foldable through alt from aempty. All are D_CONSTEXPR and fold at
// compile time wherever the instance's empty / choice and fold_left do.

// aempty
//   function: the empty / failure element of an alternative. The type
// _Alternative must be supplied explicitly (it cannot be deduced), as with
// mempty. For maybe this is nothing; for a sequence it is the empty sequence.
//
//   Example: aempty<maybe<int>>() -> nothing
template<typename _Alternative>
D_NODISCARD
D_CONSTEXPR
auto aempty()
-> decltype(alternative_traits<_Alternative>::empty())
{
    return alternative_traits<_Alternative>::empty();
}


// alt
//   function: associative choice between two contexts of the same alternative
//   (Haskell's `<|>`). maybe keeps the first that holds a value; a view or
// producer yields the first sequence then the second. For a uniform
// alternative the result is the same F<A>; for a lazy sequence family it is a
// concat context with the same value_type.
//
//   Example: alt(nothing<int>(), just(7)) -> just(7)
template<typename _AlternativeA,
         typename _AlternativeB>
D_NODISCARD
D_CONSTEXPR
auto alt
(
    _AlternativeA&& _a,
    _AlternativeB&& _b
)
-> decltype(alternative_traits<typename std::decay<_AlternativeA>::type>::choice(
       std::forward<_AlternativeA>(_a),
       std::forward<_AlternativeB>(_b)))
{
    return alternative_traits<typename std::decay<_AlternativeA>::type>::choice(
        std::forward<_AlternativeA>(_a),
        std::forward<_AlternativeB>(_b));
}


NS_INTERNAL

    // alternative_choice_helper
    //   helper: the reducer behind asum -- folds the running accumulator and
    // the next option together with alt, threading the accumulator by value.
    // A named functor keeps it usable on every floor. Intended for a uniform
    // alternative, whose alt returns the same type (so the fold accumulator is
    // stable).
    template<typename _Alternative>
    struct alternative_choice_helper
    {
        D_CONSTEXPR
        _Alternative operator()(
            _Alternative        _acc,
            const _Alternative& _option
        ) const
        {
            return ::djinterp::alt(_acc, _option);
        }
    };

NS_END  // internal


// asum
//   function: chooses across every element of a Foldable whose elements are
// themselves a (uniform) alternative -- folding from aempty with alt. Returns
// the first element that "succeeds" (for maybe, the first just); aempty if the
// foldable is empty. The Alternative counterpart of mconcat.
//
//   Example: asum(vector<maybe<int>>{ nothing, just(3), just(9) }) -> just(3)
template<typename _Foldable>
D_NODISCARD
D_CONSTEXPR
foldable_value_type_t<_Foldable>
asum
(
    const _Foldable& _fa
)
{
    using alternative_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        ::djinterp::aempty<alternative_t>(),
        internal::alternative_choice_helper<alternative_t>());
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_ALTERNATIVE_
