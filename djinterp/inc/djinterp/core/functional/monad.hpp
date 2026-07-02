/******************************************************************************
* djinterp [functional]                                              monad.hpp
*
* Monad protocol and generic monadic operations (C++).
*   Defines a trait-based protocol that maybe<T>, result<T, E>, and any
* future monadic types can implement. A monad here is a type constructor
* M<T> plus three operations: unit<T>(x) producing M<T>, bind(M<T>, f)
* threading a value through f : T -> M<U>, and join(M<M<T>>) flattening
* one layer. From these, map (functorial), kleisli composition, and
* lift_m2 (applicative-style) are derived.
*
*   Because C++ has no native type classes, monads are recognized by
* specializing monad_traits<M> with the three primitive operations.
* Each concrete monad supplies its own specialization in its own header.
*
*   The predicate SFINAE structural traits and C++20 concepts in
* Section 0 describe the protocol vocabulary -- monad-ness, the value
* and rebind types, Kleisli-arrow shape, bindability / mappability, and
* combinator shape -- so generic code can constrain and introspect on
* it without first naming a concrete monad.
*
*   Operator| is overloaded for (M<T>) | bind_with(f) and similar
* combinators, enabling the same pipeline syntax already used by
* view.hpp.
*
* USAGE:
*   maybe<int> x = just(5);
*
*   auto r = x
*          | bind_with([](int v) { return safe_divide(100, v); }) // maybe<int>
*          | map_with([](int v) { return v * 2; })                // maybe<int>
*          | or_else_with(0);                                     // int
*
*   // Generic over any monad: works on maybe, result, future, ...
*   template<typename _Monad>
*   _Monad pure_chain(_Monad _m)
*   {
*       return monad_map(_m, [](auto v) { return v + 1; });
*   }
*
* 
* path:      /inc/djinterp/core/functional/monad.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    MONAD PROTOCOL
      1.  monad_traits<M>                         (primary, undefined)
      2.  is_monad<T>                             (detection trait)
II.   GENERIC MONADIC OPERATIONS
      1.  monad_unit<M, T>                        (lift value to M<T>)
      2.  monad_bind                              (>>= in Haskell)
      3.  monad_map                               (fmap, functorial)
      4.  monad_join                              (flatten one layer)
      5.  monad_then                              (>> -- sequence, discard)
      6.  kleisli_compose                         (>=> -- f then g)
      7.  lift_m2                                 (binary applicative lift)
III.  PIPELINE COMBINATORS
      1.  bind_with(f)                            (RHS of operator|)
      2.  map_with(f)
      3.  then_with(other)
IV.   PIPELINE OPERATORS
      1.  operator|(monad, combinator)
*/


#ifndef DJINTERP_FUNCTIONAL_MONAD_
#define DJINTERP_FUNCTIONAL_MONAD_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    MONAD PROTOCOL                                        ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits
//   trait: primary template, undefined by default. Each concrete
// monad specializes monad_traits<M<T>> (or, when M is a class
// template, monad_traits<M<...>>) to expose:
//
//     - value_type    : the inner type T
//     - rebind<U>     : M<U> (for changing the inner type)
//     - unit(x)       : static M<T> unit(T) -- lift a value
//     - bind(m, f)    : static M<U> bind(M<T>, f) where f : T -> M<U>
//     - is_specialized = true_type (marker)
//
//   Specializations live in the concrete monad's own header
// (maybe.hpp, result.hpp, future.hpp, ...). The primary is left
// undefined so that uses with non-monad types produce a clean
// template-resolution error.
template<typename _Monad>
struct monad_traits;


NS_INTERNAL

    // is_monad_helper
    //   helper: SFINAE detector for whether monad_traits<T> is
    // specialized. Looks for the is_specialized marker that all
    // specializations are required to provide.
    template<typename _Type>
    struct is_monad_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename monad_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_monad
//   trait: true if _Type has a specialization of monad_traits.
// Used to SFINAE-constrain generic monadic operations.
template<typename _Type>
struct is_monad
    : internal::is_monad_helper<
          typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
static constexpr bool is_monad_v = is_monad<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the monad protocol, built
// on the core ::void_t SFINAE sink declared in djinterp.hpp.  is_monad
// (above, kept where the protocol is introduced) answers "does a
// monad_traits specialization exist?"; the traits here answer the
// finer-grained questions the generic operations and the operator|
// pipeline depend on: what is the inner value type, what does rebind
// yield, is a function a valid Kleisli arrow / mapping function for a
// monad, is a (monad, function) pair bindable / mappable, and is a
// type a pipeline combinator for a monad.  Each predicate reduces to a
// `static constexpr bool value` (or a `::type` for the type-yielding
// traits).  The C++20 concept mirrors close the section.

NS_INTERNAL

    // monad_value_type_helper
    //   helper: SFINAE extractor for monad_traits<M>::value_type
    // (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Monad>
    struct monad_value_type_helper
    {};

    // monad_value_type_helper (well-formed specialization)
    //   helper: yields monad_traits<M>::value_type when present.
    template<typename _Monad>
    struct monad_value_type_helper<
        void_t<typename monad_traits<_Monad>::value_type>,
        _Monad>
    {
        using type = typename monad_traits<_Monad>::value_type;
    };

    // monad_rebind_helper
    //   helper: SFINAE extractor for monad_traits<M>::rebind<U>
    // (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Monad,
             typename _To>
    struct monad_rebind_helper
    {};

    // monad_rebind_helper (well-formed specialization)
    //   helper: yields monad_traits<M>::rebind<U> when present.
    template<typename _Monad,
             typename _To>
    struct monad_rebind_helper<
        void_t<typename monad_traits<_Monad>::template rebind<_To>>,
        _Monad,
        _To>
    {
        using type = typename monad_traits<_Monad>::template rebind<_To>;
    };

    // call_result_helper
    //   helper: SFINAE extractor for the call expression
    // _Function(_Arg) (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Function,
             typename _Arg>
    struct call_result_helper
    {};

    // call_result_helper (well-formed specialization)
    //   helper: yields the result of _Function(const _Arg&) when the
    // call expression is well-formed.
    template<typename _Function,
             typename _Arg>
    struct call_result_helper<
        void_t<decltype(std::declval<const _Function&>()(
            std::declval<const _Arg&>()))>,
        _Function,
        _Arg>
    {
        using type = decltype(std::declval<const _Function&>()(
            std::declval<const _Arg&>()));
    };

    // is_bindable_helper
    //   helper: detection sink for a well-formed monad_bind(M, F)
    // (primary: false).
    template<typename _AlwaysVoid,
             typename _Monad,
             typename _Function>
    struct is_bindable_helper : std::false_type
    {};

    // is_mappable_helper
    //   helper: detection sink for a well-formed monad_map(M, F)
    // (primary: false).
    template<typename _AlwaysVoid,
             typename _Monad,
             typename _Function>
    struct is_mappable_helper : std::false_type
    {};

    // is_monad_combinator_helper
    //   helper: detection sink for "c.apply(m) is well-formed" --
    // i.e. _Combinator is a pipeline combinator accepting a _Monad
    // (primary: false).
    template<typename _AlwaysVoid,
             typename _Combinator,
             typename _Monad>
    struct is_monad_combinator_helper : std::false_type
    {};

    // is_monadic_function_helper
    //   helper: detection sink for the Kleisli-arrow shape -- true
    // when _Function is callable on _ValueType and the (decayed)
    // result is itself a monad.  Primary: false.  All ill-formed
    // sub-expressions are confined to the specialization's match,
    // so the primary is reached cleanly for any non-arrow case.
    template<typename _AlwaysVoid,
             typename _Function,
             typename _Monad>
    struct is_monadic_function_helper : std::false_type
    {};

NS_END  // internal

// monad_value_type
//   trait: the inner value type T of a monad M, i.e.
// monad_traits<M>::value_type.  SFINAE-friendly: has a `::type` only
// when M is a specialized monad.
template<typename _Monad>
struct monad_value_type
{
    using type = typename internal::monad_value_type_helper<
        void, typename std::decay<_Monad>::type>::type;
};

// monad_value_type_t
//   type: convenience alias for monad_value_type<M>::type.
template<typename _Monad>
using monad_value_type_t = typename monad_value_type<_Monad>::type;

// monad_rebind
//   trait: the monad M re-parameterized over a new inner type U, i.e.
// monad_traits<M>::rebind<U>.  SFINAE-friendly.
template<typename _Monad,
         typename _To>
struct monad_rebind
{
    using type = typename internal::monad_rebind_helper<
        void, typename std::decay<_Monad>::type, _To>::type;
};

// monad_rebind_t
//   type: convenience alias for monad_rebind<M, U>::type.
template<typename _Monad,
         typename _To>
using monad_rebind_t = typename monad_rebind<_Monad, _To>::type;

// is_monadic_function
//   trait: true when _Function is a valid Kleisli arrow for _Monad --
// callable with the monad's value type and yielding a (decayed)
// result that is itself a monad.  SFINAE-safe for every argument: a
// non-monad _Monad, a non-callable _Function, or a non-monad result
// all resolve cleanly to false.
template<typename _Function,
         typename _Monad>
struct is_monadic_function
    : internal::is_monadic_function_helper<
          void, _Function, typename std::decay<_Monad>::type>
{};

// is_bindable
//   trait: true when monad_bind(declval<M>(), declval<F>()) is a
// well-formed expression.
template<typename _Monad,
         typename _Function>
struct is_bindable
    : internal::is_bindable_helper<void, _Monad, _Function>
{};

// is_mappable
//   trait: true when monad_map(declval<M>(), declval<F>()) is a
// well-formed expression.
template<typename _Monad,
         typename _Function>
struct is_mappable
    : internal::is_mappable_helper<void, _Monad, _Function>
{};

// is_monad_combinator
//   trait: true when _Combinator exposes an apply(_Monad) member that
// is callable -- the structural shape every pipeline combinator
// (bind_combinator, map_combinator, then_combinator) satisfies.
template<typename _Combinator,
         typename _Monad>
struct is_monad_combinator
    : internal::is_monad_combinator_helper<void, _Combinator, _Monad>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_monadic_function_v
    //   value: convenience alias for is_monadic_function<...>::value.
    template<typename _Function,
             typename _Monad>
    constexpr bool is_monadic_function_v =
        is_monadic_function<_Function, _Monad>::value;

    // is_bindable_v
    //   value: convenience alias for is_bindable<...>::value.
    template<typename _Monad,
             typename _Function>
    constexpr bool is_bindable_v = is_bindable<_Monad, _Function>::value;

    // is_mappable_v
    //   value: convenience alias for is_mappable<...>::value.
    template<typename _Monad,
             typename _Function>
    constexpr bool is_mappable_v = is_mappable<_Monad, _Function>::value;

    // is_monad_combinator_v
    //   value: convenience alias for is_monad_combinator<...>::value.
    template<typename _Combinator,
             typename _Monad>
    constexpr bool is_monad_combinator_v =
        is_monad_combinator<_Combinator, _Monad>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // monad
    //   concept: satisfied when _Type is a specialized monad.
    template<typename _Type>
    concept monad = is_monad<_Type>::value;

    // monadic_function_for
    //   concept: satisfied when _Function is a Kleisli arrow for the
    // monad _Monad (callable on its value type, returning a monad).
    template<typename _Function,
             typename _Monad>
    concept monadic_function_for =
        is_monadic_function<_Function, _Monad>::value;

    // bindable_with
    //   concept: satisfied when monad_bind(_Monad, _Function) is
    // well-formed.
    template<typename _Monad,
             typename _Function>
    concept bindable_with = is_bindable<_Monad, _Function>::value;

    // mappable_with
    //   concept: satisfied when monad_map(_Monad, _Function) is
    // well-formed.
    template<typename _Monad,
             typename _Function>
    concept mappable_with = is_mappable<_Monad, _Function>::value;

    // monad_combinator_for
    //   concept: satisfied when _Combinator can be applied to _Monad
    // (the operator| pipeline RHS shape).
    template<typename _Combinator,
             typename _Monad>
    concept monad_combinator_for =
        is_monad_combinator<_Combinator, _Monad>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC MONADIC OPERATIONS                            ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN.  The operations below are D_CONSTEXPR and delegate to
// monad_traits<M>::{unit, bind}.  When a concrete monad marks those hooks
// constexpr (maybe and result mark theirs D_CONSTEXPR), these operations
// fold at compile time under C++20 over a monad whose value is a carrier leaf
// (val_t / type_t) - so the same monad_bind / monad_map expresses both a
// value-domain runtime computation and a type- or value-level compile-time
// one, with no separate type-level reimplementation.  On the C++17 floor the
// monadic types maybe / result are not literal types, so the protocol runs at
// runtime there.

// monad_unit
//   function: lifts a plain value into a monadic context. The
// monad type _Monad must be supplied explicitly because there is
// no way to deduce M<T> from T alone.
//
//   Example: monad_unit<maybe<int>>(5) -> just(5)
template<typename _Monad,
         typename _Value>
D_NODISCARD
D_CONSTEXPR
auto monad_unit
(
    _Value&& _value
)
-> decltype(monad_traits<_Monad>::unit(std::forward<_Value>(_value)))
{
    return monad_traits<_Monad>::unit(std::forward<_Value>(_value));
}


// monad_bind
//   function: threads the value inside _monad through _function,
// which must return a monad of the same kind. Semantically:
//   bind(m, f) = if m is empty/failed, return that; else return f(m.value()).
//
//   The specific behavior is delegated to monad_traits<M>::bind.
template<typename _Monad,
         typename _Function>
D_NODISCARD D_CONSTEXPR auto 
monad_bind(
    _Monad&&    _monad,
    _Function&& _function
)
-> decltype(monad_traits<typename std::decay<_Monad>::type>::bind(
       std::forward<_Monad>(_monad),
       std::forward<_Function>(_function)))
{
    return monad_traits<typename std::decay<_Monad>::type>::bind(
        std::forward<_Monad>(_monad),
        std::forward<_Function>(_function));
}


// monad_map
//   function: functorial map. Applies _function : T -> U to the
// value inside the monad, yielding a monad of U with the same
// shape (just/nothing or ok/err). Implemented in terms of bind
// and unit so concrete monads need only provide those two.
template<typename _Monad,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto monad_map
(
    _Monad&&    _monad,
    _Function&& _function
)
-> typename monad_traits<typename std::decay<_Monad>::type>::template rebind<
       typename std::decay<decltype(
           std::declval<_Function&>()(
               std::declval<const typename monad_traits<
                   typename std::decay<_Monad>::type>::value_type&>()))>::type>
{
    using monad_t      = typename std::decay<_Monad>::type;
    using traits       = monad_traits<monad_t>;
    using inner_t      = typename traits::value_type;
    using mapped_t     = typename std::decay<decltype(
        std::declval<_Function&>()(std::declval<const inner_t&>()))>::type;
    using rebound_t    = typename traits::template rebind<mapped_t>;

    return monad_bind(
        std::forward<_Monad>(_monad),
        [_function](const inner_t& _v) {
            return monad_traits<rebound_t>::unit(_function(_v));
        });
}


// monad_join
//   function: flattens a monad of monads. join(M<M<T>>) -> M<T>.
// Implemented in terms of bind with identity.
//
//   The return type is the outer monad's value_type -- which, for a
// monad of monads, IS the inner monad M<T>.  (The previous spelling
// passed a value_type *value* as bind's function argument inside the
// trailing return type, which is ill-formed for any monad whose bind
// requires a callable; the body's identity lambda was always correct.)
template<typename _OuterMonad>
D_NODISCARD
D_CONSTEXPR
auto monad_join
(
    _OuterMonad&& _monad
)
-> typename monad_traits<
       typename std::decay<_OuterMonad>::type>::value_type
{
    using inner_monad_t = typename monad_traits<
        typename std::decay<_OuterMonad>::type>::value_type;

    return monad_bind(
        std::forward<_OuterMonad>(_monad),
        [](const inner_monad_t& _inner) { return _inner; });
}


// monad_then
//   function: sequence two monads, discarding the value of the
// first. then(m1, m2) is equivalent to bind(m1, [m2](_) { return m2; }).
// Useful for short-circuiting failure propagation while ignoring
// successful intermediate values.
//
//   The return type is the decayed second monad type.  (The previous
// spelling passed a _Monad2 *value* as bind's function argument inside
// the trailing return type, which is ill-formed for any monad whose
// bind requires a callable; the body's capture lambda was correct.)
template<typename _Monad1,
         typename _Monad2>
D_NODISCARD
D_CONSTEXPR
auto monad_then
(
    _Monad1&& _first,
    _Monad2&& _second
)
-> typename std::decay<_Monad2>::type
{
    using inner_t = typename monad_traits<
        typename std::decay<_Monad1>::type>::value_type;
    using second_t = typename std::decay<_Monad2>::type;

    second_t second_copy = _second;

    return monad_bind(
        std::forward<_Monad1>(_first),
        [second_copy](const inner_t&) { return second_copy; });
}


NS_INTERNAL

    // kleisli_helper
    //   helper: stores two monadic functions and threads a value
    // through both via monad_bind. Used by kleisli_compose to
    // avoid the C++14 generic-lambda formulation.
    template<typename _F,
             typename _G>
    class kleisli_helper
    {
    public:
        template<typename _FFwd,
                 typename _GFwd>
        D_CONSTEXPR
        kleisli_helper(
            _FFwd&& _f,
            _GFwd&& _g
        )
            : m_f(std::forward<_FFwd>(_f))
            , m_g(std::forward<_GFwd>(_g))
        {}

        template<typename _Input>
        D_CONSTEXPR
        auto operator()(
            _Input&& _input
        ) const
        -> decltype(monad_bind(
               std::declval<const _F&>()(std::forward<_Input>(_input)),
               std::declval<const _G&>()))
        {
            return monad_bind(m_f(std::forward<_Input>(_input)), m_g);
        }

    private:
        _F m_f;
        _G m_g;
    };

NS_END  // internal


// kleisli_compose
//   function: composes two monadic functions f : A -> M<B> and
// g : B -> M<C> into a single function A -> M<C> that threads
// through the monad. The Haskell notation is f >=> g.
//
//   Returns a callable object that, on each application, binds
// the result of f into g. Both functions are captured by value.
template<typename _F,
         typename _G>
D_NODISCARD
D_CONSTEXPR
internal::kleisli_helper<typename std::decay<_F>::type,
                         typename std::decay<_G>::type>
kleisli_compose
(
    _F&& _f,
    _G&& _g
)
{
    return internal::kleisli_helper<
        typename std::decay<_F>::type,
        typename std::decay<_G>::type>(
            std::forward<_F>(_f),
            std::forward<_G>(_g));
}


// lift_m2
//   function: applicative-style binary lift. Given a binary
// function f : (A, B) -> C and two monads m_a, m_b, produces
// M<C> by binding into m_a, then binding into m_b, then unit-ing
// the result of f.
//
//   Note: this is left-biased (m_a is unwrapped first); for
// commutative monads (maybe, result on success), order is
// observationally irrelevant, but for state-bearing monads the
// difference matters.
//
//   The return type is M<C> = rebind<C> of the first monad, where C
// is the result of f(A, B).  (The previous spelling passed the binary
// _Function as bind's unary function argument inside the trailing
// return type, which is ill-formed: bind expects a unary A -> M<U>.)
template<typename _MonadA,
         typename _MonadB,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto lift_m2
(
    _MonadA&&   _ma,
    _MonadB&&   _mb,
    _Function&& _function
)
-> typename monad_traits<typename std::decay<_MonadA>::type>::template rebind<
       typename std::decay<decltype(
           std::declval<_Function&>()(
               std::declval<const typename monad_traits<
                   typename std::decay<_MonadA>::type>::value_type&>(),
               std::declval<const typename monad_traits<
                   typename std::decay<_MonadB>::type>::value_type&>()))>::type>
{
    using ma_t    = typename std::decay<_MonadA>::type;
    using mb_t    = typename std::decay<_MonadB>::type;
    using a_t     = typename monad_traits<ma_t>::value_type;
    using b_t     = typename monad_traits<mb_t>::value_type;
    using c_t     = typename std::decay<decltype(
        std::declval<_Function&>()(
            std::declval<const a_t&>(),
            std::declval<const b_t&>()))>::type;
    using rebound = typename monad_traits<ma_t>::template rebind<c_t>;

    mb_t      mb_copy = _mb;
    _Function fn_copy = _function;

    return monad_bind(
        std::forward<_MonadA>(_ma),
        [mb_copy, fn_copy](const a_t& _a) {
            return monad_bind(
                mb_copy,
                [_a, fn_copy](const b_t& _b) {
                    return monad_traits<rebound>::unit(fn_copy(_a, _b));
                });
        });
}


///////////////////////////////////////////////////////////////////////////////
///             III.  PIPELINE COMBINATORS                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // bind_combinator
    //   helper: stores a monadic function for later application
    // via operator|. The function type _Function is captured by
    // value; copy semantics depend on the function's own.
    template<typename _Function>
    class bind_combinator
    {
    public:
        template<typename _FnFwd>
        D_CONSTEXPR
        explicit bind_combinator(
            _FnFwd&& _function
        )
            : m_function(std::forward<_FnFwd>(_function))
        {}

        // apply
        //   forwards the monadic value to monad_bind with the
        // stored function.
        template<typename _Monad>
        D_CONSTEXPR
        auto apply(
            _Monad&& _monad
        ) const
        -> decltype(monad_bind(
               std::forward<_Monad>(_monad),
               std::declval<const _Function&>()))
        {
            return monad_bind(
                std::forward<_Monad>(_monad), m_function);
        }

    private:
        _Function m_function;
    };


    // map_combinator
    //   helper: as bind_combinator, but invokes monad_map. Used
    // when the function does not produce a monad and only the
    // inner value needs transformation.
    template<typename _Function>
    class map_combinator
    {
    public:
        template<typename _FnFwd>
        D_CONSTEXPR
        explicit map_combinator(
            _FnFwd&& _function
        )
            : m_function(std::forward<_FnFwd>(_function))
        {}

        template<typename _Monad>
        D_CONSTEXPR
        auto apply(
            _Monad&& _monad
        ) const
        -> decltype(monad_map(
               std::forward<_Monad>(_monad),
               std::declval<const _Function&>()))
        {
            return monad_map(
                std::forward<_Monad>(_monad), m_function);
        }

    private:
        _Function m_function;
    };


    // then_combinator
    //   helper: stores a second monad to be sequenced after the
    // LHS via monad_then.
    template<typename _Monad>
    class then_combinator
    {
    public:
        template<typename _MFwd>
        D_CONSTEXPR
        explicit then_combinator(
            _MFwd&& _monad
        )
            : m_monad(std::forward<_MFwd>(_monad))
        {}

        template<typename _OtherMonad>
        D_CONSTEXPR
        auto apply(
            _OtherMonad&& _other
        ) const
        -> decltype(monad_then(
               std::forward<_OtherMonad>(_other),
               std::declval<const _Monad&>()))
        {
            return monad_then(std::forward<_OtherMonad>(_other), m_monad);
        }

    private:
        _Monad m_monad;
    };

    // is_bindable_helper (well-formed specialization)
    //   helper: true when monad_bind(M, F) is a valid expression.
    template<typename _Monad,
             typename _Function>
    struct is_bindable_helper<
        void_t<decltype(::djinterp::monad_bind(
            std::declval<_Monad>(), std::declval<_Function>()))>,
        _Monad,
        _Function> : std::true_type
    {};

    // is_mappable_helper (well-formed specialization)
    //   helper: true when monad_map(M, F) is a valid expression.
    template<typename _Monad,
             typename _Function>
    struct is_mappable_helper<
        void_t<decltype(::djinterp::monad_map(
            std::declval<_Monad>(), std::declval<_Function>()))>,
        _Monad,
        _Function> : std::true_type
    {};

    // is_monad_combinator_helper (well-formed specialization)
    //   helper: true when _Combinator.apply(_Monad) is a valid
    // expression (the pipeline combinator shape).
    template<typename _Combinator,
             typename _Monad>
    struct is_monad_combinator_helper<
        void_t<decltype(std::declval<const _Combinator&>().apply(
            std::declval<_Monad>()))>,
        _Combinator,
        _Monad> : std::true_type
    {};

    // is_monadic_function_helper (well-formed specialization)
    //   helper: matches when _Function is callable on _Monad's value
    // type AND that (decayed) result is itself a monad.  Both
    // conditions live inside the void_t, so failure of either falls
    // back to the false primary.
    template<typename _Function,
             typename _Monad>
    struct is_monadic_function_helper<
        typename std::enable_if<
            ::djinterp::is_monad<
                typename std::decay<
                    typename call_result_helper<
                        void,
                        _Function,
                        typename monad_value_type_helper<
                            void, _Monad>::type>::type>::type>::value
        >::type,
        _Function,
        _Monad> : std::true_type
    {};

NS_END  // internal


// bind_with
//   function: builds a combinator that, when piped against a
// monad, threads the monad's value through _function (which
// must return a monad of the same kind).
//   Usage:  m | bind_with([](int x) { return just(x + 1); })
template<typename _Function>
D_NODISCARD
D_CONSTEXPR
internal::bind_combinator<typename std::decay<_Function>::type>
bind_with
(
    _Function&& _function
)
{
    return internal::bind_combinator<
        typename std::decay<_Function>::type>(
            std::forward<_Function>(_function));
}


// map_with
//   function: builds a combinator that, when piped against a
// monad, applies _function to the inner value via monad_map.
//   Usage:  m | map_with([](int x) { return x * 2; })
template<typename _Function>
D_NODISCARD D_CONSTEXPR internal::map_combinator<typename std::decay<_Function>::type>
map_with(
    _Function&& _function
)
{
    return internal::map_combinator<
        typename std::decay<_Function>::type>(
            std::forward<_Function>(_function));
}


// then_with
//   function: builds a combinator that sequences the LHS monad
// with _other (discarding the LHS value).
//   Usage:  m1 | then_with(m2)
template<typename _Monad>
D_NODISCARD D_CONSTEXPR internal::then_combinator<typename std::decay<_Monad>::type>
then_with(
    _Monad&& _other
)
{
    return internal::then_combinator<
        typename std::decay<_Monad>::type>(
            std::forward<_Monad>(_other));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// operator| (monad | combinator)
//   pipes a monadic value into a combinator. SFINAE-constrained
// to fire only when the LHS is a monad and the RHS has an apply
// method accepting a monad of that kind. This guards against
// accidentally firing on unrelated types that may already
// overload operator|.
template<typename _Monad,
         typename _Combinator,
         typename std::enable_if<is_monad<_Monad>::value,
                                 int>::type = 0,
         typename = decltype(std::declval<const _Combinator&>().apply(
                             std::declval<_Monad>()))>
D_CONSTEXPR auto
operator|(
    _Monad&&      _monad,
    _Combinator&& _combinator
)
-> decltype(_combinator.apply(std::forward<_Monad>(_monad)))
{
    return _combinator.apply(std::forward<_Monad>(_monad));
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_MONAD_