/******************************************************************************
* djinterp [functional]                                            functor.hpp
*
* Functor protocol and the generic functorial map (C++).
*   A functor here is a type constructor F<T> -- a context holding values of
* type T -- together with one operation, map : (T -> U) -> F<T> -> F<U>, that
* applies a plain function to the contained value(s) without disturbing the
* surrounding context. maybe<T>, result<T, E>, lazy views, and producers are
* all functors; this header gives that shared shape a single name so generic
* code can map over any of them with one call, functor_map.
*
*   Because C++ has no native type classes, a functor is recognized by
* specializing functor_traits<F> with the map operation (and the inner
* value_type). A concrete context that already exposes a .map() supplies a
* one-line specialization; a context whose mapped type depends on the mapping
* function (a view, a producer) specializes the same protocol and lets the
* return type follow from the call. Specializations live in the concrete
* type's own header, exactly as the monad_traits specializations do.
*
*   Every monad is a functor. Rather than make each monad re-state that, this
* header carries a single blanket specialization (keyed on is_monad) that
* derives map from monad_map -- so maybe, result, and any future monad are
* functors automatically, with no per-type wiring. functor.hpp therefore
* references monad.hpp for that bridge; the two are sibling protocol headers
* in the monadic layer.
*
*   The structural traits and the C++20 Functor concept in Section 0 describe
* the protocol vocabulary -- functor-ness, the inner value type, and whether a
* given (functor, function) pair is mappable -- so generic code can constrain
* and introspect without first naming a concrete functor.
*
* USAGE:
*   maybe<int> m = just(21);
*   auto n = functor_map(m, [](int v) { return v * 2; });   // maybe<int> -> 42
*
*   // generic over ANY functor: maybe, result, view, producer, ...
*   template<typename _F, typename _Fn>
*   auto bump(_F&& _fa, _Fn _fn)
*       -> decltype(functor_map(std::forward<_F>(_fa), _fn))
*   {
*       return functor_map(std::forward<_F>(_fa), _fn);
*   }
*
* 
* path:      /inc/djinterp/core/functional/functor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    FUNCTOR PROTOCOL
      1.  functor_traits<F>                       (primary, undefined)
      2.  functor_traits<F> [monad bridge]        (every monad is a functor)
      3.  is_functor<T>                           (detection trait)
II.   GENERIC FUNCTOR OPERATIONS
      1.  functor_map                             (fmap, the one operation)
*/


#ifndef DJINTERP_FUNCTIONAL_FUNCTOR_
#define DJINTERP_FUNCTIONAL_FUNCTOR_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./monad.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    FUNCTOR PROTOCOL                                      ///
///////////////////////////////////////////////////////////////////////////////

// functor_traits
//   trait: primary template, undefined by default. Each concrete
// functor specializes functor_traits<F> (the single-argument form;
// the second parameter is a SFINAE hook used only by the blanket
// monad specialization below) to expose:
//
//     - value_type     : the inner type T of F<T>
//     - map(fa, f)      : static F<U> map(F<T>, f) where f : T -> U
//     - is_specialized  = true_type (marker)
//
//   rebind<U> (= F<U>) is supplied as well by contexts for which it is
// well-defined (maybe, result); it is intentionally NOT part of the core
// protocol, because a context whose mapped type depends on the mapping
// function -- a view, a producer -- has no single F<U> to name. For those,
// map's return type follows from the call alone. The primary is left
// undefined so a use on a non-functor produces a clean resolution error.
template<typename _Functor,
         typename _Enable = void>
struct functor_traits;


// functor_traits<_Functor> (monad bridge)
//   specialization: every monad is a functor. Keyed on is_monad, this
// derives the functor operation from the monad's own bind+unit (monad_map),
// so maybe, result, and any future monad participate as functors with no
// per-type specialization. A view / producer is not a monad, so its explicit
// specialization (in its own header) never overlaps this one.
template<typename _Functor>
struct functor_traits<
    _Functor,
    typename std::enable_if<is_monad<_Functor>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename monad_value_type<_Functor>::type;

    template<typename _To>
    using rebind = typename monad_rebind<_Functor, _To>::type;

    // map
    //   functorial map via the monad protocol (monad_map = bind+unit).
    // D_CONSTEXPR follows monad_map: it folds at compile time under C++20
    // over a monad whose value is a carrier leaf, and runs at runtime on
    // the C++17 floor where maybe / result are not literal types.
    template<typename _Function>
    static
    D_CONSTEXPR
    auto map(
        const _Functor& _fa,
        _Function       _function
    )
    -> decltype(::djinterp::monad_map(_fa, _function))
    {
        return ::djinterp::monad_map(_fa, _function);
    }
};


NS_INTERNAL

    // is_functor_helper
    //   helper: SFINAE detector for whether functor_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization (including the monad bridge) provides.
    template<typename _Type>
    struct is_functor_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename functor_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_functor
//   trait: true if _Type has a specialization of functor_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic functor operations.
template<typename _Type>
struct is_functor
    : internal::is_functor_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_functor_v
//   value: convenience alias for is_functor<_Type>::value.
template<typename _Type>
static constexpr bool is_functor_v = is_functor<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the functor protocol, built on
// the core ::void_t SFINAE sink declared in djinterp.hpp. is_functor (above,
// kept where the protocol is introduced) answers "does a functor_traits
// specialization exist?"; the traits here answer the finer-grained questions
// generic code depends on: what is the inner value type, and is a given
// (functor, function) pair mappable. Each predicate reduces to a `static
// constexpr bool value` (or a `::type` for the type-yielding trait). The
// C++20 concepts close the section. Internal helpers carry a unique functor_
// prefix so the umbrella build never collides them with the like-named
// helpers in monad.hpp.

NS_INTERNAL

    // functor_value_type_helper
    //   helper: SFINAE extractor for functor_traits<F>::value_type
    // (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Functor>
    struct functor_value_type_helper
    {};

    // functor_value_type_helper (well-formed specialization)
    //   helper: yields functor_traits<F>::value_type when present.
    template<typename _Functor>
    struct functor_value_type_helper<
        void_t<typename functor_traits<_Functor>::value_type>,
        _Functor>
    {
        using type = typename functor_traits<_Functor>::value_type;
    };

    // is_fmappable_helper
    //   helper: detection sink for a well-formed functor_map(F, Fn)
    // (primary: false). The well-formed specialization is defined after
    // functor_map below.
    template<typename _AlwaysVoid,
             typename _Functor,
             typename _Function>
    struct is_fmappable_helper : std::false_type
    {};

NS_END  // internal


// functor_value_type
//   trait: the inner value type T of a functor F, i.e.
// functor_traits<F>::value_type. SFINAE-friendly: has a `::type` only
// when F is a specialized functor.
template<typename _Functor>
struct functor_value_type
{
    using type = typename internal::functor_value_type_helper<
        void, typename std::decay<_Functor>::type>::type;
};

// functor_value_type_t
//   type: convenience alias for functor_value_type<F>::type.
template<typename _Functor>
using functor_value_type_t = typename functor_value_type<_Functor>::type;


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC FUNCTOR OPERATIONS                            ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN. functor_map is D_CONSTEXPR and delegates to
// functor_traits<F>::map. For a monad context it folds exactly where monad_map
// does (compile time under C++20 over a carrier-holding maybe / result, runtime
// on the C++17 floor). For a view / producer context, the map is the lazy
// transform that module already provides, so functor_map simply names that one
// transformation uniformly across every functor.

// functor_map
//   function: functorial map (fmap). Applies _function : T -> U to the
// value(s) inside the functor, yielding the same context over U. The exact
// result type is whatever the instance's map produces -- F<U> for a concrete
// context (maybe<U>, result<U, E>), or the instance's transformed view /
// producer otherwise -- so it is deduced rather than named.
template<typename _Functor,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto functor_map
(
    _Functor&&  _fa,
    _Function&& _function
)
-> decltype(functor_traits<typename std::decay<_Functor>::type>::map(
       std::forward<_Functor>(_fa),
       std::forward<_Function>(_function)))
{
    return functor_traits<typename std::decay<_Functor>::type>::map(
        std::forward<_Functor>(_fa),
        std::forward<_Function>(_function));
}


NS_INTERNAL

    // is_fmappable_helper (well-formed specialization)
    //   helper: true when functor_map(F, Fn) is a valid expression.
    template<typename _Functor,
             typename _Function>
    struct is_fmappable_helper<
        void_t<decltype(::djinterp::functor_map(
            std::declval<_Functor>(), std::declval<_Function>()))>,
        _Functor,
        _Function> : std::true_type
    {};

NS_END  // internal


// is_fmappable
//   trait: true when functor_map(declval<F>(), declval<Fn>()) is a
// well-formed expression.
template<typename _Functor,
         typename _Function>
struct is_fmappable
    : internal::is_fmappable_helper<void, _Functor, _Function>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_fmappable_v
    //   value: convenience alias for is_fmappable<...>::value.
    template<typename _Functor,
             typename _Function>
    constexpr bool is_fmappable_v = is_fmappable<_Functor, _Function>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Functor
    //   concept: satisfied when _Type is a specialized functor. The
    // PascalCase typeclass face, alongside Callable / Predicate.
    template<typename _Type>
    concept Functor = is_functor<_Type>::value;

    // fmappable_with
    //   concept: satisfied when functor_map(_Functor, _Function) is
    // well-formed (mirrors monad's mappable_with).
    template<typename _Functor,
             typename _Function>
    concept fmappable_with = is_fmappable<_Functor, _Function>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FUNCTOR_
