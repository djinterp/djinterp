/******************************************************************************
* djinterp [functional]                                        accumulator.hpp
*
* First-class accumulators with combinable folds (C++11+).
*   An accumulator is a (state, step, finalize) triple that consumes a
* stream of values and yields a single output. This module makes
* accumulators first-class values with their own combinators, most
* importantly `combine(a, b, c, ...)`, which runs several accumulators
* in lock-step over a single pass of the data and returns a tuple of
* their results ("applicative folds").
*
*   REFACTORED 2026-05-27: the accumulator is now parameterized on the
* Step and Final functor types instead of erasing them through
* std::function. This removes the std::function indirection (heap
* allocation for non-small targets, indirect call) and makes every
* accumulator built from constexpr-capable functors usable in a
* constant expression from C++14 on (the step/finalize bodies are
* multi-statement, so C++11's single-return constexpr rule does not
* admit them; pre-built accumulators built from value-initialised
* state are still constructed constexpr in C++11).
*
*   A type-erased escape hatch remains available as `boxed_accumulator`
* for callers who need a single concrete type (heterogeneous storage,
* returning an accumulator across an ABI boundary, runtime selection).
*
* USAGE:
*   auto stats = combine(sum<double>(),
*                        mean<double>(),
*                        min<double>(),
*                        max<double>()).run(values);
*   auto total_age = contramap(sum<int>(),
*                              [](const Person& p){ return p.age; })
*                    .run(people);
*   auto m = filtered(max<int>(),
*                     [](int x){ return x > 0; }).run(values);
*
* path:      /inc/djinterp/core/functional/accumulator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ACCUMULATOR PRIMITIVE
      1.  accumulator<_State, _Input, _Output, _Step, _Final>
      2.  make_accumulator
I.b   ACCUMULATOR STRUCTURAL TRAITS & CONCEPTS
      1.  member / typedef detection (has_*)
      2.  composite traits (has_accumulator_typedefs, is_accumulator,
          is_boxed_accumulator)
      3.  convenience aliases (*_v, accumulator_state_t, ...)
      4.  concepts (accumulator_like, ...)               (C++20)
II.   PRE-BUILT ACCUMULATORS
      1.  sum / product / count / count_if
      2.  min / max / min_by / max_by
      3.  mean / variance / stddev
      4.  first / last / nth
      5.  joining / to_vector / to_map_by / group_by / histogram / top_k
      6.  all_match / any_match / none_match
III.  ACCUMULATOR COMBINATORS
      1.  contramap / map_output / filtered / take
      2.  combine                                 (variadic parallel folds)
IV.   TYPE ERASURE
      1.  boxed_accumulator<_Input, _Output>
      2.  box_accumulator
*/

#ifndef DJINTERP_FUNCTIONAL_ACCUMULATOR_
#define DJINTERP_FUNCTIONAL_ACCUMULATOR_ 1

// std
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP


//   DUAL DOMAIN.  An accumulator is a (state, step, finalize) fold over a value
// stream.  Because it is parameterized on the step and finalize functor types
// rather than erasing them through std::function (the 2026-05-27 refactor noted
// above), an accumulator built from constexpr-capable functors runs in a
// constant expression as readily as at run time - one fold description, two
// execution domains.  step / run / finalize are D_CONSTEXPR14, so the
// compile-time fold is available from C++14 over a constexpr-iterable input
// (a raw array or iterator range, e.g. run(data, count)); the container
// overload additionally needs C++17, where a std::array's iterators become
// constexpr.
//   Scope: like a transducer, an accumulator folds a HOMOGENEOUS value stream
// (one state type) in either domain; the heterogeneous, type-or-value
// compile-time fold is value_list / reduce_ct (see reduce.hpp).  combine(...)
// drives several accumulators in lock-step over one pass and is constexpr on
// the same terms.

///////////////////////////////////////////////////////////////////////////////
///             I.    ACCUMULATOR PRIMITIVE                                 ///
///////////////////////////////////////////////////////////////////////////////

// accumulator
//   class: a (state, step, finalize) triple absorbing _Input values
// into _State and producing a single _Output. Parameterized on the
// _Step and _Final functor types so no type erasure is required.
//
//   _Step   has signature   void(_State&, const _Input&)
//   _Final  has signature   _Output(const _State&)
//
//   The class is intentionally not pure: step() mutates m_state in
// place. Copies share initial state but progress independently.
template<typename _State,
         typename _Input,
         typename _Output,
         typename _Step,
         typename _Final>
class accumulator
{
public:
    typedef _State  state_type;
    typedef _Input  input_type;
    typedef _Output output_type;
    typedef _Step   step_type;
    typedef _Final  final_type;

    template<typename _StateFwd,
             typename _StepFwd,
             typename _FinalFwd>
    D_CONSTEXPR
    accumulator(
        _StateFwd&& _initial_state,
        _StepFwd&&  _step,
        _FinalFwd&& _finalize
    )
        : m_state(std::forward<_StateFwd>(_initial_state)),
          m_step(std::forward<_StepFwd>(_step)),
          m_finalize(std::forward<_FinalFwd>(_finalize))
    {}

    // step
    //   method: feed one value to the accumulator. Returns *this so
    // callers may chain.
    D_CONSTEXPR14
    accumulator&
    step(
        const _Input& _value
    )
    {
        m_step(m_state, _value);

        return *this;
    }

    // finalize
    //   method: produce the output from the current state. Does not
    // modify the state; may be called multiple times.
    D_NODISCARD D_CONSTEXPR _Output
    finalize() const
    {
        return m_finalize(m_state);
    }

    // run (container)
    //   method: drive the accumulator over _container, then finalize.
    template<typename _Container>
    D_NODISCARD D_CONSTEXPR14 _Output
    run(
        const _Container& _container
    )
    {
        for (const auto& element : _container)
        {
            m_step(m_state, element);
        }

        return m_finalize(m_state);
    }

    // run (iterator range)
    template<typename _InputIt>
    D_NODISCARD D_CONSTEXPR14 _Output
    run(
        _InputIt _first,
        _InputIt _last
    )
    {
        for (_InputIt it = _first; it != _last; ++it)
        {
            m_step(m_state, *it);
        }

        return m_finalize(m_state);
    }

    // run (raw array)
    D_NODISCARD D_CONSTEXPR14 _Output
    run(
        const _Input* _data,
        std::size_t   _count
    )
    {
        for (std::size_t i = 0; i < _count; ++i)
        {
            m_step(m_state, _data[i]);
        }

        return m_finalize(m_state);
    }

    // state
    //   method: const access to the live state. Used by tests and by
    // combinators that inspect intermediate state without finalizing.
    D_NODISCARD D_CONSTEXPR14 const _State& state() const { return m_state; }

    // step_fn / finalize_fn
    //   methods: const access to the stored functors. Used by the
    // combinators (contramap / map_output / filtered / take) to
    // compose behavior without rebuilding per element.
    D_NODISCARD D_CONSTEXPR14 const _Step& step_fn() const { return m_step; }

    D_NODISCARD D_CONSTEXPR14 const _Final& finalize_fn() const { return m_finalize; }

private:
    _State m_state;
    _Step  m_step;
    _Final m_finalize;
};


// make_accumulator
//   function: factory building an accumulator from an initial state,
// a step functor, and a finalize functor. _Input and _Output are
// explicit (step/finalize are typically generic and expose no fixed
// signature); _State, _Step, _Final are deduced.
template<typename _Input,
         typename _Output,
         typename _State,
         typename _Step,
         typename _Final>
D_NODISCARD D_CONSTEXPR accumulator<typename std::decay<_State>::type,
                                    _Input,
                                    _Output,
                                    typename std::decay<_Step>::type,
                                    typename std::decay<_Final>::type>
make_accumulator(
    _State&& _initial_state,
    _Step&&  _step,
    _Final&& _finalize
)
{
    return accumulator<typename std::decay<_State>::type,
                       _Input,
                       _Output,
                       typename std::decay<_Step>::type,
                       typename std::decay<_Final>::type>(
        std::forward<_State>(_initial_state),
        std::forward<_Step>(_step),
        std::forward<_Final>(_finalize));
}


///////////////////////////////////////////////////////////////////////////////
///             I.b   ACCUMULATOR STRUCTURAL TRAITS & CONCEPTS              ///
///////////////////////////////////////////////////////////////////////////////
// SFINAE structural traits describing the accumulator contract: the five
// nested typedefs (state_type, input_type, output_type, step_type,
// final_type) and the runtime interface (step / finalize / run / state /
// step_fn / finalize_fn). The composite trait is_accumulator gates template
// overloads and static assertions on the full unboxed contract;
// is_boxed_accumulator recognizes the type-erased escape hatch (which
// exposes only input_type / output_type plus step / finalize / run).
//
//   The detection idiom is rolled locally, in internal, so this header
// stays self-contained. _v shorthands are gated on variable-template
// support; concepts on C++20.

NS_INTERNAL

    // ---- detection idiom ----
    // make_void / void_t
    //   trait: foundational SFINAE mapping of any type pack to void.
    template<typename...>
    struct make_void
    {
        using type = void;
    };

    template<typename... _Ts>
    using void_t = typename make_void<_Ts...>::type;

    // nonesuch
    //   type: placeholder representing "no such type" for the detector.
    struct nonesuch
    {
        nonesuch()                      = delete;
        ~nonesuch()                     = delete;
        nonesuch(const nonesuch&)       = delete;
        void operator=(const nonesuch&) = delete;
    };

    // detector
    //   trait: primary template for SFINAE-based detection (failure case).
    template<typename                       _Default,
             typename                       _AlwaysVoid,
             template<typename...> class    _Op,
             typename...                     _Args>
    struct detector
    {
        using value_t = std::false_type;
        using type    = _Default;
    };

    // detector (success case)
    //   trait: partial specialization when _Op<_Args...> is well-formed.
    template<typename                    _Default,
             template<typename...> class _Op,
             typename...                 _Args>
    struct detector<_Default, void_t<_Op<_Args...> >, _Op, _Args...>
    {
        using value_t = std::true_type;
        using type    = _Op<_Args...>;
    };

    // is_detected
    //   trait: std::true_type when _Op<_Args...> is well-formed.
    template<template<typename...> class _Op,
             typename...                 _Args>
    using is_detected =
        typename detector<nonesuch, void, _Op, _Args...>::value_t;

    // detected_or_t
    //   type: _Op<_Args...> when well-formed, otherwise _Default.
    template<typename                    _Default,
             template<typename...> class _Op,
             typename...                 _Args>
    using detected_or_t =
        typename detector<_Default, void, _Op, _Args...>::type;

    // ---- nested-typedef detection expressions ----

    // acc_state_type_expr
    //   trait: expression alias for nested state_type detection.
    template<typename _Type>
    using acc_state_type_expr = typename _Type::state_type;

    // acc_input_type_expr
    //   trait: expression alias for nested input_type detection.
    template<typename _Type>
    using acc_input_type_expr = typename _Type::input_type;

    // acc_output_type_expr
    //   trait: expression alias for nested output_type detection.
    template<typename _Type>
    using acc_output_type_expr = typename _Type::output_type;

    // acc_step_type_expr
    //   trait: expression alias for nested step_type detection.
    template<typename _Type>
    using acc_step_type_expr = typename _Type::step_type;

    // acc_final_type_expr
    //   trait: expression alias for nested final_type detection.
    template<typename _Type>
    using acc_final_type_expr = typename _Type::final_type;

    // ---- interface (method) detection expressions ----

    // acc_step_method_expr
    //   trait: expression alias detecting .step(const input_type&).
    template<typename _Type>
    using acc_step_method_expr = decltype(
        std::declval<_Type&>().step(
            std::declval<const typename _Type::input_type&>()));

    // acc_finalize_method_expr
    //   trait: expression alias detecting .finalize() const.
    template<typename _Type>
    using acc_finalize_method_expr = decltype(
        std::declval<const _Type&>().finalize());

    // acc_run_method_expr
    //   trait: expression alias detecting .run(container) over a
    // vector of input_type.
    template<typename _Type>
    using acc_run_method_expr = decltype(
        std::declval<_Type&>().run(
            std::declval<
                const std::vector<typename _Type::input_type>&>()));

    // acc_state_method_expr
    //   trait: expression alias detecting .state() const.
    template<typename _Type>
    using acc_state_method_expr = decltype(
        std::declval<const _Type&>().state());

    // acc_step_fn_method_expr
    //   trait: expression alias detecting .step_fn() const.
    template<typename _Type>
    using acc_step_fn_method_expr = decltype(
        std::declval<const _Type&>().step_fn());

    // acc_finalize_fn_method_expr
    //   trait: expression alias detecting .finalize_fn() const.
    template<typename _Type>
    using acc_finalize_fn_method_expr = decltype(
        std::declval<const _Type&>().finalize_fn());

NS_END  // internal


// has_state_type
//   trait: detects whether _Type::state_type exists.
template<typename _Type>
struct has_state_type
{
    static constexpr bool value =
        internal::is_detected<internal::acc_state_type_expr, _Type>::value;
};

// has_input_type
//   trait: detects whether _Type::input_type exists.
template<typename _Type>
struct has_input_type
{
    static constexpr bool value =
        internal::is_detected<internal::acc_input_type_expr, _Type>::value;
};

// has_output_type
//   trait: detects whether _Type::output_type exists.
template<typename _Type>
struct has_output_type
{
    static constexpr bool value =
        internal::is_detected<internal::acc_output_type_expr, _Type>::value;
};

// has_step_type
//   trait: detects whether _Type::step_type exists.
template<typename _Type>
struct has_step_type
{
    static constexpr bool value =
        internal::is_detected<internal::acc_step_type_expr, _Type>::value;
};

// has_final_type
//   trait: detects whether _Type::final_type exists.
template<typename _Type>
struct has_final_type
{
    static constexpr bool value =
        internal::is_detected<internal::acc_final_type_expr, _Type>::value;
};

// has_step_method
//   trait: detects whether _Type has a step(const input_type&) member.
template<typename _Type>
struct has_step_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_step_method_expr, _Type>::value;
};

// has_finalize_method
//   trait: detects whether _Type has a finalize() const member.
template<typename _Type>
struct has_finalize_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_finalize_method_expr,
                              _Type>::value;
};

// has_run_method
//   trait: detects whether _Type has a run(container) member accepting a
// vector of its input_type.
template<typename _Type>
struct has_run_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_run_method_expr, _Type>::value;
};

// has_state_method
//   trait: detects whether _Type has a state() const member.
template<typename _Type>
struct has_state_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_state_method_expr, _Type>::value;
};

// has_step_fn_method
//   trait: detects whether _Type has a step_fn() const member.
template<typename _Type>
struct has_step_fn_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_step_fn_method_expr,
                              _Type>::value;
};

// has_finalize_fn_method
//   trait: detects whether _Type has a finalize_fn() const member.
template<typename _Type>
struct has_finalize_fn_method
{
    static constexpr bool value =
        internal::is_detected<internal::acc_finalize_fn_method_expr,
                              _Type>::value;
};


// has_accumulator_typedefs
//   trait: true when _Type exposes all five accumulator nested typedefs.
template<typename _Type>
struct has_accumulator_typedefs
{
    static constexpr bool value =
        ( has_state_type<_Type>::value  &&
          has_input_type<_Type>::value  &&
          has_output_type<_Type>::value &&
          has_step_type<_Type>::value   &&
          has_final_type<_Type>::value );
};

// has_accumulator_interface
//   trait: true when _Type exposes the minimal consume/produce interface
// (step + finalize).
template<typename _Type>
struct has_accumulator_interface
{
    static constexpr bool value =
        ( has_step_method<_Type>::value &&
          has_finalize_method<_Type>::value );
};

// is_accumulator
//   trait: true when _Type satisfies the complete unboxed accumulator
// contract: all five typedefs plus the full runtime interface (step,
// finalize, state, step_fn, finalize_fn). cv-qualifiers and references on
// _Type are stripped before inspection.
template<typename _Type>
struct is_accumulator
{
private:
    using clean_type = typename std::remove_cv<
                           typename std::remove_reference<_Type>::type>::type;

public:
    static constexpr bool value =
        ( has_accumulator_typedefs<clean_type>::value  &&
          has_accumulator_interface<clean_type>::value &&
          has_state_method<clean_type>::value          &&
          has_step_fn_method<clean_type>::value        &&
          has_finalize_fn_method<clean_type>::value );
};

// is_boxed_accumulator
//   trait: true when _Type is a type-erased accumulator: it exposes
// input_type / output_type and the step / finalize / run interface, but
// not the unboxed-only state_type (the marker that distinguishes the
// erased form). cv/ref are stripped before inspection.
template<typename _Type>
struct is_boxed_accumulator
{
private:
    using clean_type = typename std::remove_cv<
                           typename std::remove_reference<_Type>::type>::type;

public:
    static constexpr bool value =
        ( has_input_type<clean_type>::value      &&
          has_output_type<clean_type>::value     &&
          has_step_method<clean_type>::value     &&
          has_finalize_method<clean_type>::value &&
          has_run_method<clean_type>::value      &&
          !has_state_type<clean_type>::value );
};


// ---- convenience aliases ----
// Variable templates are a C++14 feature; gate the *_v shorthands so the
// header stays clean under -std=c++11 -pedantic. Pre-C++14 callers use the
// ::value form.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_accumulator_v
//   constant: shorthand for is_accumulator<_Type>::value.
template<typename _Type>
static constexpr bool is_accumulator_v = is_accumulator<_Type>::value;

// is_boxed_accumulator_v
//   constant: shorthand for is_boxed_accumulator<_Type>::value.
template<typename _Type>
static constexpr bool is_boxed_accumulator_v =
    is_boxed_accumulator<_Type>::value;

// has_accumulator_typedefs_v
//   constant: shorthand for has_accumulator_typedefs<_Type>::value.
template<typename _Type>
static constexpr bool has_accumulator_typedefs_v =
    has_accumulator_typedefs<_Type>::value;

// has_accumulator_interface_v
//   constant: shorthand for has_accumulator_interface<_Type>::value.
template<typename _Type>
static constexpr bool has_accumulator_interface_v =
    has_accumulator_interface<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// accumulator_state_t
//   type: extracts state_type from an accumulator, or nonesuch if absent.
template<typename _Type>
using accumulator_state_t =
    internal::detected_or_t<internal::nonesuch,
                            internal::acc_state_type_expr, _Type>;

// accumulator_input_t
//   type: extracts input_type from an accumulator, or nonesuch if absent.
template<typename _Type>
using accumulator_input_t =
    internal::detected_or_t<internal::nonesuch,
                            internal::acc_input_type_expr, _Type>;

// accumulator_output_t
//   type: extracts output_type from an accumulator, or nonesuch if absent.
template<typename _Type>
using accumulator_output_t =
    internal::detected_or_t<internal::nonesuch,
                            internal::acc_output_type_expr, _Type>;


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// accumulator_typedefs
//   concept: satisfied when _Type exposes all five accumulator typedefs.
template<typename _Type>
concept accumulator_typedefs = requires
{
    typename _Type::state_type;
    typename _Type::input_type;
    typename _Type::output_type;
    typename _Type::step_type;
    typename _Type::final_type;
};

// accumulator_steppable
//   concept: satisfied when _Type can consume one input via .step().
template<typename _Type>
concept accumulator_steppable = requires(
    _Type&                              _acc,
    const typename _Type::input_type&   _value)
{
    _acc.step(_value);
};

// accumulator_finalizable
//   concept: satisfied when _Type can produce an output via .finalize().
template<typename _Type>
concept accumulator_finalizable = requires(const _Type& _acc)
{
    _acc.finalize();
};

// accumulator_like
//   concept: satisfied when _Type models the full unboxed accumulator
// contract (typedefs + steppable + finalizable).
template<typename _Type>
concept accumulator_like =
    ( accumulator_typedefs<_Type>    &&
      accumulator_steppable<_Type>   &&
      accumulator_finalizable<_Type> );

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///             II.   PRE-BUILT ACCUMULATOR FUNCTORS                        ///
///////////////////////////////////////////////////////////////////////////////
// Each pre-built accumulator is backed by named functor types rather
// than lambdas, so that (a) the accumulator type is nameable, and
// (b) the functors are constexpr-capable without relying on C++17
// constexpr lambdas.

NS_INTERNAL
    template<typename _Type>
    struct identity_final
    {
        D_CONSTEXPR
        const _Type& operator()(const _Type& _s) const { return _s; }
    };

    template<typename _Type>
    struct sum_step
    {
        D_CONSTEXPR14
        void operator()(_Type& _s, const _Type& _v) const { _s = _s + _v; }
    };

    template<typename _Type>
    struct product_step
    {
        D_CONSTEXPR14
        void operator()(_Type& _s, const _Type& _v) const { _s = _s * _v; }
    };

    template<typename _Type>
    struct count_step
    {
        D_CONSTEXPR14
        void operator()(std::size_t& _s, const _Type&) const { ++_s; }
    };

    struct size_t_final
    {
        D_CONSTEXPR
        const std::size_t& operator()(const std::size_t& _s) const
        {
            return _s;
        }
    };

    template<typename _Type,
             typename _Predicate>
    class count_if_step
    {
    public:
        D_CONSTEXPR
        explicit count_if_step(const _Predicate& _p) : m_pred(_p) {}

        D_CONSTEXPR14
        void operator()(std::size_t& _s, const _Type& _v) const
        {
            if (m_pred(_v)) { ++_s; }
        }

    private:
        _Predicate m_pred;
    };

    template<typename _Type>
    struct min_step
    {
        D_CONSTEXPR14
        void operator()(std::pair<_Type, bool>& _s, const _Type& _v) const
        {
            if (!_s.second || (_v < _s.first))
            {
                _s.first  = _v;
                _s.second = true;
            }
        }
    };

    template<typename _Type>
    struct max_step
    {
        D_CONSTEXPR14
        void operator()(std::pair<_Type, bool>& _s, const _Type& _v) const
        {
            if (!_s.second || (_s.first < _v))
            {
                _s.first  = _v;
                _s.second = true;
            }
        }
    };

    template<typename _Type>
    struct pair_first_final
    {
        D_CONSTEXPR
        _Type operator()(const std::pair<_Type, bool>& _s) const
        {
            return _s.first;
        }
    };

    template<typename _Type,
             typename _Key>
    class min_by_step
    {
    public:
        D_CONSTEXPR
        explicit min_by_step(const _Key& _k) : m_key(_k) {}

        D_CONSTEXPR14
        void operator()(std::pair<_Type, bool>& _s, const _Type& _v) const
        {
            if (!_s.second || (m_key(_v) < m_key(_s.first)))
            {
                _s.first  = _v;
                _s.second = true;
            }
        }

    private:
        _Key m_key;
    };

    template<typename _Type,
             typename _Key>
    class max_by_step
    {
    public:
        D_CONSTEXPR
        explicit max_by_step(const _Key& _k) : m_key(_k) {}

        D_CONSTEXPR14
        void operator()(std::pair<_Type, bool>& _s, const _Type& _v) const
        {
            if (!_s.second || (m_key(_s.first) < m_key(_v)))
            {
                _s.first  = _v;
                _s.second = true;
            }
        }

    private:
        _Key m_key;
    };

    template<typename _Type>
    struct mean_step
    {
        D_CONSTEXPR14
        void operator()(std::pair<double, std::size_t>& _s,
                        const _Type& _v) const
        {
            _s.first  += static_cast<double>(_v);
            _s.second += 1;
        }
    };

    struct mean_final
    {
        D_CONSTEXPR14
        double operator()(const std::pair<double, std::size_t>& _s) const
        {
            if (_s.second == 0) { return 0.0; }

            return _s.first / static_cast<double>(_s.second);
        }
    };

    // Welford state: (count, mean, M2)
    template<typename _Type>
    struct welford_step
    {
        D_CONSTEXPR14
        void operator()(std::tuple<std::size_t, double, double>& _s,
                        const _Type& _v) const
        {
            std::size_t& n    = std::get<0>(_s);
            double&      mean = std::get<1>(_s);
            double&      m2   = std::get<2>(_s);

            ++n;
            double delta  = static_cast<double>(_v) - mean;
            mean         += delta / static_cast<double>(n);
            double delta2 = static_cast<double>(_v) - mean;
            m2           += delta * delta2;
        }
    };

    struct variance_final
    {
        D_CONSTEXPR14
        double operator()(
            const std::tuple<std::size_t, double, double>& _s) const
        {
            std::size_t n = std::get<0>(_s);

            if (n < 2) { return 0.0; }

            return std::get<2>(_s) / static_cast<double>(n);
        }
    };

    struct stddev_final
    {
        D_CONSTEXPR14 double
        operator()(
            const std::tuple<std::size_t, double, double>& _s) const
        {
            std::size_t n = std::get<0>(_s);

            if (n < 2) { return 0.0; }

            double var = std::get<2>(_s) / static_cast<double>(n);

            if (var <= 0.0) { return 0.0; }

            // Newton's method sqrt; constexpr-friendly, no <cmath>.
            double x = var;
            for (int i = 0; i < 16; ++i)
            {
                x = 0.5 * (x + (var / x));
            }

            return x;
        }
    };

    template<typename _Type>
    struct first_step
    {
        D_CONSTEXPR14 void
        operator()(
            std::pair<_Type, bool>& _s, const _Type& _v
        ) const
        {
            if (!_s.second)
            {
                _s.first  = _v;
                _s.second = true;
            }
        }
    };

    template<typename _Type>
    struct last_step
    {
        D_CONSTEXPR14 void
        operator()(
            _Type&       _s, 
            const _Type& _v
        ) const
        {
            _s = _v; 
        }
    };

    // nth state: (value, seen, want)
    template<typename _Type>
    struct nth_state
    {
        _Type        value;
        std::size_t  seen;
        std::size_t  want;

        D_CONSTEXPR
        nth_state() : value(), seen(0), want(0) {}

        D_CONSTEXPR
        explicit nth_state(std::size_t _want)
            : value(), seen(0), want(_want) {}
    };

    template<typename _Type>
    struct nth_step
    {
        D_CONSTEXPR14
        void operator()(nth_state<_Type>& _s, const _Type& _v) const
        {
            if (_s.seen == _s.want) { _s.value = _v; }
            ++_s.seen;
        }
    };

    template<typename _Type>
    struct nth_final
    {
        D_CONSTEXPR
        _Type operator()(const nth_state<_Type>& _s) const
        {
            return _s.value;
        }
    };

    // joining: (string, seen-any). Uses ostringstream -> runtime only.
    template<typename _Type>
    class joining_step
    {
    public:
        explicit joining_step(std::string _sep) : m_sep(std::move(_sep)) {}

        void operator()(std::pair<std::string, bool>& _s,
                        const _Type& _v) const
        {
            std::ostringstream oss;

            if (_s.second) { _s.first += m_sep; }

            oss << _v;
            _s.first += oss.str();
            _s.second = true;
        }

    private:
        std::string m_sep;
    };

    struct joining_final
    {
        const std::string& operator()(
            const std::pair<std::string, bool>& _s) const
        {
            return _s.first;
        }
    };

    template<typename _Type>
    struct to_vector_step
    {
        // constexpr from C++20 (constexpr std::vector); runtime below.
        D_CONSTEXPR14
        void operator()(std::vector<_Type>& _s, const _Type& _v) const
        {
            _s.push_back(_v);
        }
    };

    template<typename _Type>
    struct histogram_step
    {
        void operator()(std::map<_Type, std::size_t>& _s,
                        const _Type& _v) const
        {
            ++_s[_v];
        }
    };

    template<typename _Type,
             typename _Key>
    class to_map_by_step
    {
    public:
        explicit to_map_by_step(const _Key& _k) : m_key(_k) {}

        template<typename _Map>
        void operator()(_Map& _s, const _Type& _v) const
        {
            _s[m_key(_v)] = _v;
        }

    private:
        _Key m_key;
    };

    template<typename _Type,
             typename _Key>
    class group_by_step
    {
    public:
        explicit group_by_step(const _Key& _k) : m_key(_k) {}

        template<typename _Map>
        void operator()(_Map& _s, const _Type& _v) const
        {
            _s[m_key(_v)].push_back(_v);
        }

    private:
        _Key m_key;
    };

    template<typename _Type>
    class top_k_step
    {
    public:
        D_CONSTEXPR
        explicit top_k_step(std::size_t _k) : m_k(_k) {}

        D_CONSTEXPR14
        void operator()(std::vector<_Type>& _s, const _Type& _v) const
        {
            if ((_s.size() >= m_k) && !(_s.back() < _v)) { return; }

            typename std::vector<_Type>::iterator it = _s.begin();

            while ((it != _s.end()) && !(*it < _v)) { ++it; }

            _s.insert(it, _v);

            if (_s.size() > m_k) { _s.resize(m_k); }
        }

    private:
        std::size_t m_k;
    };

    template<typename _Type,
             typename _Predicate>
    class all_match_step
    {
    public:
        D_CONSTEXPR
        explicit all_match_step(const _Predicate& _p) : m_pred(_p) {}

        D_CONSTEXPR14
        void operator()(bool& _s, const _Type& _v) const
        {
            _s = _s && m_pred(_v);
        }

    private:
        _Predicate m_pred;
    };

    template<typename _Type,
             typename _Predicate>
    class any_match_step
    {
    public:
        D_CONSTEXPR
        explicit any_match_step(const _Predicate& _p) : m_pred(_p) {}

        D_CONSTEXPR14
        void operator()(bool& _s, const _Type& _v) const
        {
            _s = _s || m_pred(_v);
        }

    private:
        _Predicate m_pred;
    };

    template<typename _Type,
             typename _Predicate>
    class none_match_step
    {
    public:
        D_CONSTEXPR
        explicit none_match_step(const _Predicate& _p) : m_pred(_p) {}

        D_CONSTEXPR14
        void operator()(bool& _s, const _Type& _v) const
        {
            if (m_pred(_v)) { _s = false; }
        }

    private:
        _Predicate m_pred;
    };

    struct bool_final
    {
        D_CONSTEXPR
        bool operator()(const bool& _s) const { return _s; }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.b  PRE-BUILT ACCUMULATOR FACTORIES                       ///
///////////////////////////////////////////////////////////////////////////////

// sum
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<_Type, _Type, _Type,
            internal::sum_step<_Type>,
            internal::identity_final<_Type> >
sum()
{
    return accumulator<_Type, _Type, _Type,
                       internal::sum_step<_Type>,
                       internal::identity_final<_Type> >(
        _Type(),
        internal::sum_step<_Type>(),
        internal::identity_final<_Type>());
}

// product
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<_Type, _Type, _Type,
            internal::product_step<_Type>,
            internal::identity_final<_Type> >
product()
{
    return accumulator<_Type, _Type, _Type,
                       internal::product_step<_Type>,
                       internal::identity_final<_Type> >(
        _Type(1),
        internal::product_step<_Type>(),
        internal::identity_final<_Type>());
}

// count
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::size_t, _Type, std::size_t,
            internal::count_step<_Type>,
            internal::size_t_final>
count()
{
    return accumulator<std::size_t, _Type, std::size_t,
                       internal::count_step<_Type>,
                       internal::size_t_final>(
        std::size_t(0),
        internal::count_step<_Type>(),
        internal::size_t_final());
}

// count_if
template<typename _Type,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR accumulator<std::size_t, _Type, std::size_t,
            internal::count_if_step<_Type,
                typename std::decay<_Predicate>::type>,
            internal::size_t_final>
count_if(_Predicate _predicate)
{
    typedef typename std::decay<_Predicate>::type pred_t;

    return accumulator<std::size_t, _Type, std::size_t,
                       internal::count_if_step<_Type, pred_t>,
                       internal::size_t_final>(
        std::size_t(0),
        internal::count_if_step<_Type, pred_t>(_predicate),
        internal::size_t_final());
}

// min
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<_Type, bool>, _Type, _Type,
            internal::min_step<_Type>,
            internal::pair_first_final<_Type> >
min()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type,
                       internal::min_step<_Type>,
                       internal::pair_first_final<_Type> >(
        std::pair<_Type, bool>(_Type(), false),
        internal::min_step<_Type>(),
        internal::pair_first_final<_Type>());
}

// max
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<_Type, bool>, _Type, _Type,
            internal::max_step<_Type>,
            internal::pair_first_final<_Type> >
max()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type,
                       internal::max_step<_Type>,
                       internal::pair_first_final<_Type> >(
        std::pair<_Type, bool>(_Type(), false),
        internal::max_step<_Type>(),
        internal::pair_first_final<_Type>());
}

// min_by
template<typename _Type,
         typename _Key>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<_Type, bool>, _Type, _Type,
            internal::min_by_step<_Type, typename std::decay<_Key>::type>,
            internal::pair_first_final<_Type> >
min_by(_Key _key_fn)
{
    typedef typename std::decay<_Key>::type key_t;

    return accumulator<std::pair<_Type, bool>, _Type, _Type,
                       internal::min_by_step<_Type, key_t>,
                       internal::pair_first_final<_Type> >(
        std::pair<_Type, bool>(_Type(), false),
        internal::min_by_step<_Type, key_t>(_key_fn),
        internal::pair_first_final<_Type>());
}

// max_by
template<typename _Type,
         typename _Key>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<_Type, bool>, _Type, _Type,
            internal::max_by_step<_Type, typename std::decay<_Key>::type>,
            internal::pair_first_final<_Type> >
max_by(_Key _key_fn)
{
    typedef typename std::decay<_Key>::type key_t;

    return accumulator<std::pair<_Type, bool>, _Type, _Type,
                       internal::max_by_step<_Type, key_t>,
                       internal::pair_first_final<_Type> >(
        std::pair<_Type, bool>(_Type(), false),
        internal::max_by_step<_Type, key_t>(_key_fn),
        internal::pair_first_final<_Type>());
}

// mean
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<double, std::size_t>, _Type, double,
            internal::mean_step<_Type>,
            internal::mean_final>
mean()
{
    return accumulator<std::pair<double, std::size_t>, _Type, double,
                       internal::mean_step<_Type>,
                       internal::mean_final>(
        std::pair<double, std::size_t>(0.0, std::size_t(0)),
        internal::mean_step<_Type>(),
        internal::mean_final());
}

// variance (population, Welford)
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::tuple<std::size_t, double, double>, _Type, double,
            internal::welford_step<_Type>,
            internal::variance_final>
variance()
{
    return accumulator<std::tuple<std::size_t, double, double>, _Type, double,
                       internal::welford_step<_Type>,
                       internal::variance_final>(
        std::make_tuple(std::size_t(0), 0.0, 0.0),
        internal::welford_step<_Type>(),
        internal::variance_final());
}

// stddev (population)
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::tuple<std::size_t, double, double>, _Type, double,
            internal::welford_step<_Type>,
            internal::stddev_final>
stddev()
{
    return accumulator<std::tuple<std::size_t, double, double>, _Type, double,
                       internal::welford_step<_Type>,
                       internal::stddev_final>(
        std::make_tuple(std::size_t(0), 0.0, 0.0),
        internal::welford_step<_Type>(),
        internal::stddev_final());
}

// first
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<_Type, bool>, _Type, _Type,
            internal::first_step<_Type>,
            internal::pair_first_final<_Type> >
first()
{
    return accumulator<std::pair<_Type, bool>, _Type, _Type,
                       internal::first_step<_Type>,
                       internal::pair_first_final<_Type> >(
        std::pair<_Type, bool>(_Type(), false),
        internal::first_step<_Type>(),
        internal::pair_first_final<_Type>());
}

// last
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<_Type, _Type, _Type,
            internal::last_step<_Type>,
            internal::identity_final<_Type> >
last()
{
    return accumulator<_Type, _Type, _Type,
                       internal::last_step<_Type>,
                       internal::identity_final<_Type> >(
        _Type(),
        internal::last_step<_Type>(),
        internal::identity_final<_Type>());
}

// nth
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<internal::nth_state<_Type>, _Type, _Type,
            internal::nth_step<_Type>,
            internal::nth_final<_Type> >
nth(std::size_t _n)
{
    return accumulator<internal::nth_state<_Type>, _Type, _Type,
                       internal::nth_step<_Type>,
                       internal::nth_final<_Type> >(
        internal::nth_state<_Type>(_n),
        internal::nth_step<_Type>(),
        internal::nth_final<_Type>());
}

// joining (runtime only — ostringstream)
template<typename _Type>
D_NODISCARD
accumulator<std::pair<std::string, bool>, _Type, std::string,
            internal::joining_step<_Type>,
            internal::joining_final>
joining(std::string _separator)
{
    return accumulator<std::pair<std::string, bool>, _Type, std::string,
                       internal::joining_step<_Type>,
                       internal::joining_final>(
        std::pair<std::string, bool>(std::string(), false),
        internal::joining_step<_Type>(std::move(_separator)),
        internal::joining_final());
}

// to_vector (constexpr from C++20)
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::vector<_Type>, _Type, std::vector<_Type>,
            internal::to_vector_step<_Type>,
            internal::identity_final<std::vector<_Type> > >
to_vector()
{
    return accumulator<std::vector<_Type>, _Type, std::vector<_Type>,
                       internal::to_vector_step<_Type>,
                       internal::identity_final<std::vector<_Type> > >(
        std::vector<_Type>(),
        internal::to_vector_step<_Type>(),
        internal::identity_final<std::vector<_Type> >());
}

// histogram (runtime — std::map)
template<typename _Type>
D_NODISCARD
accumulator<std::map<_Type, std::size_t>, _Type,
            std::map<_Type, std::size_t>,
            internal::histogram_step<_Type>,
            internal::identity_final<std::map<_Type, std::size_t> > >
histogram()
{
    typedef std::map<_Type, std::size_t> map_t;

    return accumulator<map_t, _Type, map_t,
                       internal::histogram_step<_Type>,
                       internal::identity_final<map_t> >(
        map_t(),
        internal::histogram_step<_Type>(),
        internal::identity_final<map_t>());
}

// to_map_by (runtime — std::map)
template<typename _Type,
         typename _Key>
D_NODISCARD
accumulator<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                _Type>,
            _Type,
            std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                _Type>,
            internal::to_map_by_step<_Type, typename std::decay<_Key>::type>,
            internal::identity_final<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                _Type> > >
to_map_by(_Key _key_fn)
{
    typedef typename std::decay<_Key>::type key_fn_t;
    typedef typename std::decay<decltype(
        std::declval<_Key&>()(std::declval<const _Type&>()))>::type key_t;
    typedef std::map<key_t, _Type> map_t;

    return accumulator<map_t, _Type, map_t,
                       internal::to_map_by_step<_Type, key_fn_t>,
                       internal::identity_final<map_t> >(
        map_t(),
        internal::to_map_by_step<_Type, key_fn_t>(_key_fn),
        internal::identity_final<map_t>());
}

// group_by (runtime — std::map)
template<typename _Type,
         typename _Key>
D_NODISCARD
accumulator<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                std::vector<_Type> >,
            _Type,
            std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                std::vector<_Type> >,
            internal::group_by_step<_Type, typename std::decay<_Key>::type>,
            internal::identity_final<std::map<typename std::decay<decltype(
                std::declval<_Key&>()(std::declval<const _Type&>()))>::type,
                std::vector<_Type> > > >
group_by(_Key _key_fn)
{
    typedef typename std::decay<_Key>::type key_fn_t;
    typedef typename std::decay<decltype(
        std::declval<_Key&>()(std::declval<const _Type&>()))>::type key_t;
    typedef std::map<key_t, std::vector<_Type> > map_t;

    return accumulator<map_t, _Type, map_t,
                       internal::group_by_step<_Type, key_fn_t>,
                       internal::identity_final<map_t> >(
        map_t(),
        internal::group_by_step<_Type, key_fn_t>(_key_fn),
        internal::identity_final<map_t>());
}

// top_k
template<typename _Type>
D_NODISCARD D_CONSTEXPR accumulator<std::vector<_Type>, _Type, std::vector<_Type>,
            internal::top_k_step<_Type>,
            internal::identity_final<std::vector<_Type> > >
top_k(std::size_t _k)
{
    return accumulator<std::vector<_Type>, _Type, std::vector<_Type>,
                       internal::top_k_step<_Type>,
                       internal::identity_final<std::vector<_Type> > >(
        std::vector<_Type>(),
        internal::top_k_step<_Type>(_k),
        internal::identity_final<std::vector<_Type> >());
}

// all_match
template<typename _Type,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR accumulator<bool, _Type, bool,
            internal::all_match_step<_Type,
                typename std::decay<_Predicate>::type>,
            internal::bool_final>
all_match(_Predicate _predicate)
{
    typedef typename std::decay<_Predicate>::type pred_t;

    return accumulator<bool, _Type, bool,
                       internal::all_match_step<_Type, pred_t>,
                       internal::bool_final>(
        true,
        internal::all_match_step<_Type, pred_t>(_predicate),
        internal::bool_final());
}

// any_match
template<typename _Type,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR accumulator<bool, _Type, bool,
            internal::any_match_step<_Type,
                typename std::decay<_Predicate>::type>,
            internal::bool_final>
any_match(_Predicate _predicate)
{
    typedef typename std::decay<_Predicate>::type pred_t;

    return accumulator<bool, _Type, bool,
                       internal::any_match_step<_Type, pred_t>,
                       internal::bool_final>(
        false,
        internal::any_match_step<_Type, pred_t>(_predicate),
        internal::bool_final());
}

// none_match
template<typename _Type,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR accumulator<bool, _Type, bool,
            internal::none_match_step<_Type,
                typename std::decay<_Predicate>::type>,
            internal::bool_final>
none_match(_Predicate _predicate)
{
    typedef typename std::decay<_Predicate>::type pred_t;

    return accumulator<bool, _Type, bool,
                       internal::none_match_step<_Type, pred_t>,
                       internal::bool_final>(
        true,
        internal::none_match_step<_Type, pred_t>(_predicate),
        internal::bool_final());
}


///////////////////////////////////////////////////////////////////////////////
///             III.  ACCUMULATOR COMBINATORS                               ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // contramap_step: pre-applies _Function to each value before
    // forwarding to the inner step.
    template<typename _NewInput,
             typename _InnerStep,
             typename _Function>
    class contramap_step
    {
    public:
        D_CONSTEXPR
        contramap_step(const _InnerStep& _s, const _Function& _f)
            : m_step(_s), m_fn(_f) {}

        template<typename _State>
        D_CONSTEXPR14
        void operator()(_State& _state, const _NewInput& _v) const
        {
            m_step(_state, m_fn(_v));
        }

    private:
        _InnerStep m_step;
        _Function  m_fn;
    };

    // filtered_step: forwards only values satisfying the predicate.
    template<typename _Input,
             typename _InnerStep,
             typename _Predicate>
    class filtered_step
    {
    public:
        D_CONSTEXPR
        filtered_step(const _InnerStep& _s, const _Predicate& _p)
            : m_step(_s), m_pred(_p) {}

        template<typename _State>
        D_CONSTEXPR14
        void operator()(_State& _state, const _Input& _v) const
        {
            if (m_pred(_v)) { m_step(_state, _v); }
        }

    private:
        _InnerStep m_step;
        _Predicate m_pred;
    };

    // map_output_final: post-applies _Function to the inner output.
    template<typename _InnerFinal,
             typename _Function>
    class map_output_final
    {
    public:
        D_CONSTEXPR
        map_output_final(const _InnerFinal& _f, const _Function& _fn)
            : m_final(_f), m_fn(_fn) {}

        template<typename _State>
        D_CONSTEXPR
        auto operator()(const _State& _s) const
            -> decltype(std::declval<const _Function&>()(
                   std::declval<const _InnerFinal&>()(_s)))
        {
            return m_fn(m_final(_s));
        }

    private:
        _InnerFinal m_final;
        _Function   m_fn;
    };

    // take wrapper: state is pair<inner_state, count>.
    template<typename _Input,
             typename _InnerStep>
    class take_step
    {
    public:
        D_CONSTEXPR
        take_step(const _InnerStep& _s, std::size_t _n)
            : m_step(_s), m_n(_n) {}

        template<typename _State>
        D_CONSTEXPR14
        void operator()(_State& _state, const _Input& _v) const
        {
            if (_state.second < m_n)
            {
                m_step(_state.first, _v);
                ++_state.second;
            }
        }

    private:
        _InnerStep  m_step;
        std::size_t m_n;
    };

    template<typename _InnerFinal>
    class take_final
    {
    public:
        D_CONSTEXPR
        explicit take_final(const _InnerFinal& _f) : m_final(_f) {}

        template<typename _Pair>
        D_CONSTEXPR
        auto operator()(const _Pair& _s) const
            -> decltype(std::declval<const _InnerFinal&>()(_s.first))
        {
            return m_final(_s.first);
        }

    private:
        _InnerFinal m_final;
    };

NS_END  // internal


// contramap
//   function: adapts an accumulator to accept _NewInput by pre-
// applying _function : _NewInput -> Input before each step.
template<typename _NewInput,
         typename _Acc,
         typename _Function>
D_NODISCARD D_CONSTEXPR accumulator<typename _Acc::state_type,
            _NewInput,
            typename _Acc::output_type,
            internal::contramap_step<_NewInput,
                typename _Acc::step_type,
                typename std::decay<_Function>::type>,
            typename _Acc::final_type>
contramap(_Acc _inner, _Function _function)
{
    typedef typename std::decay<_Function>::type fn_t;
    typedef internal::contramap_step<_NewInput,
        typename _Acc::step_type, fn_t> step_t;

    return accumulator<typename _Acc::state_type,
                       _NewInput,
                       typename _Acc::output_type,
                       step_t,
                       typename _Acc::final_type>(
        _inner.state(),
        step_t(_inner.step_fn(), _function),
        _inner.finalize_fn());
}


// map_output
//   function: post-applies _function to the inner accumulator's output.
template<typename _Acc,
         typename _Function>
D_NODISCARD D_CONSTEXPR accumulator<typename _Acc::state_type,
            typename _Acc::input_type,
            typename std::decay<decltype(std::declval<_Function&>()(
                std::declval<typename _Acc::output_type>()))>::type,
            typename _Acc::step_type,
            internal::map_output_final<typename _Acc::final_type,
                typename std::decay<_Function>::type> >
map_output(_Acc _inner, _Function _function)
{
    typedef typename std::decay<_Function>::type fn_t;
    typedef typename std::decay<decltype(std::declval<_Function&>()(
        std::declval<typename _Acc::output_type>()))>::type new_out_t;
    typedef internal::map_output_final<
        typename _Acc::final_type, fn_t> final_t;

    return accumulator<typename _Acc::state_type,
                       typename _Acc::input_type,
                       new_out_t,
                       typename _Acc::step_type,
                       final_t>(
        _inner.state(),
        _inner.step_fn(),
        final_t(_inner.finalize_fn(), _function));
}


// filtered
//   function: gates the input of an inner accumulator with a predicate.
template<typename _Acc,
         typename _Predicate>
D_NODISCARD D_CONSTEXPR accumulator<typename _Acc::state_type,
            typename _Acc::input_type,
            typename _Acc::output_type,
            internal::filtered_step<typename _Acc::input_type,
                typename _Acc::step_type,
                typename std::decay<_Predicate>::type>,
            typename _Acc::final_type>
filtered(_Acc _inner, _Predicate _predicate)
{
    typedef typename std::decay<_Predicate>::type pred_t;
    typedef internal::filtered_step<typename _Acc::input_type,
        typename _Acc::step_type, pred_t> step_t;

    return accumulator<typename _Acc::state_type,
                       typename _Acc::input_type,
                       typename _Acc::output_type,
                       step_t,
                       typename _Acc::final_type>(
        _inner.state(),
        step_t(_inner.step_fn(), _predicate),
        _inner.finalize_fn());
}


// take
//   function: caps the number of inputs the inner accumulator sees.
template<typename _Acc>
D_NODISCARD D_CONSTEXPR accumulator<std::pair<typename _Acc::state_type, std::size_t>,
            typename _Acc::input_type,
            typename _Acc::output_type,
            internal::take_step<typename _Acc::input_type,
                typename _Acc::step_type>,
            internal::take_final<typename _Acc::final_type> >
take(_Acc _inner, std::size_t _n)
{
    typedef std::pair<typename _Acc::state_type, std::size_t> wrapped_t;
    typedef internal::take_step<typename _Acc::input_type,
        typename _Acc::step_type> step_t;
    typedef internal::take_final<typename _Acc::final_type> final_t;

    return accumulator<wrapped_t,
                       typename _Acc::input_type,
                       typename _Acc::output_type,
                       step_t,
                       final_t>(
        wrapped_t(_inner.state(), std::size_t(0)),
        step_t(_inner.step_fn(), _n),
        final_t(_inner.finalize_fn()));
}


///////////////////////////////////////////////////////////////////////////////
///             III.b  COMBINE  (variadic parallel folds)                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // combine_helper: holds a tuple of accumulators and drives them
    // in lock-step over one pass.
    template<typename... _Accs>
    class combine_helper
    {
    public:
        using output_tuple = std::tuple<typename _Accs::output_type...>;
        using input_type   = typename std::tuple_element<
            0, std::tuple<typename _Accs::input_type...> >::type;

        template<typename... _AccsFwd>
        D_CONSTEXPR
        explicit combine_helper(_AccsFwd&&... _accs)
            : m_accs(std::forward<_AccsFwd>(_accs)...)
        {}

        D_CONSTEXPR14
        void step(const input_type& _value)
        {
            step_helper(_value,
                        std::integral_constant<std::size_t, 0>());
        }

        D_NODISCARD
        D_CONSTEXPR14
        output_tuple finalize() const
        {
            return finalize_helper(
                std::integral_constant<std::size_t, 0>(),
                std::tuple<>());
        }

        template<typename _Container>
        D_NODISCARD
        D_CONSTEXPR14
        output_tuple run(const _Container& _container)
        {
            for (const auto& element : _container)
            {
                step(element);
            }

            return finalize();
        }

        template<typename _InputIt>
        D_NODISCARD
        D_CONSTEXPR14
        output_tuple run(_InputIt _first, _InputIt _last)
        {
            for (_InputIt it = _first; it != _last; ++it)
            {
                step(*it);
            }

            return finalize();
        }

    private:
        template<std::size_t _I>
        D_CONSTEXPR14
        typename std::enable_if<(_I < sizeof...(_Accs))>::type
        step_helper(const input_type& _value,
                    std::integral_constant<std::size_t, _I>)
        {
            std::get<_I>(m_accs).step(_value);
            step_helper(_value,
                        std::integral_constant<std::size_t, _I + 1>());
        }

        template<std::size_t _I>
        D_CONSTEXPR14
        typename std::enable_if<(_I == sizeof...(_Accs))>::type
        step_helper(const input_type&,
                    std::integral_constant<std::size_t, _I>)
        {}

        template<std::size_t _I, typename... _SoFar>
        D_CONSTEXPR14
        typename std::enable_if<(_I < sizeof...(_Accs)),
                                output_tuple>::type
        finalize_helper(std::integral_constant<std::size_t, _I>,
                        std::tuple<_SoFar...> _so_far) const
        {
            return finalize_helper(
                std::integral_constant<std::size_t, _I + 1>(),
                std::tuple_cat(
                    std::move(_so_far),
                    std::make_tuple(std::get<_I>(m_accs).finalize())));
        }

        template<std::size_t _I, typename... _SoFar>
        D_CONSTEXPR14
        typename std::enable_if<(_I == sizeof...(_Accs)),
                                output_tuple>::type
        finalize_helper(std::integral_constant<std::size_t, _I>,
                        std::tuple<_SoFar...> _so_far) const
        {
            return _so_far;
        }

        std::tuple<_Accs...> m_accs;
    };

NS_END  // internal


// combine
//   function: runs multiple accumulators in lock-step over one pass.
// Returns a combine_helper whose run(container) yields a std::tuple of
// outputs, one per accumulator. All inner accumulators must accept the
// same input type (taken from the first).
template<typename... _Accs>
D_NODISCARD D_CONSTEXPR internal::combine_helper<typename std::decay<_Accs>::type...>
combine(_Accs&&... _accs)
{
    return internal::combine_helper<typename std::decay<_Accs>::type...>(
        std::forward<_Accs>(_accs)...);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   TYPE ERASURE  (escape hatch)                          ///
///////////////////////////////////////////////////////////////////////////////

// boxed_accumulator
//   class: type-erased accumulator of (Input -> Output). Stores
// step/finalize via std::function. Use when a single concrete type is
// required (heterogeneous containers, ABI boundaries, runtime
// selection). Comes with the usual std::function overhead; for
// compile-time-fixed chains, prefer the unboxed factories above.
template<typename _Input,
         typename _Output>
class boxed_accumulator
{
public:
    typedef _Input  input_type;
    typedef _Output output_type;

    // construct from any unboxed accumulator
    template<typename _Acc>
    explicit boxed_accumulator(_Acc _inner)
    {
        // capture inner by value; expose step/finalize through
        // std::function over an opaque shared state.
        auto state = std::make_shared<_Acc>(std::move(_inner));

        m_step = [state](const _Input& _v) { state->step(_v); };
        m_finalize = [state]() -> _Output { return state->finalize(); };
        m_run_vec = [state](const std::vector<_Input>& _c) -> _Output
        {
            return state->run(_c);
        };
    }

    boxed_accumulator& step(const _Input& _value)
    {
        m_step(_value);

        return *this;
    }

    D_NODISCARD
    _Output finalize() const { return m_finalize(); }

    template<typename _Container>
    D_NODISCARD
    _Output run(const _Container& _container)
    {
        std::vector<_Input> vec(std::begin(_container),
                                std::end(_container));

        return m_run_vec(vec);
    }

private:
    std::function<void(const _Input&)>              m_step;
    std::function<_Output()>                        m_finalize;
    std::function<_Output(const std::vector<_Input>&)> m_run_vec;
};


// box_accumulator
//   function: wraps any unboxed accumulator in a boxed_accumulator.
// _Input and _Output are taken from the accumulator's typedefs.
template<typename _Acc>
D_NODISCARD
boxed_accumulator<typename _Acc::input_type,
                  typename _Acc::output_type>
box_accumulator(_Acc _inner)
{
    return boxed_accumulator<typename _Acc::input_type,
                             typename _Acc::output_type>(
        std::move(_inner));
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_ACCUMULATOR_
