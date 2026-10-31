/*******************************************************************************
* djinterp [cpp]                                                    producer.hpp
*
* First-class producers (sources) for functional dataflow (C++).
*   A producer is a pull-driven source of values: a callable of signature
* `DProducerStep<A>()` returning a maybe-value indicating whether more data
* is available. This module elevates producers to first-class status with
* combinators for iteration, unfolding, repetition, cycling, and
* concatenation.
*   Producers are the dual of consumers: where consumers absorb values
* indefinitely without producing them, producers emit values indefinitely
* without absorbing them. Together with accumulators (state-folding sinks)
* and transformers, they form a complete dataflow vocabulary.
*
*   Producers are inherently lazy. Many of the producers in this module are
* notionally infinite (`repeat`, `cycle`, `iterate`); they only become
* finite when paired with a sink that takes a bounded prefix.
*
* USAGE:
*   // explicit iteration via the producer interface
*   auto fibs = iterate(std::make_pair(0, 1),
*       [](std::pair<int,int> p){
*           return std::make_pair(p.second, p.first + p.second);
*       });
*   for (int i = 0; i < 10; ++i)
*   {
*       auto step = fibs();             // step.has_value == true here
*       std::cout << step.value.first << '\n';
*   }
*
*   // bounded consumption via take_n
*   auto first_ten = take_n(fibs, 10).collect();
*
*   // sequencing: 1..3 then 100..102
*   auto seq = concat(range(1, 4), range(100, 103));
*
*
* path:      /inc/djinterp/cpp/functional/producer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRODUCER STEP TYPE
      ------------------
      1.    DProducerStep<T>                    (has_value + value)

II.   INTERNAL PRODUCER HELPER CLASSES
      --------------------------------
      1.    DIterateHelper                      (seed + step function)
      2.    DUnfoldHelper                       (state -> maybe value)
      3.    DRangeHelper                        (numeric range)
      4.    DRepeatHelper                       (infinite single value)
      5.    DRepeatNHelper                     (n-bounded single value)
      6.    DCycleHelper                        (repeat container forever)
      7.    DGenerateHelper                     (call nullary fn forever)
      8.    DEmptyProducerHelper
      9.    DSingleHelper                       (one-shot value)
      10.   DTakeNHelper                       (first n then exhaust)
      11.   DDropNHelper                       (skip first n)
      12.   DConcatHelper                       (sequence two producers)
      13.   DInterleaveHelper                   (alternate between two)
      14.   DTransformHelper                    (apply f to outputs)
      15.   DFilterHelper                       (keep matching outputs)

III.  PRODUCER FACTORIES   (flat in djinterp)
      ---------------------------------------
      1.    iterate
      2.    unfold
      3.    range / iota
      4.    repeat / repeat_n
      5.    cycle
      6.    generate
      7.    empty
      8.    single
      9.    take_n / drop_n
      10.   concat / interleave
      11.   transform / filter
      12.   from_container

IV.   TERMINAL CONVENIENCES
      ---------------------
      1.    collect                             (drain to vector)
      2.    for_each                            (apply consumer until exhaust)
      3.    fold                                (drain via accumulator step)

V.    PRODUCER TRAITS & CONCEPTS
      --------------------------
      1.    is_producer_step<T>                 (is T a DProducerStep?)
      2.    is_producer<T>                      (does T model the protocol?)
      3.    producer_value_type<T>              (the emitted value type)
      4.    producer_step_type / producer       (C++20 concepts)
*/

#ifndef DJINTERP_FUNCTIONAL_PRODUCER_HPP
#define DJINTERP_FUNCTIONAL_PRODUCER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./function_traits.hpp"
#include "./functor.hpp"
#include "./foldable.hpp"
#include "../meta/carrier.hpp"      // val_t / type_t leaves for the compile-time unfold
#include "../meta/value_list.hpp"   // materialization target of the compile-time unfold


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    PRODUCER STEP TYPE                                    ///
///////////////////////////////////////////////////////////////////////////////

// DProducerStep
//   struct: result of one pull from a producer. has_value indicates
// whether a value was produced; value is meaningful only when
// has_value is true. This is a minimal Maybe<T> dedicated to the
// producer protocol; producers do not depend on a full maybe<T>
// module to avoid circular include order.
template<typename Type>
struct DProducerStep
{
    bool   has_value;
    Type   value;

    D_CONSTEXPR
    DProducerStep()
        : has_value(false)
        , value()
    {}

    D_CONSTEXPR
    explicit DProducerStep(
        const Type& _value
    )
        : has_value(true)
        , value(_value)
    {}

    D_CONSTEXPR
    explicit DProducerStep(
        Type&& _value
    )
        : has_value(true)
        , value(std::move(_value))
    {}
};


// make_step
//   function: convenience builder for a DProducerStep containing a
// value. Decays the input type so that the resulting step holds a
// non-reference value.
template<typename Type>
D_CONSTEXPR
DProducerStep<typename std::decay<Type>::type> make_step(
    Type&& _value
)
{
    return DProducerStep<typename std::decay<Type>::type>(
        std::forward<Type>(_value));
}


// no_step
//   function: produces an empty step of the given type, signalling
// exhaustion to a downstream consumer.
template<typename Type>
D_CONSTEXPR
DProducerStep<Type> no_step()
{
    return DProducerStep<Type>();
}


///////////////////////////////////////////////////////////////////////////////
///             II.   INTERNAL PRODUCER HELPER CLASSES                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // DProducerBase
    //   CRTP mixin giving every producer helper a uniform collect()
    //   terminal. Previously collect() was defined only on a few helpers
    //   (take_n / drop_n / single), so range(...).collect(),
    //   transform(...).collect(), etc. did not compile despite the
    //   documented `.collect()` usage. Deriving every helper from this
    //   base provides collect() once, in terms of the derived operator().
    //   (added 2026-05-30)
    template<typename Derived>
    class DProducerBase
    {
    public:
        // collect
        //   pulls all remaining values into a vector by repeatedly
        // invoking the derived producer until exhaustion.
        template<typename D = Derived>
        std::vector<typename D::value_type>
        collect() const
        {
            const Derived& self = static_cast<const Derived&>(*this);
            std::vector<typename D::value_type> result;

            while (true)
            {
                auto step = self();

                if (!step.has_value)
                {
                    break;
                }

                result.push_back(std::move(step.value));
            }

            return result;
        }
    };

    // DIterateHelper
    //   helper: infinite producer driven by repeated application of
    // a step function to a seed. The seed is the first value emitted;
    // each subsequent value is _step(previous).
    template<typename Seed,
             typename Step>
    class DIterateHelper
        : public DProducerBase<DIterateHelper<Seed, Step>>
    {
    public:
        using value_type = Seed;
        using step_type  = DProducerStep<Seed>;

        template<typename SeedFwd,
                 typename StepFwd>
        D_CONSTEXPR
        DIterateHelper(
            SeedFwd&& _seed,
            StepFwd&& _step
        )
            : m_current(std::forward<SeedFwd>(_seed))
            , m_step(std::forward<StepFwd>(_step))
            , m_first(true)
        {}

        step_type operator()() const
        {
            if (m_first)
            {
                m_first = false;

                return make_step(m_current);
            }

            m_current = m_step(m_current);

            return make_step(m_current);
        }

    private:
        mutable Seed m_current;
        Step          m_step;
        mutable bool  m_first;
    };


    // DUnfoldHelper
    //   helper: open-ended producer in which the step function
    // examines the current state and may either yield a value (and
    // a new state) or signal exhaustion. Step has the signature
    // DProducerStep<pair<Value, State>>(State).
    template<typename State,
             typename Step,
             typename Value>
    class DUnfoldHelper
        : public DProducerBase<DUnfoldHelper<State, Step, Value>>
    {
    public:
        using value_type = Value;
        using step_type  = DProducerStep<Value>;

        template<typename StateFwd,
                 typename StepFwd>
        D_CONSTEXPR
        DUnfoldHelper(
            StateFwd&& _state,
            StepFwd&&  _step
        )
            : m_state(std::forward<StateFwd>(_state))
            , m_step(std::forward<StepFwd>(_step))
            , m_done(false)
        {}

        step_type operator()() const
        {
            if (m_done)
            {
                return no_step<Value>();
            }

            auto result = m_step(m_state);

            if (!result.has_value)
            {
                m_done = true;

                return no_step<Value>();
            }

            m_state = result.value.second;

            return make_step(result.value.first);
        }

    private:
        mutable State m_state;
        Step           m_step;
        mutable bool   m_done;
    };


    // DRangeHelper
    //   helper: numeric range from _start (inclusive) to _end
    // (exclusive) with optional step. Direction is determined by the
    // sign of _step; a zero step is treated as exhausted.
    template<typename Int>
    class DRangeHelper
        : public DProducerBase<DRangeHelper<Int>>
    {
    public:
        using value_type = Int;
        using step_type  = DProducerStep<Int>;

        D_CONSTEXPR
        DRangeHelper(
            Int _start,
            Int _end,
            Int _step
        )
            : m_current(_start)
            , m_end(_end)
            , m_step(_step)
        {}

        step_type operator()() const
        {
            // exhausted if step is zero, or if direction would not
            // close the gap
            if ( (m_step == Int(0)) ||
                 ( (m_step > Int(0)) && (m_current >= m_end) ) ||
                 ( (m_step < Int(0)) && (m_current <= m_end) ) )
            {
                return no_step<Int>();
            }

            Int v = m_current;
            m_current = static_cast<Int>(m_current + m_step);

            return make_step(v);
        }

    private:
        mutable Int m_current;
        Int          m_end;
        Int          m_step;
    };


    // DRepeatHelper
    //   helper: produces a stored value indefinitely.
    template<typename Value>
    class DRepeatHelper
        : public DProducerBase<DRepeatHelper<Value>>
    {
    public:
        using value_type = Value;
        using step_type  = DProducerStep<Value>;

        template<typename ValueFwd>
        explicit D_CONSTEXPR
        DRepeatHelper(
            ValueFwd&& _value
        )
            : m_value(std::forward<ValueFwd>(_value))
        {}

        step_type operator()() const
        {
            return make_step(m_value);
        }

    private:
        Value m_value;
    };


    // DRepeatNHelper
    //   helper: produces a stored value exactly _n times, then
    // signals exhaustion.
    template<typename Value>
    class DRepeatNHelper
        : public DProducerBase<DRepeatNHelper<Value>>
    {
    public:
        using value_type = Value;
        using step_type  = DProducerStep<Value>;

        template<typename ValueFwd>
        D_CONSTEXPR
        DRepeatNHelper(
            ValueFwd&& _value,
            std::size_t _n
        )
            : m_value(std::forward<ValueFwd>(_value))
            , m_remaining(_n)
        {}

        step_type operator()() const
        {
            if (m_remaining == 0)
            {
                return no_step<Value>();
            }

            --m_remaining;

            return make_step(m_value);
        }

    private:
        Value                       m_value;
        mutable std::size_t         m_remaining;
    };


    // DCycleHelper
    //   helper: cycles through a container forever. Holds the
    // container by value to avoid lifetime hazards. Yields exhaustion
    // immediately if the container is empty (avoiding a hang).
    template<typename Container>
    class DCycleHelper
        : public DProducerBase<DCycleHelper<Container>>
    {
    public:
        using value_type = typename std::decay<decltype(
            *std::begin(std::declval<const Container&>()))>::type;
        using step_type  = DProducerStep<value_type>;

        template<typename ContainerFwd>
        explicit D_CONSTEXPR
        DCycleHelper(
            ContainerFwd&& _container
        )
            : m_container(std::forward<ContainerFwd>(_container))
            , m_it(std::begin(m_container))
        {}

        step_type operator()() const
        {
            if (m_container.empty())
            {
                return no_step<value_type>();
            }

            if (m_it == std::end(m_container))
            {
                m_it = std::begin(m_container);
            }

            value_type v = *m_it;
            ++m_it;

            return make_step(std::move(v));
        }

    private:
        Container m_container;
        mutable typename Container::const_iterator m_it;
    };


    // DGenerateHelper
    //   helper: invokes a nullary function on each pull and yields
    // its result. Useful for random numbers, time samples, etc.
    template<typename Function>
    class DGenerateHelper
        : public DProducerBase<DGenerateHelper<Function>>
    {
    public:
        using value_type = typename std::decay<
            decltype(std::declval<Function&>()())>::type;
        using step_type  = DProducerStep<value_type>;

        template<typename FunctionFwd>
        explicit D_CONSTEXPR
        DGenerateHelper(
            FunctionFwd&& _function
        )
            : m_function(std::forward<FunctionFwd>(_function))
        {}

        step_type operator()() const
        {
            return make_step(m_function());
        }

    private:
        mutable Function m_function;
    };


    // DEmptyProducerHelper
    //   helper: produces no values. Useful as an identity for concat.
    template<typename Type>
    struct DEmptyProducerHelper
        : public DProducerBase<DEmptyProducerHelper<Type>>
    {
        using value_type = Type;
        using step_type  = DProducerStep<Type>;

        D_CONSTEXPR
        step_type operator()() const
        {
            return no_step<Type>();
        }
    };


    // DSingleHelper
    //   helper: produces a single stored value, then signals
    // exhaustion. Equivalent to repeat_n(_value, 1) but cheaper.
    template<typename Value>
    class DSingleHelper
        : public DProducerBase<DSingleHelper<Value>>
    {
    public:
        using value_type = Value;
        using step_type  = DProducerStep<Value>;

        template<typename ValueFwd>
        explicit D_CONSTEXPR
        DSingleHelper(
            ValueFwd&& _value
        )
            : m_value(std::forward<ValueFwd>(_value))
            , m_done(false)
        {}

        step_type operator()() const
        {
            if (m_done)
            {
                return no_step<Value>();
            }

            m_done = true;

            return make_step(m_value);
        }

    private:
        Value        m_value;
        mutable bool m_done;
    };


    // DTakeNHelper
    //   helper: forwards at most _n values from an inner producer,
    // then signals exhaustion regardless of whether the inner
    // producer is exhausted.
    template<typename Producer>
    class DTakeNHelper
        : public DProducerBase<DTakeNHelper<Producer>>
    {
    public:
        using value_type = typename Producer::value_type;
        using step_type  = DProducerStep<value_type>;

        template<typename ProducerFwd>
        D_CONSTEXPR
        DTakeNHelper(
            ProducerFwd&& _producer,
            std::size_t    _n
        )
            : m_producer(std::forward<ProducerFwd>(_producer))
            , m_remaining(_n)
        {}

        step_type operator()() const
        {
            if (m_remaining == 0)
            {
                return no_step<value_type>();
            }

            auto step = m_producer();

            if (!step.has_value)
            {
                m_remaining = 0;

                return step;
            }

            --m_remaining;

            return step;
        }

        // collect
        //   convenience: pulls all remaining values into a vector and
        // returns it. Equivalent to repeatedly invoking operator()
        // until exhaustion.
        std::vector<value_type> collect() const
        {
            std::vector<value_type> result;

            while (true)
            {
                auto step = (*this)();

                if (!step.has_value)
                {
                    break;
                }

                result.push_back(std::move(step.value));
            }

            return result;
        }

    private:
        mutable Producer     m_producer;
        mutable std::size_t  m_remaining;
    };


    // DDropNHelper
    //   helper: pulls and discards _n values from an inner producer
    // on first use, then forwards subsequent pulls. If the inner
    // producer is exhausted within the first _n pulls, the wrapper
    // is also exhausted.
    template<typename Producer>
    class DDropNHelper
        : public DProducerBase<DDropNHelper<Producer>>
    {
    public:
        using value_type = typename Producer::value_type;
        using step_type  = DProducerStep<value_type>;

        template<typename ProducerFwd>
        D_CONSTEXPR
        DDropNHelper(
            ProducerFwd&& _producer,
            std::size_t    _n
        )
            : m_producer(std::forward<ProducerFwd>(_producer))
            , m_to_drop(_n)
        {}

        step_type operator()() const
        {
            while (m_to_drop > 0)
            {
                auto step = m_producer();

                if (!step.has_value)
                {
                    m_to_drop = 0;

                    return step;
                }

                --m_to_drop;
            }

            return m_producer();
        }

    private:
        mutable Producer     m_producer;
        mutable std::size_t  m_to_drop;
    };


    // DConcatHelper
    //   helper: emits all values of _first, then all values of
    // _second. _first and _second must have the same value_type.
    template<typename First,
             typename Second>
    class DConcatHelper
        : public DProducerBase<DConcatHelper<First, Second>>
    {
    public:
        using value_type = typename First::value_type;
        using step_type  = DProducerStep<value_type>;

        template<typename FirstFwd,
                 typename SecondFwd>
        D_CONSTEXPR
        DConcatHelper(
            FirstFwd&&  _first,
            SecondFwd&& _second
        )
            : m_first(std::forward<FirstFwd>(_first))
            , m_second(std::forward<SecondFwd>(_second))
            , m_first_done(false)
        {}

        step_type operator()() const
        {
            if (!m_first_done)
            {
                auto step = m_first();

                if (step.has_value)
                {
                    return step;
                }

                m_first_done = true;
            }

            return m_second();
        }

    private:
        mutable First    m_first;
        mutable Second   m_second;
        mutable bool     m_first_done;
    };


    // DInterleaveHelper
    //   helper: alternates one pull from each of two producers. If
    // either producer is exhausted, the wrapper signals exhaustion
    // immediately (does NOT fall back to the longer one).
    template<typename First,
             typename Second>
    class DInterleaveHelper
        : public DProducerBase<DInterleaveHelper<First, Second>>
    {
    public:
        using value_type = typename First::value_type;
        using step_type  = DProducerStep<value_type>;

        template<typename FirstFwd,
                 typename SecondFwd>
        D_CONSTEXPR
        DInterleaveHelper(
            FirstFwd&&  _first,
            SecondFwd&& _second
        )
            : m_first(std::forward<FirstFwd>(_first))
            , m_second(std::forward<SecondFwd>(_second))
            , m_turn(false)
        {}

        step_type operator()() const
        {
            step_type step = m_turn ? m_second() : m_first();
            m_turn = !m_turn;

            return step;
        }

    private:
        mutable First    m_first;
        mutable Second   m_second;
        mutable bool     m_turn;
    };


    // DTransformHelper
    //   helper: applies a function to each value emitted by an inner
    // producer. value_type is the result of the function applied to
    // the inner value_type.
    template<typename Producer,
             typename Function>
    class DTransformHelper
        : public DProducerBase<DTransformHelper<Producer, Function>>
    {
    public:
        using source_type = typename Producer::value_type;
        using value_type  = typename std::decay<decltype(
            std::declval<Function&>()(
                std::declval<const source_type&>()))>::type;
        using step_type   = DProducerStep<value_type>;

        template<typename ProducerFwd,
                 typename FunctionFwd>
        D_CONSTEXPR
        DTransformHelper(
            ProducerFwd&& _producer,
            FunctionFwd&& _function
        )
            : m_producer(std::forward<ProducerFwd>(_producer))
            , m_function(std::forward<FunctionFwd>(_function))
        {}

        step_type operator()() const
        {
            auto step = m_producer();

            if (!step.has_value)
            {
                return no_step<value_type>();
            }

            return make_step(m_function(step.value));
        }

    private:
        mutable Producer m_producer;
        mutable Function m_function;
    };


    // DFilterHelper
    //   helper: forwards only those values from an inner producer
    // that satisfy a predicate. Pulls repeatedly from the inner
    // producer until a match or exhaustion is found.
    template<typename Producer,
             typename Predicate>
    class DFilterHelper
        : public DProducerBase<DFilterHelper<Producer, Predicate>>
    {
    public:
        using value_type = typename Producer::value_type;
        using step_type  = DProducerStep<value_type>;

        template<typename ProducerFwd,
                 typename PredicateFwd>
        D_CONSTEXPR
        DFilterHelper(
            ProducerFwd&&  _producer,
            PredicateFwd&& _predicate
        )
            : m_producer(std::forward<ProducerFwd>(_producer))
            , m_predicate(std::forward<PredicateFwd>(_predicate))
        {}

        step_type operator()() const
        {
            while (true)
            {
                auto step = m_producer();

                if (!step.has_value)
                {
                    return step;
                }

                if (m_predicate(step.value))
                {
                    return step;
                }
            }
        }

    private:
        mutable Producer   m_producer;
        mutable Predicate m_predicate;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  PRODUCER FACTORIES                                    ///
///////////////////////////////////////////////////////////////////////////////

// NOTE: the producer factories below were previously nested in a
// `namespace producers` sub-namespace; they are now flat in djinterp so
// callers write `iterate(...)`, `range(...)`, etc. directly. The internal
// helper classes they return remain in `djinterp::internal`.

    // iterate
    //   function: infinite producer iterate(x, f) yielding x, f(x),
    // f(f(x)), and so on. Useful for arithmetic progressions,
    // recurrences, and other deterministic sequences.
    template<typename Seed,
             typename Step>
    D_CONSTEXPR
    internal::DIterateHelper<typename std::decay<Seed>::type,
                             typename std::decay<Step>::type>
    iterate(
        Seed&& _seed,
        Step&& _step
    )
    {
        return internal::DIterateHelper<
            typename std::decay<Seed>::type,
            typename std::decay<Step>::type>(
                std::forward<Seed>(_seed),
                std::forward<Step>(_step));
    }


    // unfold
    //   function: builds a producer from a state seed and a step
    // function of signature DProducerStep<pair<Value, State>>(State).
    // Returning an empty step signals exhaustion. Value must be
    // supplied explicitly because the step's return type is generally
    // not deducible from the state alone.
    template<typename Value,
             typename State,
             typename Step>
    D_CONSTEXPR
    internal::DUnfoldHelper<typename std::decay<State>::type,
                            typename std::decay<Step>::type,
                            Value>
    unfold(
        State&& _state,
        Step&&  _step
    )
    {
        return internal::DUnfoldHelper<
            typename std::decay<State>::type,
            typename std::decay<Step>::type,
            Value>(
                std::forward<State>(_state),
                std::forward<Step>(_step));
    }


    // range
    //   function: numeric range producer. Emits _start, _start + _step,
    // ..., stopping when _step's direction would not bring the value
    // closer to _end. _step may be negative for descending ranges.
    template<typename Int>
    D_CONSTEXPR
    internal::DRangeHelper<Int> range(
        Int _start,
        Int _end,
        Int _step
    )
    {
        return internal::DRangeHelper<Int>(_start, _end, _step);
    }


    // range (default step)
    //   function: half-open range [_start, _end) with step = 1.
    template<typename Int>
    D_CONSTEXPR
    internal::DRangeHelper<Int> range(
        Int _start,
        Int _end
    )
    {
        return internal::DRangeHelper<Int>(_start, _end, Int(1));
    }


    // iota
    //   function: alias for range with start and end, matching the
    // STL <numeric>/<ranges> naming convention.
    template<typename Int>
    D_CONSTEXPR
    internal::DRangeHelper<Int> iota(
        Int _start,
        Int _end
    )
    {
        return internal::DRangeHelper<Int>(_start, _end, Int(1));
    }


    // repeat
    //   function: infinite producer emitting _value on every pull.
    // Pair with take_n or a bounded consumer to make it finite.
    template<typename Value>
    D_CONSTEXPR
    internal::DRepeatHelper<typename std::decay<Value>::type> repeat(
        Value&& _value
    )
    {
        return internal::DRepeatHelper<typename std::decay<Value>::type>(
            std::forward<Value>(_value));
    }


    // repeat_n
    //   function: emits _value exactly _n times, then exhausts.
    template<typename Value>
    D_CONSTEXPR
    internal::DRepeatNHelper<typename std::decay<Value>::type> repeat_n(
        Value&&     _value,
        std::size_t _n
    )
    {
        return internal::DRepeatNHelper<typename std::decay<Value>::type>(
            std::forward<Value>(_value), _n);
    }


    // cycle
    //   function: infinite producer that emits the elements of
    // _container in order, restarting from the beginning each time
    // the end is reached. Empty containers exhaust immediately.
    template<typename Container>
    D_CONSTEXPR
    internal::DCycleHelper<typename std::decay<Container>::type> cycle(
        Container&& _container
    )
    {
        return internal::DCycleHelper<
            typename std::decay<Container>::type>(
                std::forward<Container>(_container));
    }


    // generate
    //   function: infinite producer that invokes a nullary function
    // on every pull and emits its result.
    template<typename Function>
    D_CONSTEXPR
    internal::DGenerateHelper<typename std::decay<Function>::type> generate(
        Function&& _function
    )
    {
        return internal::DGenerateHelper<
            typename std::decay<Function>::type>(
                std::forward<Function>(_function));
    }


    // empty
    //   function: producer that yields no values. The element type
    // must be explicit since there is no other source for it.
    template<typename Type>
    D_CONSTEXPR
    internal::DEmptyProducerHelper<Type> empty()
    {
        return internal::DEmptyProducerHelper<Type>{};
    }


    // single
    //   function: producer that emits _value once and then exhausts.
    template<typename Value>
    D_CONSTEXPR
    internal::DSingleHelper<typename std::decay<Value>::type> single(
        Value&& _value
    )
    {
        return internal::DSingleHelper<typename std::decay<Value>::type>(
            std::forward<Value>(_value));
    }


    // take_n
    //   function: bounds an inner producer to its first _n outputs.
    // The resulting producer also exposes a .collect() method for
    // direct conversion to vector.
    template<typename Producer>
    D_CONSTEXPR
    internal::DTakeNHelper<typename std::decay<Producer>::type> take_n(
        Producer&&  _producer,
        std::size_t _n
    )
    {
        return internal::DTakeNHelper<
            typename std::decay<Producer>::type>(
                std::forward<Producer>(_producer), _n);
    }


    // drop_n
    //   function: discards the first _n outputs of an inner producer
    // and forwards the rest.
    template<typename Producer>
    D_CONSTEXPR
    internal::DDropNHelper<typename std::decay<Producer>::type> drop_n(
        Producer&&  _producer,
        std::size_t _n
    )
    {
        return internal::DDropNHelper<
            typename std::decay<Producer>::type>(
                std::forward<Producer>(_producer), _n);
    }


    // concat
    //   function: sequence two producers; emits all of _first, then
    // all of _second.
    template<typename First,
             typename Second>
    D_CONSTEXPR
    internal::DConcatHelper<typename std::decay<First>::type,
                            typename std::decay<Second>::type>
    concat(
        First&&  _first,
        Second&& _second
    )
    {
        return internal::DConcatHelper<
            typename std::decay<First>::type,
            typename std::decay<Second>::type>(
                std::forward<First>(_first),
                std::forward<Second>(_second));
    }


    // interleave
    //   function: alternate one pull from each of two producers.
    // Exhausts as soon as either inner producer exhausts.
    template<typename First,
             typename Second>
    D_CONSTEXPR
    internal::DInterleaveHelper<typename std::decay<First>::type,
                                typename std::decay<Second>::type>
    interleave(
        First&&  _first,
        Second&& _second
    )
    {
        return internal::DInterleaveHelper<
            typename std::decay<First>::type,
            typename std::decay<Second>::type>(
                std::forward<First>(_first),
                std::forward<Second>(_second));
    }


    // transform
    //   function: producer wrapper that applies _function to each
    // value emitted by _producer.
    template<typename Producer,
             typename Function>
    D_CONSTEXPR
    internal::DTransformHelper<typename std::decay<Producer>::type,
                               typename std::decay<Function>::type>
    transform(
        Producer&& _producer,
        Function&& _function
    )
    {
        return internal::DTransformHelper<
            typename std::decay<Producer>::type,
            typename std::decay<Function>::type>(
                std::forward<Producer>(_producer),
                std::forward<Function>(_function));
    }


    // filter
    //   function: producer wrapper that emits only those inner values
    // satisfying _predicate. Each pull may invoke the inner producer
    // multiple times until a match (or exhaustion) is found.
    template<typename Producer,
             typename Predicate>
    D_CONSTEXPR
    internal::DFilterHelper<typename std::decay<Producer>::type,
                            typename std::decay<Predicate>::type>
    filter(
        Producer&&  _producer,
        Predicate&& _predicate
    )
    {
        return internal::DFilterHelper<
            typename std::decay<Producer>::type,
            typename std::decay<Predicate>::type>(
                std::forward<Producer>(_producer),
                std::forward<Predicate>(_predicate));
    }


    // from_container
    //   function: turns any iterable container into a finite producer.
    // The container is held by value; pass by std::move to avoid a
    // copy. The producer exhausts when the iterator reaches end().
    template<typename Container>
    class DFromContainerProducer
        : public internal::DProducerBase<DFromContainerProducer<Container> >
    {
    public:
        using container_type = typename std::decay<Container>::type;
        using value_type     = typename std::decay<decltype(
            *std::begin(std::declval<const container_type&>()))>::type;
        using step_type      = DProducerStep<value_type>;

        template<typename ContainerFwd>
        explicit D_CONSTEXPR
        DFromContainerProducer(
            ContainerFwd&& _container
        )
            : m_container(std::forward<ContainerFwd>(_container))
            , m_it(std::begin(m_container))
            , m_end(std::end(m_container))
        {}

        step_type operator()() const
        {
            if (m_it == m_end)
            {
                return no_step<value_type>();
            }

            value_type v = *m_it;
            ++m_it;

            return make_step(std::move(v));
        }

    private:
        container_type m_container;
        mutable typename container_type::const_iterator m_it;
        typename container_type::const_iterator         m_end;
    };

    // from_container (factory)
    template<typename Container>
    D_CONSTEXPR
    DFromContainerProducer<typename std::decay<Container>::type> from_container(
        Container&& _container
    )
    {
        return DFromContainerProducer<
            typename std::decay<Container>::type>(
                std::forward<Container>(_container));
    }


///////////////////////////////////////////////////////////////////////////////
///             IV.   TERMINAL CONVENIENCES                                 ///
///////////////////////////////////////////////////////////////////////////////

// collect
//   function: drains a producer (which must be finite or already
// bounded) into a std::vector. Pulls until the producer signals
// exhaustion.
template<typename Producer>
D_NODISCARD std::vector<typename Producer::value_type> collect(
    Producer& _producer
)
{
    std::vector<typename Producer::value_type> result;

    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        result.push_back(std::move(step.value));
    }

    return result;
}


// for_each (producer)
//   function: pulls every value from _producer and forwards it to
// _consumer, until the producer signals exhaustion.
template<typename Producer,
         typename Consumer>
void for_each(
    Producer& _producer,
    Consumer  _consumer
)
{
    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        _consumer(step.value);
    }

    return;
}

// fold (producer)
//   function: drains a producer through a binary step function and
// accumulator, returning the final accumulated value.
template<typename Producer,
         typename Acc,
         typename Step>
D_NODISCARD Acc fold(
    Producer& _producer,
    Acc       _init,
    Step      _step
)
{
    while (true)
    {
        auto step = _producer();

        if (!step.has_value)
        {
            break;
        }

        _init = _step(static_cast<const Acc&>(_init), step.value);
    }

    return _init;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    PRODUCER TRAITS & CONCEPTS                            ///
///////////////////////////////////////////////////////////////////////////////
// SFINAE-friendly structural introspection of the producer protocol. A
// "producer" is a const-invocable nullary callable whose result is a
// DProducerStep<V>, and which advertises that V via a nested value_type.
// These traits surface that contract as first-class, and the C++20 concepts
// wrap them for use in requires-clauses.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // producer_make_void
    //   trait: header-local map from any type sequence to void; the
    // foundation for the detection idiom below. Kept local so the trait
    // block carries no dependency on an external void_t facility.
    template<typename...>
    struct producer_make_void
    {
        typedef void type;
    };

    // producer_void_t
    //   type: header-local alias for producer_make_void<...>::type.
    template<typename... Types>
    using producer_void_t = typename producer_make_void<Types...>::type;


    // is_producer_step_helper
    //   trait: detects whether Type is a DProducerStep specialization
    // (primary / failure case).
    template<typename Type>
    struct is_producer_step_helper : std::false_type
    {};

    // is_producer_step_helper<DProducerStep<Value>>
    //   trait: success specialization for DProducerStep.
    template<typename Value>
    struct is_producer_step_helper<DProducerStep<Value> > : std::true_type
    {};


    // is_producer_helper
    //   trait: detects whether Type satisfies the producer protocol --
    // it has a nested value_type, is const-invocable with no arguments,
    // and the call result is a DProducerStep. Primary / failure case.
    template<typename Type,
             typename AlwaysVoid = void>
    struct is_producer_helper : std::false_type
    {};

    // is_producer_helper (success case)
    //   trait: specialization that fires only when every protocol element
    // is well-formed.
    template<typename Type>
    struct is_producer_helper<
        Type,
        producer_void_t<
            typename Type::value_type,
            decltype(std::declval<const Type&>()())> >
        : is_producer_step_helper<
              typename std::decay<
                  decltype(std::declval<const Type&>()())>::type>
    {};

NS_END  // internal


// is_producer_step
//   trait: true if Type (decayed) is a DProducerStep specialization.
template<typename Type>
struct is_producer_step
    : internal::is_producer_step_helper<typename std::decay<Type>::type>
{};


// is_producer
//   trait: true if Type (decayed) models the producer protocol: a nested
// value_type, const-nullary-invocable, returning a DProducerStep.
template<typename Type>
struct is_producer
    : internal::is_producer_helper<typename std::decay<Type>::type>
{};


// producer_value_type
//   trait: extracts the value_type a producer emits. Only well-formed when
// is_producer<Type>::value is true; intended for use behind that guard.
template<typename Type>
struct producer_value_type
{
    typedef typename std::decay<Type>::type::value_type type;
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_producer_step_v
    //   value: convenience alias for is_producer_step<Type>::value.
    template<typename Type>
    static constexpr bool is_producer_step_v =
        is_producer_step<Type>::value;

    // is_producer_v
    //   value: convenience alias for is_producer<Type>::value.
    template<typename Type>
    static constexpr bool is_producer_v = is_producer<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // producer_step_type
    //   concept: satisfied by any DProducerStep specialization.
    template<typename Type>
    concept producer_step_type = is_producer_step<Type>::value;

    // producer
    //   concept: satisfied by any type modeling the producer protocol.
    template<typename Type>
    concept producer = is_producer<Type>::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#if D_ENV_LANG_IS_CPP17_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
///             X.    COMPILE-TIME UNFOLD DRIVER                            ///
///////////////////////////////////////////////////////////////////////////////
//   The producers above are the RUNTIME source driver: a stateful pull source
// whose unfold step has the shape  State -> DProducerStep<pair<Value, State>>
// (see DUnfoldHelper), materialized eagerly by DProducerBase::collect().  This
// section adds the COMPILE-TIME driver: it runs a pure unfold step over carrier
// states (val_t / type_t) to a fixed point during translation, materializing
// the produced value carriers into a DValueList - the same source description
// drives both a runtime computation and a constant-evaluated one.
//
//   The compile-time step result is a DISTINCT type per case (some_t vs none_t)
// rather than DProducerStep<...>: DProducerStep records exhaustion in a runtime
// bool field, which a type-level driver cannot branch on, whereas some_t /
// none_t put the decision in the type so template recursion can dispatch on it.
// A step is an ordinary constexpr callable that returns one or the other, e.g.
//
//   template<auto Limit>
//   struct iota_step
//   {
//       template<auto I>
//       D_CONSTEXPR auto operator()(val_t<I>) const
//       {
//           if constexpr (I < Limit) return some_t<val_t<I>, val_t<I + 1>>{};
//           else                       return none_t{};
//       }
//   };
//   using first_five = unfold_ct_t<iota_step<5>, val_t<0>>;  // DValueList<0,1,2,3,4>

// none_t
//   compile-time unfold step result: the source is exhausted.
struct none_t
{};

// some_t
//   compile-time unfold step result: a value carrier Value together with the
// next state carrier Next.  Distinct from none_t at the type level so the
// driver can branch by type.
template<typename Value,
         typename Next>
struct some_t
{
    using value = Value;
    using next  = Next;
};

NS_INTERNAL

    // unfold_ct_helper
    //   metafunction: type-level recursion that drives a constexpr unfold Step
    // from a state to exhaustion, growing the accumulated DValueList Acc.
    // Dispatched on the step's result type StepResult (none_t or some_t<V,N>).
    template<typename Step,
             typename Acc,
             typename StepResult>
    struct unfold_ct_helper;

    // exhausted: the accumulated list is the result.
    template<typename Step,
             typename Acc>
    struct unfold_ct_helper<Step, Acc, none_t>
    {
        using type = Acc;
    };

    // yielded value carrier Value with next state carrier Next: append the
    // value to the list and recurse on the next state.
    template<typename Step,
             typename Acc,
             typename Value,
             typename Next>
    struct unfold_ct_helper<Step, Acc, some_t<Value, Next>>
    {
        using grown = decltype(
            append(std::declval<Acc>(), std::declval<Value>()));

        using type = typename unfold_ct_helper<
            Step,
            grown,
            decltype(std::declval<const Step&>()(std::declval<Next>()))
        >::type;
    };

NS_END

// unfold_ct_t
//   type: the DValueList produced by running constexpr unfold Step from the
// initial state carrier Init to exhaustion at compile time.
template<typename Step,
         typename Init>
using unfold_ct_t = typename internal::unfold_ct_helper<
    Step,
    DValueList<>,
    decltype(std::declval<const Step&>()(std::declval<Init>()))
>::type;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER




///////////////////////////////////////////////////////////////////////////////
///             XI.   FUNCTOR INSTANCE                                      ///
///////////////////////////////////////////////////////////////////////////////
//   A producer is a Functor: the flat transform(producer, f) is its map. This
// teaches the generic functor_map (functor.hpp) to drive a producer through
// the one canonical name, so the same call that maps a maybe / result / view
// also maps a producer. A producer's mapped type is
// internal::DTransformHelper<P, F> -- it depends on the mapping function F, so
// there is no single F<U> to rebind; the result type follows from the existing
// transform and is deduced. Keyed on is_producer, mutually exclusive with the
// monad bridge in functor.hpp (a producer is not a monad) and the view
// instance.

template<typename Producer>
struct functor_traits<
    Producer,
    typename std::enable_if<is_producer<Producer>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename producer_value_type<Producer>::type;

    // map
    //   functorial map by delegating to the flat transform combinator (the
    // existing per-type fmap for producers). Lazy: the wrapped producer is
    // pulled only when the result producer is invoked.
    template<typename ProducerArg,
             typename Function>
    static D_CONSTEXPR
    auto map(
        ProducerArg&& _producer,
        Function&&    _function
    ) -> decltype(::djinterp::transform( std::forward<ProducerArg>(_producer), std::forward<Function>(_function)))
    {
        return ::djinterp::transform(
            std::forward<ProducerArg>(_producer),
            std::forward<Function>(_function));
    }
};


///////////////////////////////////////////////////////////////////////////////
///             XII.  FOLDABLE INSTANCE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A producer is a Foldable: its elements are collapsed by pulling the source
// to exhaustion and threading the reducer through them. This teaches the
// generic fold_left (foldable.hpp) -- and therefore fold_to_vector,
// fold_length, fold_any, fold_all, fold_right, ... -- to drive a producer
// through the one canonical name. Keyed on is_producer so the single instance
// covers every producer; mutually exclusive with the maybe / result / view
// instances. The fold is non-destructive (a copy is pulled, leaving the
// caller's producer untouched); an infinite producer must be bounded before
// folding.

template<typename Producer>
struct foldable_traits<
    Producer,
    typename std::enable_if<is_producer<Producer>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename producer_value_type<Producer>::type;

    // fold_left
    //   strict left fold by pulling a copy of the producer to exhaustion; the
    // accumulator is threaded by move so collecting folds stay O(n).
    //   D_CONSTEXPR -- a producer is not a literal type before C++20, and
    // the pull loop needs relaxed constexpr.
    template<typename Acc,
             typename Function>
    static D_CONSTEXPR_CPP14
    Acc fold_left(
        const Producer& _producer,
        Acc             _init,
        Function        _function
    )
    {
        Producer _cursor = _producer;

        while (true)
        {
            DProducerStep<value_type> _step = _cursor();

            if (!_step.has_value)
            {
                break;
            }

            _init = _function(std::move(_init), _step.value);
        }

        return _init;
    }
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_HPP
