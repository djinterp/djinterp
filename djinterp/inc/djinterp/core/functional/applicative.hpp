/******************************************************************************
* djinterp [functional]                                        applicative.hpp
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    APPLICATIVE PROTOCOL
      1.  applicative_traits<F>                   (primary, undefined)
      2.  applicative_traits<F> [monad bridge]    (every monad is applicative)
      3.  is_applicative<T>                        (detection trait)
II.   GENERIC APPLICATIVE OPERATIONS
      1.  pure<F>                                  (lift value into F)
      2.  ap                                       (F<a->b> -> F<a> -> F<b>)
      3.  lift_a2                                  (binary applicative lift)
*/


#ifndef DJINTERP_FUNCTIONAL_APPLICATIVE_
#define DJINTERP_FUNCTIONAL_APPLICATIVE_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
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
    template<typename _FunctorA>
    class applicative_ap_binder
    {
    public:
        // The self-type guard keeps this single-argument forwarding
        // constructor from outcompeting the implicit copy / move
        // constructors when the binder itself is copied (the bridge's map
        // takes its function by value) -- the single-argument analogue of
        // why kleisli_helper's two-argument constructor is collision-free.
        template<typename _FaFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<_FaFwd>::type,
                         applicative_ap_binder>::value>::type>
        D_CONSTEXPR
        explicit applicative_ap_binder(
            _FaFwd&& _fa
        )
            : m_fa(std::forward<_FaFwd>(_fa))
        {}

        template<typename _Function>
        D_CONSTEXPR
        auto operator()(
            const _Function& _function
        ) const
        -> decltype(::djinterp::monad_map(
               std::declval<const _FunctorA&>(), _function))
        {
            return ::djinterp::monad_map(m_fa, _function);
        }

    private:
        _FunctorA m_fa;
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
template<typename _Applicative,
         typename _Enable = void>
struct applicative_traits;


// applicative_traits<_Applicative> (monad bridge)
//   specialization: every monad is an applicative. Keyed on is_monad, this
// derives pure from the monad's unit and ap from bind + map, so maybe,
// result, and any future monad participate as applicatives with no per-type
// specialization. A view / producer is not a monad, so its explicit
// specialization (in its own header) never overlaps this one.
template<typename _Applicative>
struct applicative_traits<
    _Applicative,
    typename std::enable_if<is_monad<_Applicative>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename monad_value_type<_Applicative>::type;

    template<typename _To>
    using rebind = typename monad_rebind<_Applicative, _To>::type;

    // pure
    //   lift a bare value into the applicative via the monad's unit.
    template<typename _Value>
    static
    D_CONSTEXPR
    auto pure(
        _Value&& _value
    )
    -> decltype(::djinterp::monad_unit<_Applicative>(
           std::forward<_Value>(_value)))
    {
        return ::djinterp::monad_unit<_Applicative>(
            std::forward<_Value>(_value));
    }

    // ap
    //   wrapped application via the monad protocol:
    //   ap(ff, fa) = bind ff (\f -> map f fa).
    // D_CONSTEXPR follows monad_bind / monad_map: it folds at compile time
    // under C++20 over a monad whose value is a carrier leaf, and runs at
    // runtime on the C++17 floor where maybe / result are not literal types.
    template<typename _WrappedFn,
             typename _FunctorA>
    static
    D_CONSTEXPR
    auto ap(
        _WrappedFn&& _ff,
        _FunctorA&&  _fa
    )
    -> decltype(::djinterp::monad_bind(
           std::forward<_WrappedFn>(_ff),
           internal::applicative_ap_binder<
               typename std::decay<_FunctorA>::type>(
                   std::forward<_FunctorA>(_fa))))
    {
        return ::djinterp::monad_bind(
            std::forward<_WrappedFn>(_ff),
            internal::applicative_ap_binder<
                typename std::decay<_FunctorA>::type>(
                    std::forward<_FunctorA>(_fa)));
    }
};


NS_INTERNAL

    // is_applicative_helper
    //   helper: SFINAE detector for whether applicative_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization (including the monad bridge) provides.
    template<typename _Type>
    struct is_applicative_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename applicative_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_applicative
//   trait: true if _Type has a specialization of applicative_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic applicative
// operations.
template<typename _Type>
struct is_applicative
    : internal::is_applicative_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_applicative_v
//   value: convenience alias for is_applicative<_Type>::value.
template<typename _Type>
static constexpr bool is_applicative_v = is_applicative<_Type>::value;
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
    template<typename _AlwaysVoid,
             typename _Applicative>
    struct applicative_value_type_helper
    {};

    // applicative_value_type_helper (well-formed specialization)
    //   helper: yields applicative_traits<F>::value_type when present.
    template<typename _Applicative>
    struct applicative_value_type_helper<
        void_t<typename applicative_traits<_Applicative>::value_type>,
        _Applicative>
    {
        using type = typename applicative_traits<_Applicative>::value_type;
    };

    // is_applicable_helper
    //   helper: detection sink for a well-formed ap(Ff, Fa) (primary:
    // false). The well-formed specialization is defined after ap below.
    template<typename _AlwaysVoid,
             typename _WrappedFn,
             typename _FunctorA>
    struct is_applicable_helper : std::false_type
    {};

NS_END  // internal


// applicative_value_type
//   trait: the inner value type T of an applicative F, i.e.
// applicative_traits<F>::value_type. SFINAE-friendly: has a `::type` only
// when F is a specialized applicative.
template<typename _Applicative>
struct applicative_value_type
{
    using type = typename internal::applicative_value_type_helper<
        void, typename std::decay<_Applicative>::type>::type;
};

// applicative_value_type_t
//   type: convenience alias for applicative_value_type<F>::type.
template<typename _Applicative>
using applicative_value_type_t =
    typename applicative_value_type<_Applicative>::type;


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
    template<typename _Function,
             typename _First>
    class applicative_a2_binder
    {
    public:
        template<typename _FnFwd,
                 typename _FirstFwd>
        D_CONSTEXPR
        applicative_a2_binder(
            _FnFwd&&    _function,
            _FirstFwd&& _first
        )
            : m_function(std::forward<_FnFwd>(_function))
            , m_first(std::forward<_FirstFwd>(_first))
        {}

        template<typename _Second>
        D_CONSTEXPR
        auto operator()(
            const _Second& _second
        ) const
        -> decltype(std::declval<const _Function&>()(
               std::declval<const _First&>(), _second))
        {
            return m_function(m_first, _second);
        }

    private:
        _Function m_function;
        _First    m_first;
    };

    // applicative_a2_curry
    //   helper: the outer stage of lift_a2's currying. Stores the binary
    // function f and, applied to a first argument a, yields the inner
    // binder `\b -> f(a, b)` -- i.e. `\a -> \b -> f(a, b)`. Mapping this over
    // the first context turns F<A> into F<B -> C>, ready for ap with F<B>.
    template<typename _Function>
    class applicative_a2_curry
    {
    public:
        // Self-type guard, as in applicative_ap_binder: this single-argument
        // forwarding constructor must not shadow copy / move when the curry
        // is duplicated (functor_map over the first context copies it).
        template<typename _FnFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<_FnFwd>::type,
                         applicative_a2_curry>::value>::type>
        D_CONSTEXPR
        explicit applicative_a2_curry(
            _FnFwd&& _function
        )
            : m_function(std::forward<_FnFwd>(_function))
        {}

        template<typename _First>
        D_CONSTEXPR
        applicative_a2_binder<_Function, typename std::decay<_First>::type>
        operator()(
            const _First& _first
        ) const
        {
            return applicative_a2_binder<
                _Function, typename std::decay<_First>::type>(
                    m_function, _first);
        }

    private:
        _Function m_function;
    };

NS_END  // internal


// pure
//   function: lifts a plain value into an applicative context. The
// applicative type _Applicative must be supplied explicitly because there
// is no way to deduce F<T> from T alone (the dual of monad_unit).
//
//   Example: pure<maybe<int>>(5) -> just(5)
template<typename _Applicative,
         typename _Value>
D_NODISCARD
D_CONSTEXPR
auto pure
(
    _Value&& _value
)
-> decltype(applicative_traits<_Applicative>::pure(
       std::forward<_Value>(_value)))
{
    return applicative_traits<_Applicative>::pure(
        std::forward<_Value>(_value));
}


// ap
//   function: applies a wrapped function to a wrapped argument,
//   F<a -> b> -> F<a> -> F<b> (Haskell `<*>`). Both contexts must be the
// same applicative F; the operation is delegated to applicative_traits<F>::ap
// keyed on the wrapped-function context. For maybe / result this short-
// circuits: a nothing / err on either side propagates.
template<typename _WrappedFn,
         typename _FunctorA>
D_NODISCARD
D_CONSTEXPR
auto ap
(
    _WrappedFn&& _ff,
    _FunctorA&&  _fa
)
-> decltype(applicative_traits<typename std::decay<_WrappedFn>::type>::ap(
       std::forward<_WrappedFn>(_ff),
       std::forward<_FunctorA>(_fa)))
{
    return applicative_traits<typename std::decay<_WrappedFn>::type>::ap(
        std::forward<_WrappedFn>(_ff),
        std::forward<_FunctorA>(_fa));
}


NS_INTERNAL

    // is_applicable_helper (well-formed specialization)
    //   helper: true when ap(Ff, Fa) is a valid expression.
    template<typename _WrappedFn,
             typename _FunctorA>
    struct is_applicable_helper<
        void_t<decltype(::djinterp::ap(
            std::declval<_WrappedFn>(), std::declval<_FunctorA>()))>,
        _WrappedFn,
        _FunctorA> : std::true_type
    {};

NS_END  // internal


// is_applicable
//   trait: true when ap(declval<Ff>(), declval<Fa>()) is a well-formed
// expression.
template<typename _WrappedFn,
         typename _FunctorA>
struct is_applicable
    : internal::is_applicable_helper<void, _WrappedFn, _FunctorA>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_applicable_v
    //   value: convenience alias for is_applicable<...>::value.
    template<typename _WrappedFn,
             typename _FunctorA>
    constexpr bool is_applicable_v =
        is_applicable<_WrappedFn, _FunctorA>::value;

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
template<typename _FunctorA,
         typename _FunctorB,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto lift_a2
(
    _FunctorA&& _fa,
    _FunctorB&& _fb,
    _Function&& _function
)
-> decltype(::djinterp::ap(
       ::djinterp::functor_map(
           std::declval<_FunctorA>(),
           std::declval<internal::applicative_a2_curry<
               typename std::decay<_Function>::type> >()),
       std::declval<_FunctorB>()))
{
    return ::djinterp::ap(
        ::djinterp::functor_map(
            std::forward<_FunctorA>(_fa),
            internal::applicative_a2_curry<
                typename std::decay<_Function>::type>(
                    std::forward<_Function>(_function))),
        std::forward<_FunctorB>(_fb));
}


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Applicative
    //   concept: satisfied when _Type is a specialized applicative. The
    // PascalCase typeclass face, alongside Functor / Callable / Predicate.
    template<typename _Type>
    concept Applicative = is_applicative<_Type>::value;

    // applicable_with
    //   concept: satisfied when ap(_WrappedFn, _FunctorA) is well-formed
    // (mirrors monad's bindable_with and functor's fmappable_with).
    template<typename _WrappedFn,
             typename _FunctorA>
    concept applicable_with = is_applicable<_WrappedFn, _FunctorA>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_APPLICATIVE_
