/*******************************************************************************
* djinterp [core]                                                applicative.hpp
*
* Applicative protocol and its generic operations: ap and lift_a2 (C++).
*   An applicative functor sits between Functor and Monad: it is a context
* F<T> that, beyond plain map, can (1) lift a bare value into the context --
* pure -- and (2) apply a wrapped function to a wrapped argument -- ap, the
* operation written `<*>` in Haskell, F<(A -> B)> -> F<A> -> F<B>. From those
* two, lift_a2 lifts an ordinary binary function (A, B) -> C to operate on two
* contexts at once, F<A> -> F<B> -> F<C>, the applicative counterpart of the
* monad layer's lift_m2.
*
*   As with functor / monad, C++ has no native type classes, so an applicative
* is recognized by specializing applicative_traits<F> with pure and ap (and
* the inner value_type). pure and ap are the only per-type obligations; ap, of
* necessity, names the same F for the wrapped function and the wrapped
* argument, so a context already participates by exposing those two pieces.
* lift_a2 is then derived once, generically, for every applicative -- it is not
* a per-type obligation.
*
*   Every monad is an applicative. A single blanket specialization (keyed on
* is_monad) derives pure from monad_unit and ap from monad_bind + monad_map, so
* maybe, result, and any future monad are applicatives automatically, with no
* per-type wiring -- mirroring the monad bridge in functor.hpp. applicative.hpp
* therefore references monad.hpp (for the bridge) and functor.hpp (lift_a2 is
* expressed as functor_map followed by ap); the three are sibling protocol
* headers in the monadic layer.
*
* USAGE:
*   maybe<int> a = just(2);
*   maybe<int> b = just(3);
*   auto s = lift_a2(a, b, [](int x, int y){ return x + y; });   // just(5)
*
*   // apply a wrapped function to a wrapped argument
*   auto f = just(times2{});                 // maybe<times2>
*   auto r = ap(f, just(21));                // just(42)
*
*   // lift a bare value into a chosen applicative
*   auto p = pure<maybe<int>>(7);            // just(7)
*
*
* path:      /inc/djinterp/core/functional/applicative.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.10
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
      ---------------------------------------------

I.    APPLICATIVE PROTOCOL
      --------------------
      1.    applicative_traits<F>                   (primary, undefined)
      2.    applicative_traits<F> [monad bridge]    (every monad is applicative)
      3.    is_applicative<T>                        (detection trait)

II.   GENERIC APPLICATIVE OPERATIONS
      ------------------------------
      1.    pure<F>                                  (lift value into F)
      2.    ap                                       (F<a->b> -> F<a> -> F<b>)
      3.    lift_a2                                  (binary applicative lift)
*/


#ifndef DJINTERP_FUNCTIONAL_APPLICATIVE_HPP
#define DJINTERP_FUNCTIONAL_APPLICATIVE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // void_t
#include "./monad.hpp"
#include "./functor.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    APPLICATIVE PROTOCOL                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // applicative_ap_binder
    //   helper: captures the wrapped argument fa : F<A> and, applied to a
    // plain function f : A -> B, yields functorial map of f over fa, i.e.
    // monad_map(fa, f) : F<B>. This is the `\f -> map f fa` step that the
    // monad bridge's ap binds across the wrapped function F<A -> B>. A named
    // class (not a lambda) is used so it can appear in the bridge's trailing
    // return type, mirroring kleisli_helper in monad.hpp.
    template<typename FunctorA>
    class applicative_ap_binder
    {
    public:
        // The self-type guard keeps this single-argument forwarding
        // constructor from outcompeting the implicit copy / move
        // constructors when the binder itself is copied (the bridge's map
        // takes its function by value) -- the single-argument analogue of
        // why kleisli_helper's two-argument constructor is collision-free.
        template<typename FaFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<FaFwd>::type,
                         applicative_ap_binder>::value>::type>
        D_CONSTEXPR
        explicit applicative_ap_binder(
            FaFwd&& _fa
        )
            : m_fa(std::forward<FaFwd>(_fa))
        {}

        template<typename Function>
        D_CONSTEXPR
        auto operator()(
            const Function& _function
        ) const
        -> decltype(::djinterp::monad_map(
               std::declval<const FunctorA&>(), _function))
        {
            return ::djinterp::monad_map(m_fa, _function);
        }

    private:
        FunctorA m_fa;
    };

NS_END  // internal


// applicative_traits
//   trait: primary template, undefined by default. Each concrete
// applicative specializes applicative_traits<F> (the single-argument
// form; the second parameter is a SFINAE hook used only by the blanket
// monad specialization below) to expose:
//
//     - value_type        : the inner type T of F<T>
//     - pure(value)        : static F<T> pure(T) -- lift a bare value
//     - ap(ff, fa)         : static F<U> ap(F<T->U>, F<T>) -- wrapped apply
//     - is_specialized     = true_type (marker)
//
//   pure and ap are the whole obligation; lift_a2 is derived generically
// below. The primary is left undefined so a use on a non-applicative
// produces a clean resolution error.
template<typename Applicative,
         typename Enable = void>
struct applicative_traits;


// applicative_traits<Applicative> (monad bridge)
//   specialization: every monad is an applicative. Keyed on is_monad, this
// derives pure from the monad's unit and ap from bind + map, so maybe,
// result, and any future monad participate as applicatives with no per-type
// specialization. A view / producer is not a monad, so its explicit
// specialization (in its own header) never overlaps this one.
template<typename Applicative>
struct applicative_traits<
    Applicative,
    typename std::enable_if<is_monad<Applicative>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename monad_value_type<Applicative>::type;

    template<typename To>
    using rebind = typename monad_rebind<Applicative, To>::type;

    // pure
    //   lift a bare value into the applicative via the monad's unit.
    template<typename Value>
    static
    D_CONSTEXPR
    auto pure(
        Value&& _value
    )
    -> decltype(::djinterp::monad_unit<Applicative>(
           std::forward<Value>(_value)))
    {
        return ::djinterp::monad_unit<Applicative>(
            std::forward<Value>(_value));
    }

    // ap
    //   wrapped application via the monad protocol:
    //   ap(ff, fa) = bind ff (\f -> map f fa).
    // D_CONSTEXPR follows monad_bind / monad_map: it folds at compile time
    // under C++20 over a monad whose value is a carrier leaf, and runs at
    // runtime on the C++17 floor where maybe / result are not literal types.
    template<typename WrappedFn,
             typename FunctorA>
    static
    D_CONSTEXPR
    auto ap(
        WrappedFn&& _ff,
        FunctorA&&  _fa
    )
    -> decltype(::djinterp::monad_bind(
           std::forward<WrappedFn>(_ff),
           internal::applicative_ap_binder<
               typename std::decay<FunctorA>::type>(
                   std::forward<FunctorA>(_fa))))
    {
        return ::djinterp::monad_bind(
            std::forward<WrappedFn>(_ff),
            internal::applicative_ap_binder<
                typename std::decay<FunctorA>::type>(
                    std::forward<FunctorA>(_fa)));
    }
};


NS_INTERNAL

    // is_applicative_helper
    //   helper: SFINAE detector for whether applicative_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization (including the monad bridge) provides.
    template<typename Type>
    struct is_applicative_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename applicative_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

NS_END  // internal


// is_applicative
//   trait: true if Type has a specialization of applicative_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic applicative
// operations.
template<typename Type>
struct is_applicative
    : internal::is_applicative_helper<typename std::decay<Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_applicative_v
//   value: convenience alias for is_applicative<Type>::value.
template<typename Type>
static constexpr bool is_applicative_v = is_applicative<Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the applicative protocol, built
// on the core ::void_t SFINAE sink declared in djinterp.hpp. is_applicative
// (above, kept where the protocol is introduced) answers "does an
// applicative_traits specialization exist?"; the traits here answer the
// finer-grained questions generic code depends on: what is the inner value
// type, and is a given (wrapped function, wrapped argument) pair applicable
// via ap. Each predicate reduces to a `static constexpr bool value` (or a
// `::type` for the type-yielding trait). The C++20 concepts close the
// section. Internal helpers carry a unique applicative_ prefix so the
// umbrella build never collides them with the like-named helpers in
// monad.hpp / functor.hpp.

NS_INTERNAL

    // applicative_value_type_helper
    //   helper: SFINAE extractor for applicative_traits<F>::value_type
    // (primary: no `type`, soft failure).
    template<typename AlwaysVoid,
             typename Applicative>
    struct applicative_value_type_helper
    {};

    // applicative_value_type_helper (well-formed specialization)
    //   helper: yields applicative_traits<F>::value_type when present.
    template<typename Applicative>
    struct applicative_value_type_helper<
        void_t<typename applicative_traits<Applicative>::value_type>,
        Applicative>
    {
        using type = typename applicative_traits<Applicative>::value_type;
    };

    // is_applicable_helper
    //   helper: detection sink for a well-formed ap(Ff, Fa) (primary:
    // false). The well-formed specialization is defined after ap below.
    template<typename AlwaysVoid,
             typename WrappedFn,
             typename FunctorA>
    struct is_applicable_helper : std::false_type
    {};

NS_END  // internal


// applicative_value_type
//   trait: the inner value type T of an applicative F, i.e.
// applicative_traits<F>::value_type. SFINAE-friendly: has a `::type` only
// when F is a specialized applicative.
template<typename Applicative>
struct applicative_value_type
{
    using type = typename internal::applicative_value_type_helper<
        void, typename std::decay<Applicative>::type>::type;
};

// applicative_value_type_t
//   type: convenience alias for applicative_value_type<F>::type.
template<typename Applicative>
using applicative_value_type_t =
    typename applicative_value_type<Applicative>::type;


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC APPLICATIVE OPERATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN. pure / ap delegate to applicative_traits<F>; lift_a2 is
// derived from functor_map followed by ap and so is written once for every
// applicative. For a monad context they fold exactly where monad_bind /
// monad_map do (compile time under C++20 over a carrier-holding maybe /
// result, runtime on the C++17 floor). For a view / producer context, ap is
// whatever that module's explicit specialization supplies.

NS_INTERNAL

    // applicative_a2_binder
    //   helper: the inner stage of lift_a2's currying. Stores the binary
    // function f and a fixed first argument a, and applied to a second
    // argument b yields f(a, b) -- i.e. `\b -> f(a, b)`. A named class (not
    // a lambda) so it can appear in trailing return types on every floor.
    template<typename Function,
             typename First>
    class applicative_a2_binder
    {
    public:
        template<typename FnFwd,
                 typename FirstFwd>
        D_CONSTEXPR
        applicative_a2_binder(
            FnFwd&&    _function,
            FirstFwd&& _first
        )
            : m_function(std::forward<FnFwd>(_function))
            , m_first(std::forward<FirstFwd>(_first))
        {}

        template<typename Second>
        D_CONSTEXPR
        auto operator()(
            const Second& _second
        ) const
        -> decltype(std::declval<const Function&>()(
               std::declval<const First&>(), _second))
        {
            return m_function(m_first, _second);
        }

    private:
        Function m_function;
        First     m_first;
    };

    // applicative_a2_curry
    //   helper: the outer stage of lift_a2's currying. Stores the binary
    // function f and, applied to a first argument a, yields the inner
    // binder `\b -> f(a, b)` -- i.e. `\a -> \b -> f(a, b)`. Mapping this over
    // the first context turns F<A> into F<B -> C>, ready for ap with F<B>.
    template<typename Function>
    class applicative_a2_curry
    {
    public:
        // Self-type guard, as in applicative_ap_binder: this single-argument
        // forwarding constructor must not shadow copy / move when the curry
        // is duplicated (functor_map over the first context copies it).
        template<typename FnFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<FnFwd>::type,
                         applicative_a2_curry>::value>::type>
        D_CONSTEXPR
        explicit applicative_a2_curry(
            FnFwd&& _function
        )
            : m_function(std::forward<FnFwd>(_function))
        {}

        template<typename First>
        D_CONSTEXPR
        applicative_a2_binder<Function, typename std::decay<First>::type>
        operator()(
            const First& _first
        ) const
        {
            return applicative_a2_binder<
                Function, typename std::decay<First>::type>(
                    m_function, _first);
        }

    private:
        Function m_function;
    };

NS_END  // internal


// pure
//   function: lifts a plain value into an applicative context. The
// applicative type Applicative must be supplied explicitly because there
// is no way to deduce F<T> from T alone (the dual of monad_unit).
//
//   Example: pure<maybe<int>>(5) -> just(5)
template<typename Applicative,
         typename Value>
D_NODISCARD
D_CONSTEXPR
auto pure
(
    Value&& _value
)
-> decltype(applicative_traits<Applicative>::pure(
       std::forward<Value>(_value)))
{
    return applicative_traits<Applicative>::pure(
        std::forward<Value>(_value));
}


// ap
//   function: applies a wrapped function to a wrapped argument,
//   F<a -> b> -> F<a> -> F<b> (Haskell `<*>`). Both contexts must be the
// same applicative F; the operation is delegated to applicative_traits<F>::ap
// keyed on the wrapped-function context. For maybe / result this short-
// circuits: a nothing / err on either side propagates.
template<typename WrappedFn,
         typename FunctorA>
D_NODISCARD
D_CONSTEXPR
auto ap
(
    WrappedFn&& _ff,
    FunctorA&&  _fa
)
-> decltype(applicative_traits<typename std::decay<WrappedFn>::type>::ap(
       std::forward<WrappedFn>(_ff),
       std::forward<FunctorA>(_fa)))
{
    return applicative_traits<typename std::decay<WrappedFn>::type>::ap(
        std::forward<WrappedFn>(_ff),
        std::forward<FunctorA>(_fa));
}


NS_INTERNAL

    // is_applicable_helper (well-formed specialization)
    //   helper: true when ap(Ff, Fa) is a valid expression.
    template<typename WrappedFn,
             typename FunctorA>
    struct is_applicable_helper<
        void_t<decltype(::djinterp::ap(
            std::declval<WrappedFn>(), std::declval<FunctorA>()))>,
        WrappedFn,
        FunctorA> : std::true_type
    {};

NS_END  // internal


// is_applicable
//   trait: true when ap(declval<Ff>(), declval<Fa>()) is a well-formed
// expression.
template<typename WrappedFn,
         typename FunctorA>
struct is_applicable
    : internal::is_applicable_helper<void, WrappedFn, FunctorA>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_applicable_v
    //   value: convenience alias for is_applicable<...>::value.
    template<typename WrappedFn,
             typename FunctorA>
    constexpr bool is_applicable_v =
        is_applicable<WrappedFn, FunctorA>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// lift_a2
//   function: applicative-style binary lift. Given a binary function
//   f : (A, B) -> C and two applicatives fa : F<A>, fb : F<B>, produces
//   F<C> by mapping the curried f over fa -- giving F<B -> C> -- and then
// applying that to fb with ap. Derived once here for every applicative, the
// counterpart of the monad layer's lift_m2.
//
//   Left-biased like lift_m2 (fa is mapped first); for maybe / result this
// is observationally irrelevant, but the order is fixed for predictability.
template<typename FunctorA,
         typename FunctorB,
         typename Function>
D_NODISCARD
D_CONSTEXPR
auto lift_a2
(
    FunctorA&& _fa,
    FunctorB&& _fb,
    Function&& _function
)
-> decltype(::djinterp::ap(
       ::djinterp::functor_map(
           std::declval<FunctorA>(),
           std::declval<internal::applicative_a2_curry<
               typename std::decay<Function>::type> >()),
       std::declval<FunctorB>()))
{
    return ::djinterp::ap(
        ::djinterp::functor_map(
            std::forward<FunctorA>(_fa),
            internal::applicative_a2_curry<
                typename std::decay<Function>::type>(
                    std::forward<Function>(_function))),
        std::forward<FunctorB>(_fb));
}


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Applicative
    //   concept: satisfied when Type is a specialized applicative. The
    // PascalCase typeclass face, alongside Functor / Callable / Predicate.
    template<typename Type>
    concept Applicative = is_applicative<Type>::value;

    // applicable_with
    //   concept: satisfied when ap(WrappedFn, FunctorA) is well-formed
    // (mirrors monad's bindable_with and functor's fmappable_with).
    template<typename WrappedFn,
             typename FunctorA>
    concept applicable_with = is_applicable<WrappedFn, FunctorA>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_APPLICATIVE_HPP
