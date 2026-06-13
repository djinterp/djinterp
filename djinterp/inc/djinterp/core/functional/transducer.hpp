/******************************************************************************
* djinterp [functional]                                         transducer.hpp
*
* Composable, source-and-sink-agnostic transformations (C++14+).
*   A transducer is a function from one reducer to another: it takes a
* reducer (a step function of shape (Acc, Value) -> Acc) and returns a
* new reducer that performs some transformation before forwarding to the
* original. Because transducers operate at the reducer level, the same
* transducer can be applied to ANY source (container, producer, view,
* stream) and ANY sink (vector, accumulator, consumer) without
* rewriting.
*
*   This module is the bridge between view.hpp (eager-source, eager-sink)
* and the more general producer / accumulator / consumer story: the same
* `map(f) | filter(p) | take(n)` chain works in all three regimes when
* expressed as a transducer.
*
*   The protocol is intentionally simple: a transducer is any callable
* that takes a reducer (any callable of shape (Acc&, const Value&) -> void)
* and returns a new reducer. Composition is ordinary function composition;
* we provide `compose(t1, t2, ...)` plus operator| pipeline syntax.
*
*   Termination signalling uses a reducing_state<_Acc> wrapper that
* downstream steps can mark as done to short-circuit (matching take_while
* behavior).
*
*   REQUIRES C++14: the helper classes here use generic lambdas (auto
* parameters) and deduced return types to abstract over the downstream
* reducer's value type without making each transducer doubly generic
* over both downstream-type and value-type. The rest of the djinterp
* functional module compiles on C++11. If C++14 is unavailable, this
* header is suppressed and the rest of the framework remains usable;
* expressing the same dataflow in C++11 means using view.hpp +
* accumulator.hpp + producer.hpp directly.
*
* USAGE:
*   auto xform = transducers::map([](int x) { return x * x; })
*              | transducers::filter([](int x) { return x % 2 == 0; })
*              | transducers::take(5);
*
*   // apply to a container -> vector
*   auto v = transduce_into_vector<int>(xform, std::vector<int>{1,2,3,4,5,6});
*
*   // apply to a container -> accumulator
*   auto s = transduce_into_accumulator(xform, sum<int>(), source_data);
*
*   // apply to a producer -> consumer
*   transduce_producer_to_consumer(xform, my_producer, my_consumer);
*
* 
* path:      /inc/djinterp/core/functional/transducer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    REDUCED WRAPPER
      1.  reduced<_Acc>                           (short-circuit signal)
      2.  is_reduced<T>
      3.  unwrap_reduced(r)
II.   CORE TRANSDUCERS  (namespace transducers)
      1.  map(f)                                  (transform each value)
      2.  filter(p)                               (drop non-matching)
      3.  filter_not(p)
      4.  take(n)                                 (first-n then short-circuit)
      5.  drop(n)
      6.  take_while(p)
      7.  drop_while(p)
      8.  distinct()
      9.  tap(side_effect)                        (peek without changing)
      10. flat_map(f)                             (one-to-many expansion)
      11. partition_by(key_fn)                    (chunk while key unchanged)
III.  COMPOSITION
      1.  compose(t1, t2, ...)                    (n-ary composition)
      2.  operator|(transducer, transducer)
IV.   DRIVERS / RUNNERS
      1.  transduce(xform, reducer, init, container)
      2.  transduce_into_vector<T>(xform, container)
      3.  transduce_into_accumulator(xform, acc, container)
      4.  transduce_producer_to_consumer(xform, producer, consumer)
      5.  into_reducer(xform, downstream)         (apply transducer to reducer)
V.    STRUCTURAL TRAITS & CONCEPTS
      1.  is_reducing_state<T>                     (a reducing_state<Acc>)
      2.  reducing_state_acc_t<T>                  (its accumulator type)
      3.  is_reducer<Fn, Acc, Value>               ((state&, const value&)->void)
      4.  is_transducer (marker, pre-existing)     (derives transducer_base)
      5.  transduces_reducer<X, Reducer>           (X applied to a reducer
                                                    yields a reducer)
      6.  transducer_result_t<X, Reducer>          (the produced reducer type)
      7.  *_v aliases  (C++14)                      (variable-template shorthands)
      8.  reducer_c / transducer_c / reducing_state_c (C++20)
*/


#ifndef DJINTERP_FUNCTIONAL_TRANSDUCER_
#define DJINTERP_FUNCTIONAL_TRANSDUCER_ 1

// std
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"
#include "./function_traits.hpp"


// transducer.hpp requires C++14 (generic lambdas and return type
// deduction). On C++11 the header is suppressed; the rest of the
// djinterp functional module remains usable. The same dataflow
// shape can be expressed in C++11 by composing view.hpp adapters
// with accumulator combinators and producer / consumer chains.
#if ( D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS  &&                              \
      D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO )


// D_CONSTEXPR14
//   macro: resolves to D_CONSTEXPR where relaxed (C++14) constexpr is
// available, and to nothing otherwise.  This header is already gated to
// C++14+, so it is effectively D_CONSTEXPR here; the guard mirrors
// consumer.hpp and keeps the spelling consistent across the module.
#ifndef D_CONSTEXPR14
#  if D_ENV_LANG_IS_CPP14_OR_HIGHER
#    define D_CONSTEXPR14 D_CONSTEXPR
#  else
#    define D_CONSTEXPR14
#  endif
#endif


NS_DJINTERP


//   DUAL DOMAIN.  A transducer is a transformation of a reducing step that is
// independent of both the source and the sink, so one transducer drives a
// runtime stream and a constant-evaluated fold alike.  The stateless and
// bounded transducers here - map, filter, take, drop, take_while, drop_while,
// flat_map (cat), and any composition of them - are D_CONSTEXPR14: applied to a
// constexpr step over a reducing_state<Acc> they fold during translation (a
// compile-time sum over a fixed sequence, say), and applied at run time they
// process a live stream unchanged.  distinct (it buffers seen values in an
// allocating set) and tap (its purpose is a side effect) remain runtime-only.
//   Scope of the lift: a transducer folds a HOMOGENEOUS value stream - a single
// Acc type threaded through reducing_state - in either domain.  The
// heterogeneous, type-or-value compile-time dataflow (folding a value_list, or
// unfolding to one) is the reduction substrate in reduce.hpp / producer.hpp; a
// transducer is the value-domain bridge between that compile-time face and the
// runtime view face (see view.hpp).

///////////////////////////////////////////////////////////////////////////////
///                         TRANSDUCER MARKER BASE                          ///
///////////////////////////////////////////////////////////////////////////////

// transducer_base
//   struct: CRTP-style marker that all transducer helpers in
// this module inherit from. Used by operator| to SFINAE-detect
// transducer operands without clashing with the operator|
// overloads in view.hpp, monad.hpp, comparator.hpp, etc.
//
//   Custom user transducers should also inherit from this so
// they participate in pipeline composition.
template<typename _Derived>
struct transducer_base
{
};


NS_INTERNAL

    // is_transducer_helper
    //   helper: detects whether _Type publicly inherits
    // transducer_base. Mirrors the is_view detection in view.hpp.
    template<typename _Type>
    struct is_transducer_helper
    {
    private:
        template<typename _T>
        static std::true_type  test(const transducer_base<_T>*);
        static std::false_type test(...);

    public:
        using type = decltype(test(static_cast<_Type*>(nullptr)));
    };

NS_END  // internal


// is_transducer
//   trait: true if _Type derives from transducer_base. Used by
// the pipeline operator| to constrain composition to genuine
// transducers.
template<typename _Type>
struct is_transducer
    : internal::is_transducer_helper<
          typename std::decay<_Type>::type>::type
{
};


///////////////////////////////////////////////////////////////////////////////
///             I.    REDUCED WRAPPER                                       ///
///////////////////////////////////////////////////////////////////////////////

// reduced
//   class: marks an accumulator as "finished early". Step
// functions may set their accumulator wrapper to reduced(state)
// to signal that no further values should be processed; drivers
// inspect this and stop iteration.
//
//   In this implementation, reduction is signalled by setting
// a separate flag on the accumulator-state wrapper rather than
// by changing types. This keeps step-function signatures simple
// (Acc&, const Value&) -> void and works across all sources.
//
//   The reduced<_Acc> type itself is provided as a convenience
// wrapper for callers who want to extract or check termination
// explicitly.
template<typename _Acc>
class reduced
{
public:
    template<typename _AccFwd,
             typename = typename std::enable_if<
                 !std::is_same<typename std::decay<_AccFwd>::type,
                               reduced>::value>::type>
    explicit D_CONSTEXPR
    reduced(
        _AccFwd&& _acc
    )
        : m_acc(std::forward<_AccFwd>(_acc))
    {}

    D_NODISCARD
    D_CONSTEXPR14
    const _Acc& value() const&
    {
        return m_acc;
    }

    D_NODISCARD
    D_CONSTEXPR14
    _Acc&& value() &&
    {
        return std::move(m_acc);
    }

private:
    _Acc m_acc;
};


// reducing_state
//   class: pairs an accumulator with a stop-flag. Step functions
// receive this by reference so they can both mutate the value
// and signal early termination.
template<typename _Acc>
class reducing_state
{
public:
    template<typename _AccFwd,
             typename = typename std::enable_if<
                 !std::is_same<typename std::decay<_AccFwd>::type,
                               reducing_state>::value>::type>
    explicit D_CONSTEXPR
    reducing_state(
        _AccFwd&& _acc
    )
        : m_acc(std::forward<_AccFwd>(_acc))
        , m_done(false)
    {}

    // accumulator (mutable)
    //   the live accumulator value; step functions mutate this.
    D_NODISCARD
    D_CONSTEXPR14
    _Acc& accumulator() noexcept
    {
        return m_acc;
    }

    // accumulator (const)
    D_NODISCARD
    D_CONSTEXPR14
    const _Acc& accumulator() const noexcept
    {
        return m_acc;
    }

    // is_done
    //   whether termination has been signalled. Drivers check
    // this after each step and stop iteration when true.
    D_NODISCARD D_CONSTEXPR bool is_done() const noexcept
    {
        return m_done;
    }

    // mark_done
    //   signal early termination. Subsequent step calls should
    // be skipped by the driver; transducers that buffer
    // (e.g. partition_by) may flush remaining state on the way
    // out (not implemented here; left as a future extension).
    D_CONSTEXPR14
    void mark_done() noexcept
    {
        m_done = true;

        return;
    }

private:
    _Acc m_acc;
    bool m_done;
};


///////////////////////////////////////////////////////////////////////////////
///             II.   CORE TRANSDUCERS                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // map_transducer_helper
    //   helper: when applied to a downstream reducer, returns a
    // new reducer that transforms each value via _function
    // before forwarding. _function may have any signature
    // V -> U so long as the downstream reducer accepts U.
    template<typename _Function>
    class map_transducer_helper
        : public transducer_base<map_transducer_helper<_Function>>
    {
    public:
        template<typename _FFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_FFwd>::type,
                                   map_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        map_transducer_helper(
            _FFwd&& _function
        )
            : m_function(std::forward<_FFwd>(_function))
        {}

        // operator() (transducer-as-callable)
        //   takes a downstream reducer and returns the wrapped
        // reducer. The wrapped reducer is a closure that
        // captures both this transducer's function and the
        // downstream reducer.
        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _Function fn = m_function;

            return [fn, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                _downstream(_state, fn(_value));
            };
        }

    private:
        _Function m_function;
    };


    // filter_transducer_helper
    //   helper: produces a reducer that forwards a value to the
    // downstream only when _predicate(value) is true.
    template<typename _Predicate>
    class filter_transducer_helper
        : public transducer_base<filter_transducer_helper<_Predicate>>
    {
    public:
        template<typename _PFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_PFwd>::type,
                                   filter_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        filter_transducer_helper(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _Predicate pred = m_predicate;

            return [pred, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                if (pred(_value))
                {
                    _downstream(_state, _value);
                }
            };
        }

    private:
        _Predicate m_predicate;
    };


    // take_transducer_helper
    //   helper: produces a reducer that forwards at most _n
    // values to the downstream, then marks the state as done.
    // The count is stored in the closure (mutable lambda) so
    // each transducer instance has independent state.
    class take_transducer_helper
        : public transducer_base<take_transducer_helper>
    {
    public:
        explicit D_CONSTEXPR
        take_transducer_helper(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            std::size_t remaining = m_n;

            return [remaining, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                if (remaining == 0)
                {
                    _state.mark_done();

                    return;
                }

                _downstream(_state, _value);
                --remaining;

                if (remaining == 0)
                {
                    _state.mark_done();
                }
            };
        }

    private:
        std::size_t m_n;
    };


    // drop_transducer_helper
    //   helper: produces a reducer that silently drops the
    // first _n values and forwards the rest.
    class drop_transducer_helper
        : public transducer_base<drop_transducer_helper>
    {
    public:
        explicit D_CONSTEXPR
        drop_transducer_helper(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            std::size_t remaining = m_n;

            return [remaining, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                if (remaining > 0)
                {
                    --remaining;

                    return;
                }

                _downstream(_state, _value);
            };
        }

    private:
        std::size_t m_n;
    };


    // take_while_transducer_helper
    //   helper: forwards values while _predicate holds; once
    // it fails, signals done and stops forwarding.
    template<typename _Predicate>
    class take_while_transducer_helper
        : public transducer_base<take_while_transducer_helper<_Predicate>>
    {
    public:
        template<typename _PFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_PFwd>::type,
                                   take_while_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        take_while_transducer_helper(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _Predicate pred = m_predicate;

            return [pred, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                if (!pred(_value))
                {
                    _state.mark_done();

                    return;
                }

                _downstream(_state, _value);
            };
        }

    private:
        _Predicate m_predicate;
    };


    // drop_while_transducer_helper
    //   helper: drops initial values while _predicate holds;
    // once it fails, forwards every subsequent value (without
    // re-evaluating the predicate).
    template<typename _Predicate>
    class drop_while_transducer_helper
        : public transducer_base<drop_while_transducer_helper<_Predicate>>
    {
    public:
        template<typename _PFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_PFwd>::type,
                                   drop_while_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        drop_while_transducer_helper(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _Predicate pred = m_predicate;
            bool       still_dropping = true;

            return [pred, _downstream, still_dropping](
                auto& _state,
                const auto& _value) mutable
            {
                if (still_dropping && pred(_value))
                {
                    return;
                }

                still_dropping = false;
                _downstream(_state, _value);
            };
        }

    private:
        _Predicate m_predicate;
    };


    // distinct_transducer_helper
    //   helper: forwards each value only the first time it is
    // seen by this particular reducer instance.
    //
    //   Design note: the value type is not known until the
    // reducer is invoked, so we cannot declare a typed seen-set
    // at transducer-construction time. We instead use a
    // shared_ptr to a type-erased buffer that the inner lambda
    // narrows to the correct vector type via reinterpret-style
    // access on first use; this is unsafe in general, so we
    // instead require a slightly less ergonomic but safer
    // approach: distinct() must be parameterised by the value
    // type explicitly.
    //
    //   For an inferred-type variant, the caller can use
    // filter(predicate) over a hashable container with a custom
    // bookkeeping closure.
    template<typename _Value>
    struct distinct_transducer_helper
        : public transducer_base<distinct_transducer_helper<_Value>>
    {
        template<typename _Downstream>
        auto operator()(
            _Downstream _downstream
        ) const
        {
            auto seen = std::make_shared<std::vector<_Value>>();

            return [seen, _downstream](
                auto&         _state,
                const _Value& _value) mutable
            {
                for (const auto& s : *seen)
                {
                    if (s == _value)
                    {
                        return;
                    }
                }

                seen->push_back(_value);
                _downstream(_state, _value);
            };
        }
    };


    // tap_transducer_helper
    //   helper: forwards every value to the downstream unchanged,
    // first invoking _side_effect on it. Useful for logging,
    // counting, or other peek-style operations.
    template<typename _SideEffect>
    class tap_transducer_helper
        : public transducer_base<tap_transducer_helper<_SideEffect>>
    {
    public:
        template<typename _SFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_SFwd>::type,
                                   tap_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        tap_transducer_helper(
            _SFwd&& _side_effect
        )
            : m_side_effect(std::forward<_SFwd>(_side_effect))
        {}

        template<typename _Downstream>
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _SideEffect side = m_side_effect;

            return [side, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                side(_value);
                _downstream(_state, _value);
            };
        }

    private:
        _SideEffect m_side_effect;
    };


    // flat_map_transducer_helper
    //   helper: produces a reducer that, for each input value,
    // invokes _function to produce a container and forwards each
    // element of that container to the downstream individually.
    template<typename _Function>
    class flat_map_transducer_helper
        : public transducer_base<flat_map_transducer_helper<_Function>>
    {
    public:
        template<typename _FFwd,
                 typename = typename std::enable_if<
                     !std::is_same<typename std::decay<_FFwd>::type,
                                   flat_map_transducer_helper>::value>::type>
        explicit D_CONSTEXPR
        flat_map_transducer_helper(
            _FFwd&& _function
        )
            : m_function(std::forward<_FFwd>(_function))
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            _Function fn = m_function;

            return [fn, _downstream](
                auto& _state,
                const auto& _value) mutable
            {
                auto expanded = fn(_value);

                for (const auto& element : expanded)
                {
                    if (_state.is_done())
                    {
                        return;
                    }

                    _downstream(_state, element);
                }
            };
        }

    private:
        _Function m_function;
    };


    // composed_transducer
    //   helper: composition of two transducers as a single
    // transducer. compose(t1, t2)(downstream) is equivalent to
    // t1(t2(downstream)). The result is again a transducer, so
    // composition associates and chains freely.
    template<typename _T1,
             typename _T2>
    class composed_transducer
        : public transducer_base<composed_transducer<_T1, _T2>>
    {
    public:
        template<typename _T1Fwd,
                 typename _T2Fwd>
        D_CONSTEXPR
        composed_transducer(
            _T1Fwd&& _t1,
            _T2Fwd&& _t2
        )
            : m_t1(std::forward<_T1Fwd>(_t1))
            , m_t2(std::forward<_T2Fwd>(_t2))
        {}

        template<typename _Downstream>
        D_CONSTEXPR14
        auto operator()(
            _Downstream _downstream
        ) const
        {
            // t1 outer, t2 inner. Reading left-to-right in user
            // code (a | b | c), each later stage sits "closer"
            // to the downstream sink; the outermost stage
            // receives values first. Hence: a | b means
            // a(b(downstream)).
            return m_t1(m_t2(_downstream));
        }

    private:
        _T1 m_t1;
        _T2 m_t2;
    };

NS_END  // internal


namespace transducers
{

    // map
    //   function: builds a transducer that, in the resulting
    // reducer, transforms each value via _function before
    // forwarding to the downstream.
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR internal::map_transducer_helper<typename std::decay<_Function>::type>
    map(
        _Function&& _function
    )
    {
        return internal::map_transducer_helper<
            typename std::decay<_Function>::type>(
                std::forward<_Function>(_function));
    }


    // filter
    //   function: builds a transducer that drops values for
    // which _predicate is false.
    template<typename _Predicate>
    D_NODISCARD D_CONSTEXPR internal::filter_transducer_helper<typename std::decay<_Predicate>::type>
    filter(
        _Predicate&& _predicate
    )
    {
        return internal::filter_transducer_helper<
            typename std::decay<_Predicate>::type>(
                std::forward<_Predicate>(_predicate));
    }


    // filter_not
    //   function: builds a transducer that drops values for
    // which _predicate is true (i.e. keeps the complement).
    // Implemented as filter with a wrapped predicate.
    template<typename _Predicate>
    D_NODISCARD D_CONSTEXPR auto filter_not(
        _Predicate _predicate
    )
    {
        return filter(
            [_predicate](const auto& _v) {
                return !_predicate(_v);
            });
    }


    // take
    //   function: builds a transducer that forwards at most _n
    // values and then signals termination.
    D_NODISCARD D_CONSTEXPR internal::take_transducer_helper
    take(
        std::size_t _n
    )
    {
        return internal::take_transducer_helper(_n);
    }


    // drop
    //   function: builds a transducer that silently skips the
    // first _n values.
    D_NODISCARD D_CONSTEXPR internal::drop_transducer_helper
    drop(
        std::size_t _n
    )
    {
        return internal::drop_transducer_helper(_n);
    }


    // take_while
    //   function: builds a transducer that forwards values
    // while _predicate holds, then signals termination at the
    // first failure.
    template<typename _Predicate>
    D_NODISCARD D_CONSTEXPR internal::take_while_transducer_helper<typename std::decay<_Predicate>::type>
    take_while(
        _Predicate&& _predicate
    )
    {
        return internal::take_while_transducer_helper<
            typename std::decay<_Predicate>::type>(
                std::forward<_Predicate>(_predicate));
    }


    // drop_while
    //   function: builds a transducer that drops initial values
    // matching _predicate, then forwards every subsequent value.
    template<typename _Predicate>
    D_NODISCARD D_CONSTEXPR internal::drop_while_transducer_helper<typename std::decay<_Predicate>::type>
    drop_while(
        _Predicate&& _predicate
    )
    {
        return internal::drop_while_transducer_helper<
            typename std::decay<_Predicate>::type>(
                std::forward<_Predicate>(_predicate));
    }


    // distinct
    //   function: builds a transducer that forwards each value
    // only the first time it is seen (across the lifetime of
    // the resulting reducer). The value type must be supplied
    // explicitly because it is not deducible from no arguments;
    // it must also support operator==.
    //
    //   Each call to distinct<T>() produces an independent
    // reducer with its own seen-set (held by shared_ptr inside
    // the closure).
    template<typename _Value>
    D_NODISCARD D_CONSTEXPR internal::distinct_transducer_helper<_Value>
    distinct()
    {
        return internal::distinct_transducer_helper<_Value>{};
    }


    // tap
    //   function: builds a transducer that forwards every value
    // unchanged, first invoking _side_effect on it. Useful for
    // logging, counting, and other peek-style operations.
    template<typename _SideEffect>
    D_NODISCARD D_CONSTEXPR internal::tap_transducer_helper<typename std::decay<_SideEffect>::type>
    tap(
        _SideEffect&& _side_effect
    )
    {
        return internal::tap_transducer_helper<
            typename std::decay<_SideEffect>::type>(
                std::forward<_SideEffect>(_side_effect));
    }


    // flat_map
    //   function: builds a transducer that, for each input
    // value, invokes _function to produce a container and
    // forwards each element of the container individually to
    // the downstream. The function must return an iterable.
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR internal::flat_map_transducer_helper<typename std::decay<_Function>::type>
    flat_map(
        _Function&& _function
    )
    {
        return internal::flat_map_transducer_helper<
            typename std::decay<_Function>::type>(
                std::forward<_Function>(_function));
    }

}   // namespace transducers


///////////////////////////////////////////////////////////////////////////////
///             III.  COMPOSITION                                           ///
///////////////////////////////////////////////////////////////////////////////

// compose (two transducers)
//   function: composes two transducers into one. compose(t1, t2)
// is a transducer that, when applied to a downstream reducer,
// produces t1(t2(downstream)) -- t1 sees values first, t2 sees
// them next (closer to the sink). This matches reading order:
// `compose(a, b, c)` means "first a, then b, then c".
template<typename _T1,
         typename _T2>
D_NODISCARD
D_CONSTEXPR
internal::composed_transducer<typename std::decay<_T1>::type,
                              typename std::decay<_T2>::type>
compose
(
    _T1&& _t1,
    _T2&& _t2
)
{
    return internal::composed_transducer<
        typename std::decay<_T1>::type,
        typename std::decay<_T2>::type>(
            std::forward<_T1>(_t1),
            std::forward<_T2>(_t2));
}


// compose (variadic)
//   function: variadic composition. compose(a, b, c, ...) folds
// from the left: compose(compose(a, b), c, ...). The fold uses
// recursive variadic template instantiation; for very long
// chains, prefer chaining with operator| instead.
template<typename _T1,
         typename _T2,
         typename _T3,
         typename... _Rest>
D_NODISCARD
D_CONSTEXPR
auto compose
(
    _T1&&        _t1,
    _T2&&        _t2,
    _T3&&        _t3,
    _Rest&&...   _rest
)
-> decltype(compose(
       compose(std::forward<_T1>(_t1), std::forward<_T2>(_t2)),
       std::forward<_T3>(_t3),
       std::forward<_Rest>(_rest)...))
{
    return compose(
        compose(std::forward<_T1>(_t1), std::forward<_T2>(_t2)),
        std::forward<_T3>(_t3),
        std::forward<_Rest>(_rest)...);
}


NS_INTERNAL

    // (placeholder for any future internal transducer-helper
    // utilities; the is_transducer_helper / transducer_base
    // pair above already covers detection.)

NS_END  // internal


// operator| (transducer | transducer)
//   composes two transducers via the pipeline operator. Order
// matches reading order: a | b means values flow into a first
// (a sees the outer source), then b (b is "closer to" the sink).
//
//   SFINAE-constrained to fire only when both operands derive
// from transducer_base. This avoids any clash with operator|
// overloads in view.hpp, monad.hpp, comparator.hpp, etc.
template<typename _T1,
         typename _T2,
         typename std::enable_if<
             ( is_transducer<_T1>::value &&
               is_transducer<_T2>::value ),
             int>::type = 0>
D_CONSTEXPR
auto operator|
(
    _T1&& _t1,
    _T2&& _t2
)
-> decltype(compose(std::forward<_T1>(_t1), std::forward<_T2>(_t2)))
{
    return compose(std::forward<_T1>(_t1), std::forward<_T2>(_t2));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   DRIVERS / RUNNERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// into_reducer
//   function: applies a transducer to a downstream reducer,
// producing the wrapped reducer. This is the primitive that
// every driver uses internally; exposed for callers building
// custom drivers.
template<typename _Transducer,
         typename _Downstream>
D_NODISCARD
auto into_reducer
(
    const _Transducer& _transducer,
    _Downstream        _downstream
)
-> decltype(_transducer(_downstream))
{
    return _transducer(_downstream);
}


// transduce
//   function: the general driver. Applies _transducer to
// _reducer to produce a wrapped reducer, then folds _container
// through that reducer starting from _init. Returns the final
// accumulator value.
//
//   _reducer must have signature
// (reducing_state<_Acc>&, const Value&) -> void  -- the standard
// transducer step shape.
template<typename _Transducer,
         typename _Reducer,
         typename _Acc,
         typename _Container>
D_NODISCARD
_Acc
transduce
(
    const _Transducer& _transducer,
    _Reducer           _reducer,
    _Acc               _init,
    const _Container&  _container
)
{
    reducing_state<_Acc> state(std::move(_init));
    auto wrapped = _transducer(_reducer);

    for (const auto& element : _container)
    {
        if (state.is_done())
        {
            break;
        }

        wrapped(state, element);
    }

    return state.accumulator();
}


// transduce_into_vector
//   function: drives a transducer over _container into a
// std::vector<_OutValue>. The downstream reducer simply pushes
// each value into the vector.
template<typename _OutValue,
         typename _Transducer,
         typename _Container>
D_NODISCARD
std::vector<_OutValue>
transduce_into_vector
(
    const _Transducer& _transducer,
    const _Container&  _container
)
{
    auto reducer = [](reducing_state<std::vector<_OutValue>>& _state,
                      const _OutValue& _value)
    {
        _state.accumulator().push_back(_value);
    };

    return transduce(_transducer, reducer,
                     std::vector<_OutValue>{}, _container);
}


// transduce_into_accumulator
//   function: drives a transducer over _container into an
// existing djinterp accumulator. The accumulator's step is
// adapted to the transducer protocol.
//
//   Requires that accumulator.hpp has been included; the
// definition is templated on _Accumulator without explicit
// dependency on the accumulator class so this header remains
// usable in isolation.
template<typename _Transducer,
         typename _Accumulator,
         typename _Container>
D_NODISCARD
auto transduce_into_accumulator
(
    const _Transducer& _transducer,
    _Accumulator       _accumulator,
    const _Container&  _container
)
-> decltype(_accumulator.finalize())
{
    using inner_value_t = typename _Accumulator::input_type;

    auto reducer = [&_accumulator](
        reducing_state<int>& _state,
        const inner_value_t& _value)
    {
        _accumulator.step(_value);
        (void)_state;
    };

    reducing_state<int> state(0);
    auto wrapped = _transducer(reducer);

    for (const auto& element : _container)
    {
        if (state.is_done())
        {
            break;
        }

        wrapped(state, element);
    }

    return _accumulator.finalize();
}


// transduce_producer_to_consumer
//   function: drives a transducer between a producer (from
// producer.hpp) and a consumer (from consumer.hpp). Pulls
// values from _producer one at a time, runs them through the
// transducer, and forwards each survivor to _consumer.
//
//   Requires that producer.hpp and consumer.hpp have been
// included for the producer_step protocol; the implementation
// uses only the pull interface (operator() returning a
// producer_step-like type).
template<typename _Transducer,
         typename _Producer,
         typename _Consumer>
void
transduce_producer_to_consumer
(
    const _Transducer& _transducer,
    _Producer&         _producer,
    _Consumer&         _consumer
)
{
    using value_t = typename _Producer::value_type;

    auto reducer = [&_consumer](
        reducing_state<int>& _state,
        const value_t&       _value)
    {
        _consumer(_value);
        (void)_state;
    };

    reducing_state<int> state(0);
    auto wrapped = _transducer(reducer);

    while (true)
    {
        if (state.is_done())
        {
            break;
        }

        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        wrapped(state, step.value);
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    STRUCTURAL TRAITS & CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////
//   Structural detection for the three shapes this module trades in:
//
//   * a REDUCING STATE  -- a reducing_state<_Acc> (the (accumulator, done)
//     pair drivers thread through a fold);
//   * a REDUCER         -- a callable of shape (reducing_state<_Acc>&,
//     const _Value&) -> void, the step function transducers wrap;
//   * a TRANSDUCER      -- a callable taking a reducer and returning a new
//     reducer. The existing is_transducer (above) recognises this module's
//     own helpers structurally via the transducer_base marker; the
//     transduces_reducer trait here is the behavioural counterpart: it asks
//     whether applying _Transducer to a SPECIFIC reducer type yields
//     something that is itself a reducer over the same (state, value).
//
//   A transducer's operator() is itself a template (it returns a generic
// lambda), so it cannot be probed without a concrete downstream reducer to
// apply it to. The behavioural traits therefore take the reducer type as a
// parameter. Detection is expression-based, reusing the call-detection
// facility added to function_traits.hpp.

NS_INTERNAL

    // reducing_state_probe
    //   helper: primary reports "not a reducing_state"; the specialization
    // recognises reducing_state<_Acc> and exposes its accumulator type.
    template<typename _Type>
    struct reducing_state_probe
    {
        static D_CONSTEXPR bool value = false;
    };

    template<typename _Acc>
    struct reducing_state_probe<reducing_state<_Acc> >
    {
        static D_CONSTEXPR bool value = true;
        using acc_type = _Acc;
    };

    // strip
    //   helper: removes reference and cv-qualifiers so the traits may be
    // queried on references and const types alike.
    template<typename _Type>
    struct strip
    {
        using type = typename std::remove_cv<
            typename std::remove_reference<_Type>::type>::type;
    };

NS_END  // internal


// is_reducing_state
//   trait: true when _Type is a reducing_state<_Acc> specialization (cv/ref
// stripped).
template<typename _Type>
struct is_reducing_state
    : std::integral_constant<bool,
          internal::reducing_state_probe<
              typename internal::strip<_Type>::type>::value>
{
};


// reducing_state_acc_t
//   alias: the accumulator type carried by a reducing_state<_Acc>. Ill-formed
// (SFINAE-friendly hard error) if _Type is not a reducing_state; query
// is_reducing_state first when that is possible.
template<typename _Type>
using reducing_state_acc_t =
    typename internal::reducing_state_probe<
        typename internal::strip<_Type>::type>::acc_type;


// is_reducer
//   trait: true when _Fn models the reducer step contract over _Acc / _Value
// -- callable as (reducing_state<_Acc>&, const _Value&). This is the shape
// every transducer wraps and every driver folds with. The result is
// conventionally void, but the trait does not insist on it: any well-formed
// call qualifies, so user reducers returning a value still register.
template<typename _Fn,
         typename _Acc,
         typename _Value>
struct is_reducer
    : std::integral_constant<bool,
          is_invocable_with<
              _Fn,
              reducing_state<_Acc>&,
              const _Value&>::value>
{
};


// transduces_reducer
//   trait: true when _Transducer, applied to a _Reducer, yields a callable
// (the new reducer). Because a transducer's operator() is templated, this is
// the behavioural test for "is _Transducer a transducer usable with this
// downstream reducer?" -- complementary to the marker-based is_transducer.
//   The produced reducer's own (state, value) shape depends on the upstream
// value type and so is not pinned here; transducer_result_t exposes the
// produced type for callers that need to probe it further.
template<typename _Transducer,
         typename _Reducer>
struct transduces_reducer
    : std::integral_constant<bool,
          is_invocable_with<_Transducer, _Reducer>::value>
{
};


// transducer_result_t
//   alias: the reducer type produced by applying _Transducer to _Reducer, or
// internal::call_nonesuch when the application is ill-formed.
template<typename _Transducer,
         typename _Reducer>
using transducer_result_t = call_result_t<_Transducer, _Reducer>;


// ---- convenience aliases ----
// Variable templates are a C++14 feature; transducer.hpp is already C++14+,
// so these are always available within this header, but the guard documents
// the dependency and keeps the pattern uniform with the rest of the module.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_reducing_state_v
//   constant: shorthand for is_reducing_state<_Type>::value.
template<typename _Type>
static constexpr bool is_reducing_state_v = is_reducing_state<_Type>::value;

// is_reducer_v
//   constant: shorthand for is_reducer<_Fn, _Acc, _Value>::value.
template<typename _Fn,
         typename _Acc,
         typename _Value>
static constexpr bool is_reducer_v = is_reducer<_Fn, _Acc, _Value>::value;

// is_transducer_v
//   constant: shorthand for is_transducer<_Type>::value.
template<typename _Type>
static constexpr bool is_transducer_v = is_transducer<_Type>::value;

// transduces_reducer_v
//   constant: shorthand for transduces_reducer<_Transducer, _Reducer>::value.
template<typename _Transducer,
         typename _Reducer>
static constexpr bool transduces_reducer_v =
    transduces_reducer<_Transducer, _Reducer>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ---- concepts (C++20) ----
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// reducing_state_c
//   concept: satisfied when _Type is a reducing_state specialization.
template<typename _Type>
concept reducing_state_c = is_reducing_state<_Type>::value;

// reducer_c
//   concept: satisfied when _Fn is callable as (reducing_state<_Acc>&,
// const _Value&) -- the reducer step contract.
template<typename _Fn,
         typename _Acc,
         typename _Value>
concept reducer_c =
    requires(const _Fn& _fn, reducing_state<_Acc>& _st, const _Value& _v)
    {
        _fn(_st, _v);
    };

// transducer_c
//   concept: satisfied when _Transducer derives from transducer_base AND
// applying it to _Reducer yields a callable. Pairs the marker-based identity
// with the behavioural application check.
template<typename _Transducer,
         typename _Reducer>
concept transducer_c =
    is_transducer<_Transducer>::value &&
    requires(const _Transducer& _x, _Reducer _r)
    {
        _x(_r);
    };

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS
        //   && D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO

#endif  // DJINTERP_FUNCTIONAL_TRANSDUCER_
