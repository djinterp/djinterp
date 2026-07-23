/******************************************************************************
* djinterp [functional]                                      contravariant.hpp
*
* Contravariant functor protocol and its map, contramap (C++).
*   A contravariant functor is a type constructor F<T> that consumes values of
* type T rather than producing them, together with one operation that runs a
* plain function *before* the context instead of after:
*
*     contramap : (B -> A) -> F<A> -> F<B>
*
*   Where a (covariant) Functor's map post-composes -- given F<A> and A -> B it
* yields F<B> -- a Contravariant pre-composes: given a consumer of A and a way
* to turn a B into an A, it yields a consumer of B.  Predicates, comparators,
* serializers, and "sinks" are the canonical instances: a predicate on A
* becomes a predicate on B by mapping each B to an A first; an A -> string
* writer becomes a B -> string writer by converting B to A up front.  This
* header gives that shared shape one name so generic code can pre-adapt any of
* them with a single call, contramap.
*
*   As with functor / applicative / monad, C++ has no native type classes, so a
* contravariant functor is recognized by specializing contravariant_traits<F>
* with the contramap operation (and the inner value_type).  There is no monad
* bridge: contravariance is not derivable from a covariant construction, so
* every instance is registered explicitly, exactly as comonad_traits is.
*
*   THE LAWS.  A lawful instance satisfies
*
*     contramap(id)                 == id
*     contramap(g) . contramap(f)   == contramap(f . g)
*
* -- identity is preserved, and composition is preserved *with the arrows
* reversed* (the defining inversion of a contravariant functor).  The structural
* traits and the C++20 Contravariant concept in Section 0 let generic code
* constrain and introspect without first naming a concrete instance.
*
* USAGE:
*   // a serializer is contravariant: A -> string
*   to_string_of<long> show_long = ...;                 // knows how to show a long
*   auto show_size = contramap(
*       [](const std::string& s){ return (long)s.size(); },  // string -> long
*       show_long);                                     // to_string_of<std::string>
*   std::string s = show_size.run("hello");             // "5"
*
* CONTENTS
*   0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
*   I.    CONTRAVARIANT PROTOCOL
*         1.  contravariant_traits<F>                  (primary, undefined)
*         2.  is_contravariant<T>                       (detection trait)
*   II.   GENERIC CONTRAVARIANT OPERATIONS
*         1.  contramap                                 (the one operation)
*
* path:      /inc/djinterp/core/functional/contravariant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.01
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_CONTRAVARIANT_
#define DJINTERP_FUNCTIONAL_CONTRAVARIANT_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    CONTRAVARIANT PROTOCOL                                ///
///////////////////////////////////////////////////////////////////////////////

// contravariant_traits
//   trait: primary template, undefined by default.  Each concrete
// contravariant functor specializes contravariant_traits<F> (the single-
// argument form; the second parameter is a SFINAE hook mirroring the other
// protocol traits) to expose:
//
//     - value_type      : the inner (consumed) type A of F<A>
//     - contramap(g, fa) : static F<B> contramap(g, F<A>) where g : B -> A
//     - is_specialized  = true_type (marker)
//
//   rebind<U> (= F<U>) is supplied as well by instances for which it is well
// defined.  contramap is the whole obligation.  The primary is left undefined
// so a use on a non-contravariant type produces a clean resolution error.
template<typename _Contravariant,
         typename _Enable = void>
struct contravariant_traits;


NS_INTERNAL

    // is_contravariant_helper
    //   helper: SFINAE detector for whether contravariant_traits<T> is
    // specialized.  Looks for the is_specialized marker every specialization
    // provides.
    template<typename _Type>
    struct is_contravariant_helper
    {
    private:
        template<typename _T>
        static auto test(
            int
        ) -> decltype(
                 typename contravariant_traits<_T>::is_specialized{},
                 std::true_type{}
            );

        template<typename>
        static std::false_type
        test(
            ...
        );

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_contravariant
//   trait: true if _Type has a specialization of contravariant_traits (after
// cv-ref stripping).  Used to SFINAE-constrain generic contravariant ops.
template<typename _Type>
struct is_contravariant
    : internal::is_contravariant_helper<typename std::decay<_Type>::type>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_contravariant_v
    //   value: convenience alias for is_contravariant<_Type>::value.
    template<typename _Type>
    static constexpr bool is_contravariant_v = is_contravariant<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS                    ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the contravariant protocol,
// mirroring functor.hpp Section 0.  is_contravariant (above) answers "does a
// contravariant_traits specialization exist?"; the trait here answers "what is
// the inner consumed type?".  Internal helpers carry a unique contravariant_
// prefix so the umbrella build never collides them with like-named helpers in
// the sibling protocol headers.

NS_INTERNAL

    // contravariant_value_type_helper
    //   helper: SFINAE extractor for contravariant_traits<F>::value_type
    // (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Contravariant>
    struct contravariant_value_type_helper
    {};

    // contravariant_value_type_helper (well-formed specialization)
    template<typename _Contravariant>
    struct contravariant_value_type_helper<
        void_t<typename contravariant_traits<_Contravariant>::value_type>,
        _Contravariant>
    {
        using type = typename contravariant_traits<_Contravariant>::value_type;
    };

NS_END  // internal


// contravariant_value_type
//   trait: the inner consumed type A of a contravariant F, i.e.
// contravariant_traits<F>::value_type.  SFINAE-friendly: has a `::type` only
// when F is a specialized contravariant functor.
template<typename _Contravariant>
struct contravariant_value_type
{
    using type = typename internal::contravariant_value_type_helper<void,
                     typename std::decay<_Contravariant>::type>::type;
};

// contravariant_value_type_t
//   type: convenience alias for contravariant_value_type<F>::type.
template<typename _Contravariant>
using contravariant_value_type_t = typename contravariant_value_type<_Contravariant>::type;


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC CONTRAVARIANT OPERATIONS                      ///
///////////////////////////////////////////////////////////////////////////////

// contramap
//   function: the contravariant map.  Pre-composes _g : B -> A onto the
// consumer _fa : F<A>, yielding F<B> -- the same context now consuming B by
// first turning each B into an A.  The exact result type is whatever the
// instance's contramap produces (F<B> for a concrete context), so it is
// deduced rather than named.  Dispatched to contravariant_traits<F>::contramap
// keyed on the context argument.
//
//   Argument order follows the standard `contramap :: (b -> a) -> f a -> f b`:
// the adapting function first, the context second.
template<typename _Function,
         typename _Contravariant>
D_NODISCARD D_CONSTEXPR auto
contramap
(
    _Function&&      _g,
    _Contravariant&& _fa
) -> decltype(contravariant_traits<typename std::decay<_Contravariant>::type>
        ::contramap(
            std::forward<_Function>(_g),
            std::forward<_Contravariant>(_fa)
        )
    )
{
    return contravariant_traits<
        typename std::decay<_Contravariant>::type>::contramap(
            std::forward<_Function>(_g),
            std::forward<_Contravariant>(_fa));
}


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Contravariant
    //   concept: satisfied when _Type is a specialized contravariant functor.
    // The PascalCase typeclass face, alongside Functor / Applicative.
    template<typename _Type>
    concept Contravariant = is_contravariant<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_CONTRAVARIANT_